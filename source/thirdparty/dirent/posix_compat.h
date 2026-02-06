/*
 * POSIX compatibility macros for Windows
 *
 * This header provides POSIX-style stat macros that are missing
 * from Windows' sys/stat.h
 */
#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

#include <sys/stat.h>

/* S_ISDIR - test for a directory */
#ifndef S_ISDIR
#   ifdef S_IFDIR
#       define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#   else
#       define S_ISDIR(m) (0)
#   endif
#endif

/* S_ISREG - test for a regular file */
#ifndef S_ISREG
#   ifdef S_IFREG
#       define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#   else
#       define S_ISREG(m) (0)
#   endif
#endif

/* S_ISLNK - test for a symbolic link (not supported on Windows) */
#ifndef S_ISLNK
#   define S_ISLNK(m) (0)
#endif

/* S_ISCHR - test for a character device */
#ifndef S_ISCHR
#   ifdef S_IFCHR
#       define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#   else
#       define S_ISCHR(m) (0)
#   endif
#endif

/* S_ISBLK - test for a block device (not supported on Windows) */
#ifndef S_ISBLK
#   define S_ISBLK(m) (0)
#endif

/* S_ISFIFO - test for a FIFO/pipe */
#ifndef S_ISFIFO
#   ifdef S_IFIFO
#       define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#   else
#       define S_ISFIFO(m) (0)
#   endif
#endif

/* S_ISSOCK - test for a socket (not supported on Windows) */
#ifndef S_ISSOCK
#   define S_ISSOCK(m) (0)
#endif

#endif /* POSIX_COMPAT_H */
