#ifndef PSND_DIRENT_H
#define PSND_DIRENT_H

/**
 * @file psnd_dirent.h
 * @brief Directory iteration that also works under MSVC.
 *
 * Several places need to walk a directory: the TOML theme and language loaders
 * scan .psnd/ config directories, and the Alda example test scans its corpus.
 * POSIX gives them <dirent.h>; MSVC does not ship one, so this header supplies
 * a FindFirstFileA wrapper exposing the same names.
 *
 * Only opendir/readdir/closedir and the d_name field are provided - that is
 * what the callers use. There is no d_type, no rewinddir, and no ordering
 * guarantee beyond the platform's own.
 */

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct dirent {
    char d_name[MAX_PATH];
};

typedef struct {
    HANDLE handle;
    WIN32_FIND_DATAA find_data;
    struct dirent entry;
    int first;
} DIR;

static DIR *opendir(const char *path)
{
    char pattern[MAX_PATH];
    DIR *dir = (DIR *)malloc(sizeof(DIR));
    if (!dir) return NULL;

    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    dir->handle = FindFirstFileA(pattern, &dir->find_data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        free(dir);
        return NULL;
    }
    dir->first = 1;
    return dir;
}

static struct dirent *readdir(DIR *dir)
{
    if (!dir) return NULL;
    if (dir->first) {
        dir->first = 0;
    } else if (!FindNextFileA(dir->handle, &dir->find_data)) {
        return NULL;
    }
    strncpy(dir->entry.d_name, dir->find_data.cFileName, MAX_PATH - 1);
    dir->entry.d_name[MAX_PATH - 1] = '\0';
    return &dir->entry;
}

static void closedir(DIR *dir)
{
    if (!dir) return;
    FindClose(dir->handle);
    free(dir);
}

#else

#include <dirent.h>

#endif /* _WIN32 */

#endif /* PSND_DIRENT_H */
