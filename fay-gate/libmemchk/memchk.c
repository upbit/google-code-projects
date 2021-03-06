﻿/*
 *  MemoryLeak Check Library for malloc / free.
 *                            By 木桩(2009)
 *
 *  Usage:
 *    Include "memcheck.h" in your project and replace all
 *    "malloc"/"free" with "__malloc"/"__free". At the end
 *    of your process, simply call the function "__memchk()"
 *    to print memory leak.
 */
#include "memchk.h"

// 链首
struct _memchk_trunk_list* mem_trunk_start = NULL;

/* memchk overload */
inline
void* _memchk_malloc(size_t num_bytes, const char* file, size_t line)
{
  void* ptr_malloc = malloc(num_bytes);
  struct _memchk_trunk_list* ptrunk_list;
  
  // 记录分配的内存，并挂到 mem_trunk_start 链头部
  ptrunk_list = mem_trunk_start;
  mem_trunk_start = (struct _memchk_trunk_list*) malloc(sizeof(struct _memchk_trunk_list));
  if (strlen(file) < 32) {
    strcpy(&mem_trunk_start->file_name[0], file);
  }else {
    strncpy(&mem_trunk_start->file_name[0], file, 31);
    mem_trunk_start->file_name[31] = '\0';
  }
  mem_trunk_start->line_num = line;
  mem_trunk_start->address = ptr_malloc;
  mem_trunk_start->size = num_bytes;
  mem_trunk_start->next = ptrunk_list;

#ifdef __DEBUG__
  #ifdef _IA64
    fprintf(stderr, "[MEMCHK: DEBUG] (%s:%lu), malloc(%lu) = %.8lX\n", file, line, num_bytes, (size_t)ptr_malloc);
  #else
    fprintf(stderr, "[MEMCHK: DEBUG] (%s:%u), malloc(%u) = %.8X\n", file, line, num_bytes, (size_t)ptr_malloc);
  #endif
#endif
  return ptr_malloc;
}

inline
void* _memchk_calloc(size_t nelem, size_t elsize, const char* file, size_t line)
{
  void* ptr_calloc = calloc(nelem, elsize);
  struct _memchk_trunk_list* ptrunk_list;
  
  // 记录分配的内存，并挂到 mem_trunk_start 链头部
  ptrunk_list = mem_trunk_start;
  mem_trunk_start = (struct _memchk_trunk_list*) malloc(sizeof(struct _memchk_trunk_list));
  if (strlen(file) < 32) {
    strcpy(&mem_trunk_start->file_name[0], file);
  }else {
    strncpy(&mem_trunk_start->file_name[0], file, 31);
    mem_trunk_start->file_name[31] = '\0';
  }
  mem_trunk_start->line_num = line;
  mem_trunk_start->address = ptr_calloc;
  mem_trunk_start->size = nelem * elsize;
  mem_trunk_start->next = ptrunk_list;

#ifdef __DEBUG__
  #ifdef _IA64
    fprintf(stderr, "[MEMCHK: DEBUG] (%s:%lu), malloc(%lu) = %.8lX\n", file, line, nelem*elsize, (size_t)ptr_calloc);
  #else
    fprintf(stderr, "[MEMCHK: DEBUG] (%s:%u), malloc(%u) = %.8X\n", file, line, nelem*elsize, (size_t)ptr_calloc);
  #endif
#endif
  return ptr_calloc;
}

inline
void _memchk_free(void *ptr, const char* file, size_t line)
{
  int fit_free = 0;
  struct _memchk_trunk_list *ptrunk_list, *per_node, *pfree_node;
  
  // 从 mem_trunk_start 链中删除对应的节点
  ptrunk_list = mem_trunk_start; per_node = NULL;
  while (ptrunk_list != NULL) {
    if (ptrunk_list->address == ptr) {
      // 匹配到结果，删除对应记录
      if (per_node == NULL) {
        pfree_node = mem_trunk_start;
        mem_trunk_start = mem_trunk_start->next;
      }else {
        pfree_node = ptrunk_list;
        per_node->next = ptrunk_list->next;
      }
      free(pfree_node);
      fit_free = 1;
      break;
    }
    per_node = ptrunk_list;
    ptrunk_list = ptrunk_list->next;
  }
  
  if (!fit_free)
#ifdef _IA64
    fprintf(stderr, "[MEMCHK: WARRING] (%s:%lu), free(%.8lX) but address not malloced!\n", file, line, (size_t)ptr);
#else
    fprintf(stderr, "[MEMCHK: WARRING] (%s:%u), free(%.8X) but address not malloced!\n", file, line, (size_t)ptr);
#endif
  
#ifdef __DEBUG__
  #ifdef _IA64
    fprintf(stderr, "[MEMCHK: DEBUG] (%s:%lu), free(0x%.8lX)\n", file, line, (size_t)ptr);
  #else
    fprintf(stderr, "[MEMCHK: DEBUG] (%s:%u), free(0x%.8X)\n", file, line, (size_t)ptr);
  #endif
#endif
  return free(ptr);
}

// 报告尚未释放的内存
inline
void __memchk()
{
  struct _memchk_trunk_list *ptrunk_list, *pfree_node;
  
  ptrunk_list = mem_trunk_start;
  while (ptrunk_list != NULL) {
#ifdef _IA64
    fprintf(stderr, "[MEMCHK] (%s:%lu) malloced address 0x%.8lX not free, %lu bytes leaked!\n",
            ptrunk_list->file_name, ptrunk_list->line_num, (size_t)ptrunk_list->address, ptrunk_list->size);
#else
    fprintf(stderr, "[MEMCHK] (%s:%u) malloced address 0x%.8X not free, %u bytes leaked!\n",
            ptrunk_list->file_name, ptrunk_list->line_num, (size_t)ptrunk_list->address, ptrunk_list->size);
#endif
    
    pfree_node = ptrunk_list;
    ptrunk_list = ptrunk_list->next;
    free(pfree_node);
  }
}
 