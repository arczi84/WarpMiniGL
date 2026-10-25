#ifndef QUARKTEX_GCC_COMPAT_H
#define QUARKTEX_GCC_COMPAT_H
/* StormC's register declarations, supported by the Amiga GCC fork. */
#define __REGA0(x) x __asm("a0")
#define __REGA1(x) x __asm("a1")
#define __REGA2(x) x __asm("a2")
#define __REGA3(x) x __asm("a3")
#define __REGA4(x) x __asm("a4")
#define __REGA5(x) x __asm("a5")
#define __REGA6(x) x __asm("a6")
#define __REGD0(x) x __asm("d0")
#define __REGD1(x) x __asm("d1")
#define __REGD2(x) x __asm("d2")
#define __REGD3(x) x __asm("d3")
#define __REGD4(x) x __asm("d4")
#define __REGD5(x) x __asm("d5")
#define __REGD6(x) x __asm("d6")
#define __REGD7(x) x __asm("d7")
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#endif
