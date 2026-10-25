/* A resident library must not depend on application CRT startup state. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stddef.h>
struct allocation { ULONG size, reserved; };
void *malloc(size_t size)
{
    struct allocation *p;
    if (!size) size = 1;
    if (size > 0x7ffffff0UL) return 0;
    p = AllocMem(size + sizeof(*p), MEMF_PUBLIC);
    if (!p) return 0;
    p->size = size + sizeof(*p);
    return p + 1;
}
void free(void *ptr)
{
    struct allocation *p;
    if (!ptr) return;
    p = (struct allocation *)ptr - 1;
    FreeMem(p, p->size);
}
void *memset(void *ptr, int c, size_t n)
{
    unsigned char *p = ptr;
    while (n--) *p++ = c;
    return ptr;
}
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}
