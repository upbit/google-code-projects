#ifndef _MEM_CHECK_C_
#define _MEM_CHECK_C_

/*
 *  MemoryLeak Check Library for malloc / free.
 *                      @Version: 0.93
 *                      @Author: 木桩(2009)
 *
 *  Usage:
 *    Include "memcheck.h" in your project and replace all
 *    "malloc"/"free" with "__malloc"/"__free". At the end
 *    of your process, simply call the function "__memchk()"
 *    to print memory leak.
 *
 *
 *  === 使用说明 ===
 *
 *  将如下代码加入你项目的头文件中，并使用 __malloc/__calloc/__free 代替
 *  原来的 malloc/calloc/free 函数
 *  (如果有必要，在Makefile中加入 -DNO_MEMCHK 可以方便的屏蔽掉此模块)
 *

#ifdef NO_MEMCHK
  #define __malloc malloc
  #define __calloc calloc
  #define __free free
#else
  #include <memchk.h>
#endif

 *
 *  在你需要检查内存泄漏的时候(通常是程序终止时)，调用如下函数输出没有释放的内存
 *

#ifndef NO_MEMCHK
  __memchk();
#endif

 *
 *
 *  === How To Use ===
 *
 *  Place the following lines into your project header, and
 *  replace all malloc/calloc/free to __malloc/__calloc/__free
 *

#ifdef NO_MEMCHK
  #define __malloc malloc
  #define __calloc calloc
  #define __free free
#else
  #include <memchk.h>
#endif

 *
 *  and call __memchk() function when you need check your memory leak.
 *

#ifndef NO_MEMCHK
  __memchk();
#endif

 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// display all malloc / free message
#define __DEBUG__

#include <inttypes.h>
#if defined (__LP64__) || defined (__64BIT__) || defined (_LP64) || (__WORDSIZE == 64)
#define _IA64
#endif

/* export function */
void* _memchk_malloc(size_t num_bytes, const char* file, size_t line);
void* _memchk_calloc(size_t nelem, size_t elsize, const char* file, size_t line);
void _memchk_free(void *ptr, const char* file, size_t line);
#define MEMCHK_MALLOC(x) _memchk_malloc(x, __FILE__, __LINE__)
#define MEMCHK_CALLOC(n, el) _memchk_calloc(n, el, __FILE__, __LINE__)
#define MEMCHK_FREE(x) _memchk_free(x, __FILE__, __LINE__)
#define __malloc MEMCHK_MALLOC
#define __calloc MEMCHK_CALLOC
#define __free MEMCHK_FREE
void __memchk();

/* type define */
struct _memchk_trunk_list {
  char file_name[32];
  size_t line_num;
  size_t size;
  void* address;
  struct _memchk_trunk_list* next;
};

#endif
