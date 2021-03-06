﻿#ifndef _SHM_MALLOC_H_
#define _SHM_MALLOC_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

/*
  - Share Memory Malloc (shmmalloc) -
                  @Version: 1.0
  
  - HOW TO USE -
  
>> Create/Link to share memory
    #define TEST_SHM_KEY 0x01234567
    
    pbuf = shm_malloc(TEST_SHM_KEY, 0x1000);
    if (pbuf == NULL) return -1;

 */

// malloc/calloc
void *shm_malloc(key_t key, uint32_t size);
void *shm_calloc(key_t key, uint32_t size);

///
///  注意，这里不提供共享内存的删除函数！如果必须释放，请尝试 ipcrm -M <key> 的方法
///
//int shm_free(...)

#endif
 