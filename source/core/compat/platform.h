/* platform.h - Cross-platform compatibility layer
 *
 * Provides portable equivalents for POSIX functions used throughout psnd.
 * Include this header instead of directly including platform-specific headers.
 *
 * Covers:
 * - File operations (access, stat)
 * - Environment (getenv)
 * - Terminal detection (isatty)
 * - Sleep functions (usleep, sleep)
 * - String functions (strcasecmp, strdup)
 * - Path handling
 */

#ifndef PSND_PLATFORM_H
#define PSND_PLATFORM_H

#ifdef _WIN32

/* ======================= Windows ========================================== */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* File descriptor operations */
#define isatty _isatty
#define fileno _fileno
#define read _read
#define write _write
#define close _close
#define dup _dup
#define dup2 _dup2

/* File access modes for _access() */
#ifndef F_OK
#define F_OK 0  /* Existence */
#endif
#ifndef R_OK
#define R_OK 4  /* Read permission */
#endif
#ifndef W_OK
#define W_OK 2  /* Write permission */
#endif
#ifndef X_OK
#define X_OK 1  /* Execute permission (always succeeds on Windows) */
#endif

#define access _access

/* Directory operations */
#define getcwd _getcwd
#define chdir _chdir
#define mkdir(path, mode) _mkdir(path)
#define rmdir _rmdir

/* String functions */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

/* strdup is available in Windows CRT but may need explicit declaration */
#ifndef strdup
#define strdup _strdup
#endif

/* Sleep functions */
static inline void usleep(unsigned int usec) {
    /* Windows Sleep() takes milliseconds */
    Sleep((usec + 999) / 1000);
}

static inline unsigned int sleep(unsigned int seconds) {
    Sleep(seconds * 1000);
    return 0;
}

/* Standard file descriptors */
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

/* Path separator */
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"

/* Maximum path length */
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

/* getenv is available on Windows */
/* setenv is not available, but we can define it if needed */
static inline int setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && getenv(name) != NULL) {
        return 0;
    }
    return _putenv_s(name, value);
}

static inline int unsetenv(const char *name) {
    return _putenv_s(name, "");
}

/* Process functions */
#define getpid _getpid

/* Pipe - Windows uses different function */
static inline int pipe(int pipefd[2]) {
    return _pipe(pipefd, 4096, 0);
}

#else

/* ======================= POSIX (macOS, Linux) ============================= */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* For strcasecmp on some systems */
#include <stdio.h>

/* Path separator */
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"

/* PATH_MAX should be defined in limits.h */
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#endif /* _WIN32 */

/* ======================= Common Utilities ================================= */

/* Safe string copy with null termination */
static inline void psnd_strlcpy(char *dst, const char *src, size_t size) {
    if (size == 0) return;
    size_t i;
    for (i = 0; i < size - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* Check if path is absolute */
static inline int psnd_path_is_absolute(const char *path) {
    if (!path || !path[0]) return 0;
#ifdef _WIN32
    /* Windows: C:\... or \\server\... */
    if ((path[0] >= 'A' && path[0] <= 'Z' || path[0] >= 'a' && path[0] <= 'z') && path[1] == ':') {
        return 1;
    }
    if (path[0] == '\\' && path[1] == '\\') {
        return 1;
    }
    return 0;
#else
    return path[0] == '/';
#endif
}

/* Get home directory */
static inline const char *psnd_get_home_dir(void) {
#ifdef _WIN32
    static char home[PATH_MAX];
    const char *userprofile = getenv("USERPROFILE");
    if (userprofile) return userprofile;
    const char *homedrive = getenv("HOMEDRIVE");
    const char *homepath = getenv("HOMEPATH");
    if (homedrive && homepath) {
        snprintf(home, sizeof(home), "%s%s", homedrive, homepath);
        return home;
    }
    return NULL;
#else
    return getenv("HOME");
#endif
}

#endif /* PSND_PLATFORM_H */
