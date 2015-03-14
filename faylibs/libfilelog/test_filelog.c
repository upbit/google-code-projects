/*
 *  libfilelog/ demo, please use `make test` to compile this program
**/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "filelog.h"

logfile_t stLog;
logfile_t *pstLog = &stLog;

int main()
{
  INIT_LOG(pstLog, "./flog.log", 4);
  
  LOG_ERROR(pstLog, "error test");
  LOG_WARN(pstLog, "warring test");
  LOG_DEBUG(pstLog, "debug test");
  LOG_FLOW(pstLog, "flow point test");
  
  LOG_DEBUG(pstLog, "try rolling log file...");
  LOG_FLOW(pstLog, "done");
  
  // 测试DUMP结构体内容到日志
  LOG_DBG_BUFDUMP(pstLog, pstLog, sizeof(logfile_t));
  
  FINI_LOG(pstLog);
  
  return 0;
}
