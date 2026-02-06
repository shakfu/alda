/*
 * Dirent interface for Microsoft Visual Studio
 *
 * Copyright (C) 1998-2019 Toni Ronkko
 * This file is part of dirent.  Dirent may be freely distributed
 * under the MIT license.  For all details and documentation, see
 * https://github.com/tronkko/dirent
 *
 * Minimal implementation for Csound compatibility on Windows.
 */
#ifndef DIRENT_H
#define DIRENT_H

/* Hide warnings about unreferenced local functions */
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable:4505)
#elif defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wunused-function"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

/* Windows includes */
#include <windows.h>

/* POSIX stat macros for Windows */
#ifndef S_ISDIR
#	define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#	define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISLNK
#	define S_ISLNK(m) (0)  /* Windows doesn't have symlinks in the traditional sense */
#endif

/* Maximum length of file name */
#if !defined(PATH_MAX)
#	define PATH_MAX MAX_PATH
#endif

/* File type flags for d_type */
#define DT_UNKNOWN 0
#define DT_REG 8
#define DT_DIR 4
#define DT_FIFO 1
#define DT_SOCK 12
#define DT_CHR 2
#define DT_BLK 6
#define DT_LNK 10

/* Macros for converting between st_mode and d_type */
#define IFTODT(mode) ((mode) & 0170000)
#define DTTOIF(type) ((type) << 12)

/* File type macros */
#define	_IFMT 0170000
#define	_IFDIR 0040000
#define	_IFCHR 0020000
#define	_IFBLK 0060000
#define	_IFREG 0100000
#define	_IFLNK 0120000
#define	_IFSOCK 0140000
#define	_IFIFO 0010000

/* Directory entry structure */
struct dirent {
    char d_name[PATH_MAX + 1];  /* File name */
    size_t d_namlen;            /* Length of file name */
    int d_type;                 /* File type */
};

/* Directory stream */
typedef struct _WDIR {
    struct dirent ent;          /* Current directory entry */
    WIN32_FIND_DATAW data;      /* Private file data */
    int cached;                 /* True if data is valid */
    int invalid;                /* True if handle is invalid */
    HANDLE handle;              /* Win32 search handle */
    wchar_t *patt;              /* Search pattern */
} DIR;

/* Internal functions */
static int _wclosedir(DIR *dirp);

/* Open directory stream */
static DIR *opendir(const char *dirname)
{
    DIR *dirp;
    int error;
    wchar_t wname[PATH_MAX + 1];
    size_t n;

    /* Convert directory name to wide character */
    error = mbstowcs_s(&n, wname, PATH_MAX + 1, dirname, PATH_MAX);
    if (error)
        return NULL;

    /* Allocate directory stream */
    dirp = (DIR*) malloc(sizeof(DIR));
    if (!dirp)
        return NULL;

    /* Allocate search pattern */
    dirp->patt = (wchar_t*) malloc(sizeof(wchar_t) * (n + 16));
    if (!dirp->patt) {
        free(dirp);
        return NULL;
    }

    /* Construct search pattern */
    wcscpy_s(dirp->patt, n + 16, wname);
    if (n > 0) {
        wchar_t c = dirp->patt[n - 1];
        if (c != '\\' && c != '/' && c != ':') {
            dirp->patt[n] = '\\';
            n++;
        }
    }
    dirp->patt[n] = '*';
    dirp->patt[n + 1] = '\0';

    /* Open stream and retrieve first entry */
    dirp->handle = FindFirstFileW(dirp->patt, &dirp->data);
    if (dirp->handle == INVALID_HANDLE_VALUE) {
        /* Failed to open directory */
        DWORD errorcode = GetLastError();
        if (errorcode == ERROR_ACCESS_DENIED ||
            errorcode == ERROR_FILE_NOT_FOUND ||
            errorcode == ERROR_PATH_NOT_FOUND) {
            free(dirp->patt);
            free(dirp);
            return NULL;
        }
    }

    /* Initialize directory entry */
    dirp->cached = 1;
    dirp->invalid = (dirp->handle == INVALID_HANDLE_VALUE);
    return dirp;
}

/* Read next directory entry */
static struct dirent *readdir(DIR *dirp)
{
    /* Return NULL if no more entries */
    if (dirp->invalid)
        return NULL;

    /* Load next entry if previous was already returned */
    if (!dirp->cached) {
        if (!FindNextFileW(dirp->handle, &dirp->data)) {
            dirp->invalid = 1;
            return NULL;
        }
    }
    dirp->cached = 0;

    /* Convert file name to multibyte */
    size_t n;
    wcstombs_s(&n, dirp->ent.d_name, PATH_MAX + 1, dirp->data.cFileName, PATH_MAX);
    dirp->ent.d_namlen = n - 1;

    /* Determine file type */
    DWORD attr = dirp->data.dwFileAttributes;
    if (attr & FILE_ATTRIBUTE_DEVICE) {
        dirp->ent.d_type = DT_CHR;
    } else if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
        dirp->ent.d_type = DT_LNK;
    } else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        dirp->ent.d_type = DT_DIR;
    } else {
        dirp->ent.d_type = DT_REG;
    }

    return &dirp->ent;
}

/* Close directory stream */
static int closedir(DIR *dirp)
{
    return _wclosedir(dirp);
}

/* Close directory stream internal */
static int _wclosedir(DIR *dirp)
{
    int ok;

    if (!dirp)
        return -1;

    /* Close handle */
    if (dirp->handle != INVALID_HANDLE_VALUE)
        ok = FindClose(dirp->handle);
    else
        ok = 1;

    /* Free resources */
    free(dirp->patt);
    free(dirp);

    return ok ? 0 : -1;
}

/* Rewind directory stream */
static void rewinddir(DIR *dirp)
{
    if (!dirp)
        return;

    /* Close previous handle */
    if (dirp->handle != INVALID_HANDLE_VALUE)
        FindClose(dirp->handle);

    /* Re-open stream */
    dirp->handle = FindFirstFileW(dirp->patt, &dirp->data);
    dirp->cached = 1;
    dirp->invalid = (dirp->handle == INVALID_HANDLE_VALUE);
}

#if defined(_MSC_VER)
#	pragma warning(pop)
#elif defined(__clang__)
#	pragma clang diagnostic pop
#endif

#endif /* DIRENT_H */
