/************************************************
 *
 *  Share Memory Malloc Library (2010)
 *
 ************************************************/
#include "shmmalloc.h"
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_FLAGS (0666)

// 返回值: 0-新建; 1-连接到已存在的; 负值-错误
static inline
int __shm_malloc(key_t key, uint32_t size, void **pbuf)
{
  int shmid;
  int exist = 1;
  
  shmid = shmget(key, size, SHM_FLAGS);     // try link to exist share memory
  if (shmid == -1)
  {
    // create new share memory
    exist = 0;
    shmid = shmget(key, size, (SHM_FLAGS | IPC_CREAT));
    if (shmid == -1)
    {
      perror("shmget");
      return -2;
    }
  }
  
  *pbuf = shmat(shmid, (void *) 0, 0);
  if (*pbuf == (void *)(-1)) {
    perror("shmat");
    return -3;
  }
  
  return exist;
}

void *shm_malloc(key_t key, uint32_t size)
{
  void *pdata = NULL;
  int iret = 0;
  
  iret = __shm_malloc(key, size, &pdata);
  if (iret >= 0)
  {
    if (iret == 0)
      fprintf(stderr, "%s(0x%.8X, %lu): alloc memory at 0x%.8X\n", __FUNCTION__, key, (size_t)size, (size_t)pdata);
    else
      fprintf(stderr, "%s(0x%.8X, %lu): link shm at 0x%.8X\n", __FUNCTION__, key, (size_t)size, (size_t)pdata);
    
    return pdata;
  }
  else
  {
    return NULL;
  }
}

void *shm_calloc(key_t key, uint32_t size)
{
  void *pdata = NULL;
  int iret = 0;
  
  iret = __shm_malloc(key, size, &pdata);
  if (iret >= 0)
  {
    if (iret == 0)
      fprintf(stderr, "%s(0x%.8X, %lu): alloc memory at 0x%.8X\n", __FUNCTION__, key, (size_t)size, (size_t)pdata);
    else
      fprintf(stderr, "%s(0x%.8X, %lu): link shm at 0x%.8X\n", __FUNCTION__, key, (size_t)size, (size_t)pdata);
    
    memset(pdata, 0, size);
    return pdata;
  }
  else
  {
    return NULL;
  }
}
