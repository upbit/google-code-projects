#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shmmalloc.h"

#define TEST_SHM_KEY 0x01234567

typedef struct {
  int data1;
  unsigned long data2;
} demo_shm_head;

int main()
{
  demo_shm_head *pbuf = NULL;
  
  pbuf = shm_malloc(TEST_SHM_KEY, 0x1000);
  if (pbuf == NULL)
  {
    return -1;
  }

  printf("shm: %d, %lu\n", pbuf->data1, pbuf->data2);
  
  pbuf->data1 = -123;
  pbuf->data2 = 1234;
  
  return 0;
}
