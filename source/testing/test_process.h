/* test_process.h - Process utilities for integration tests
 *
 * Provides fork/exec based process execution for integration tests,
 * replacing system() calls with proper process management.
 *
 * Features:
 * - Direct process execution without shell
 * - Proper exit code capture
 * - Temporary directory management with nftw cleanup
 */

#ifndef TEST_PROCESS_H
#define TEST_PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ftw.h>
#include <errno.h>

/* Maximum path length for temp directories */
#define TEST_PROC_MAX_PATH 512

/* =============================================================================
 * Process Execution
 * =============================================================================
 */

/**
 * Execute a binary with arguments using fork/exec.
 *
 * @param binary_path  Path to the binary to execute
 * @param args         NULL-terminated array of arguments (argv[0] should be program name)
 * @return             Exit code of the process, or -1 on fork/exec failure
 *
 * Example:
 *   char *args[] = {"psnd", "play", "test.joy", NULL};
 *   int result = test_exec(PSND_BINARY, args);
 */
static inline int test_exec(const char *binary_path, char *const args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        /* Fork failed */
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        /* Redirect stdout and stderr to /dev/null for quiet tests */
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        execv(binary_path, args);

        /* execv only returns on error */
        _exit(127);
    }

    /* Parent process - wait for child */
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    /* Process was killed by signal */
    return -1;
}

/**
 * Execute a binary with arguments, capturing stdout.
 *
 * @param binary_path  Path to the binary to execute
 * @param args         NULL-terminated array of arguments
 * @param output       Buffer to store stdout (can be NULL)
 * @param output_size  Size of output buffer
 * @return             Exit code of the process, or -1 on failure
 */
static inline int test_exec_capture(const char *binary_path, char *const args[],
                                     char *output, size_t output_size) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        close(pipefd[0]);  /* Close read end */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        /* Redirect stderr to /dev/null */
        freopen("/dev/null", "w", stderr);

        execv(binary_path, args);
        _exit(127);
    }

    /* Parent process */
    close(pipefd[1]);  /* Close write end */

    if (output && output_size > 0) {
        ssize_t bytes_read = read(pipefd[0], output, output_size - 1);
        if (bytes_read >= 0) {
            output[bytes_read] = '\0';
        } else {
            output[0] = '\0';
        }
    }
    close(pipefd[0]);

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}

/* =============================================================================
 * Temporary Directory Management
 * =============================================================================
 */

/**
 * Create a unique temporary directory for tests.
 *
 * @param prefix  Prefix for the directory name (e.g., "psnd_test")
 * @param path    Buffer to store the created path (must be at least TEST_PROC_MAX_PATH)
 * @return        0 on success, -1 on failure
 *
 * Example:
 *   char temp_dir[TEST_PROC_MAX_PATH];
 *   test_mkdtemp("psnd_test", temp_dir);
 *   // Use temp_dir...
 *   test_rmdir_recursive(temp_dir);
 */
static inline int test_mkdtemp(const char *prefix, char *path) {
    snprintf(path, TEST_PROC_MAX_PATH, "/tmp/%s_XXXXXX", prefix);
    if (mkdtemp(path) == NULL) {
        return -1;
    }
    return 0;
}

/* Helper callback for nftw to remove files/directories */
static inline int test_rmdir_callback(const char *fpath, const struct stat *sb,
                                       int typeflag, struct FTW *ftwbuf) {
    (void)sb;
    (void)typeflag;
    (void)ftwbuf;
    return remove(fpath);
}

/**
 * Recursively remove a directory and all its contents.
 *
 * @param path  Path to the directory to remove
 * @return      0 on success, -1 on failure
 *
 * Note: Uses nftw() instead of system("rm -rf ...") for safety and portability.
 */
static inline int test_rmdir_recursive(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    /* Safety check: don't remove root or home directories */
    if (strcmp(path, "/") == 0 || strcmp(path, getenv("HOME")) == 0) {
        return -1;
    }

    /* Use nftw to traverse and remove */
    return nftw(path, test_rmdir_callback, 64, FTW_DEPTH | FTW_PHYS);
}

/**
 * Write content to a file in a directory.
 *
 * @param dir      Directory path
 * @param filename Filename (no path separators)
 * @param content  Content to write
 * @return         0 on success, -1 on failure
 */
static inline int test_write_file(const char *dir, const char *filename,
                                   const char *content) {
    char path[TEST_PROC_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    if (fputs(content, f) == EOF) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

/**
 * Build a file path from directory and filename.
 *
 * @param dir      Directory path
 * @param filename Filename
 * @param path     Buffer to store result (must be at least TEST_PROC_MAX_PATH)
 */
static inline void test_build_path(const char *dir, const char *filename, char *path) {
    snprintf(path, TEST_PROC_MAX_PATH, "%s/%s", dir, filename);
}

#endif /* TEST_PROCESS_H */
