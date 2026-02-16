/* This file is part of msolve.
 *
 * Portability compatibility header for Windows (MinGW) cross-compilation.
 */

#ifndef MSOLVE_COMPAT_H
#define MSOLVE_COMPAT_H

#if defined(__MINGW32__) || defined(__MINGW64__) || defined(_WIN32)

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ---- posix_memalign ---- */
static inline int msolve_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    void *p = _aligned_malloc(size, alignment);
    if (p == NULL) {
        *memptr = NULL;
        return errno;
    }
    *memptr = p;
    return 0;
}
#define posix_memalign msolve_posix_memalign

/* ---- getline / getdelim ---- */
#ifndef MSOLVE_HAVE_GETLINE

static inline ssize_t msolve_getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    size_t pos = 0;
    int c;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = (char *)malloc(*n);
        if (*lineptr == NULL) {
            return -1;
        }
    }

    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n * 2;
            char *new_ptr = (char *)realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *lineptr = new_ptr;
            *n = new_size;
        }
        (*lineptr)[pos++] = (char)c;
        if (c == delim) {
            break;
        }
    }

    if (pos == 0 && c == EOF) {
        return -1;
    }

    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}

static inline ssize_t msolve_getline(char **lineptr, size_t *n, FILE *stream)
{
    return msolve_getdelim(lineptr, n, '\n', stream);
}

#define getline  msolve_getline
#define getdelim msolve_getdelim

#endif /* MSOLVE_HAVE_GETLINE */

#endif /* __MINGW32__ || __MINGW64__ || _WIN32 */

#endif /* MSOLVE_COMPAT_H */
