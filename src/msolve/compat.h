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

/* ---- posix_memalign and memory overrides ---- */
/* MinGW/MSVC uses _aligned_malloc which is NOT compatible with free().
   To support posix_memalign() alongside free() calls in the codebase,
   we must override malloc/free to use the aligned versions consistently.
*/

static inline void *msolve_aligned_calloc(size_t nmemb, size_t size)
{
    size_t total_size = nmemb * size;
    void *ptr = _aligned_malloc(total_size, 16);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

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

/* Override standard allocators to use aligned versions, ensuring compatibility
   with posix_memalign's behavior on Windows */
#ifdef free
#undef free
#endif
#define free(p) _aligned_free((p))

#ifdef malloc
#undef malloc
#endif
#define malloc(s) _aligned_malloc((s), 16)

#ifdef calloc
#undef calloc
#endif
#define calloc(n, s) msolve_aligned_calloc((n), (s))

#ifdef realloc
#undef realloc
#endif
#define realloc(p, s) _aligned_realloc((p), (s), 16)

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
        *lineptr = (char *)calloc(*n, 1);
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
            /* zero the newly allocated region so callers that
               scan from *n-1 backwards never hit uninitialised bytes */
            memset(new_ptr + *n, 0, new_size - *n);
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
    /* msolve's iofiles.c passes &len (the buffer capacity *n) to helper
       functions like remove_newlines() and remove_trailing_delim(), which
       treat it as the string length and index from *n-1 backwards.
       glibc's getdelim typically returns a tight buffer so *n ≈ pos+1,
       making that usage safe.  Replicate that here so the same code works. */
    *n = pos;
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
