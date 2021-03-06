﻿#ifndef FAY_CORE_C
#define FAY_CORE_C
#include "fay_original.h"
#include <signal.h>

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
fay_rc_t *fayrc;

#define SHMBUF_NAME         "/p2p_ringbuffer"
#define SHMBUF_SIZE         64*0x100000     // 64M
#define RECORD_SIZE         (sizeof(ring_pkg_hdr_t))

/*****************************************************************************/
/* Functions - init / free                                                   */
/*****************************************************************************/
// 分配并初始化 fayrc
void init_fayoriginal_resource()
{
  fayrc = (fay_rc_t*) __calloc(1, sizeof(fay_rc_t));
  if (fayrc == NULL) {
    perror("malloc");
    exit(0);
  }
  //memset(fayrc, 0, sizeof(fay_rc_t));
  
  SHM_DBUF_INIT(SHMBUF_NAME, SHMBUF_SIZE, RECORD_SIZE);
  
  return;
}

// 释放 fayrc 所占的空间
void free_fayoriginal_resource()
{
  if (fayrc != NULL) {
    __free(fayrc);
    fayrc = NULL;
  }
  SHM_DBUF_FREE(SHMBUF_NAME);
  
#ifndef NO_MEMCHK
  __memchk();
#endif
  return;
}
#endif
 