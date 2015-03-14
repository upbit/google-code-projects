#ifndef _SZ_FILELOG_H_
#define _SZ_FILELOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
  - File Log (filelog) -
                  @Version: 1.0
  
  - HOW TO USE -
  
>> Init log file
    logfile_t stLog;
    logfile_t *pstLog = &stLog;
    INIT_LOG(pstLog, "xxx.log", 100000);

>> Write message to log
    LOG_DEBUG(pstLog, "...");
    ...

>> Close log file
    FINI_LOG(pstLog);

 */

#define MAX_PATH_LEN (256)

typedef struct {
  unsigned long ulLogLines, ulMaxLines;
  FILE *fp;
  char szLogPath[MAX_PATH_LEN];
} logfile_t, LOG_FILE;

// inner function
int init_filelog(logfile_t *pLogFile, const char *szLogPath, const unsigned long ulMaxLognum);
int get_time_str(char *szBuffer);
int rolling_log(logfile_t *pLogFile);

/* Log Level */
#ifndef LOG_LEVEL
#error Please use -DLOG_LEVEL=3 to set log level. (0-Disable; 1-ERROR; 2-E,WARN; 3-E,W,DEBUG; 4-E,W,D,FLOW; 5-ALL)
#else
#define _RT_LOG_LEVEL LOG_LEVEL
#endif

#define LOG_TIME_LEVEL    1       // set 0 to disable
#define MAX_TIME_LEN      64

#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_FLOW    4
#define LOG_LEVEL_ALL     5

// MAIN LOG
#define FILE_LOG(flog, fmt2, ...) do { \
  if ((flog) == NULL) break; \
  if ((flog)->fp == NULL) break; \
  char m_local_tbuffer[MAX_TIME_LEN]; \
  if (++((flog)->ulLogLines) > (flog)->ulMaxLines) rolling_log(flog); \
  if (get_time_str(&m_local_tbuffer[0]) == 0) { \
    fprintf((flog)->fp, "%s%s:%d " fmt2 "\n", m_local_tbuffer, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
  }else { \
    fprintf((flog)->fp, "%s:%d " fmt2 "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__); \
  }\
  fflush((flog)->fp); \
}while (0);

#if _RT_LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(pstlog, fmt, ...) FILE_LOG(pstlog, "[ERROR] " fmt, ## __VA_ARGS__)
#else
#define LOG_ERROR(pstlog, fmt, ...)
#endif
#if _RT_LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(pstlog, fmt, ...)  FILE_LOG(pstlog, "[WARN] " fmt, ## __VA_ARGS__)
#else
#define LOG_WARN(pstlog, fmt, ...)
#endif
#if _RT_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(pstlog, fmt, ...) FILE_LOG(pstlog, "[DEBUG] " fmt, ## __VA_ARGS__)
#else
#define LOG_DEBUG(pstlog, fmt, ...)
#endif
#if _RT_LOG_LEVEL >= LOG_LEVEL_FLOW
#define LOG_FLOW(pstlog, fmt, ...)  FILE_LOG(pstlog, "[FLOW] " fmt, ## __VA_ARGS__)
#else
#define LOG_FLOW(pstlog, fmt, ...)
#endif

// 导出缓冲区
#if _RT_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DBG_BUFDUMP(flog, buf, len) do { \
  LOG_DEBUG(flog, "==== MEM_DUMP(%d) ====", len); \
  fprintf((flog)->fp, "            00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"); \
  fprintf((flog)->fp, "  ---------+-----------------------------------------------"); \
  int __imacro_dloop = 0; \
  for (__imacro_dloop = 0; __imacro_dloop < (int)len; __imacro_dloop++) \
  { \
    if (__imacro_dloop % 16 == 0) { \
      fprintf((flog)->fp, "\n  %.8X: ", (size_t)(buf)+__imacro_dloop); \
      ++(flog)->ulLogLines; \
    } \
    fprintf((flog)->fp, "%.2X ", *(unsigned char*)((size_t)(buf)+__imacro_dloop)); \
  } \
  fprintf((flog)->fp, "\n\n"); \
  fflush((flog)->fp); \
  (flog)->ulLogLines += 2; \
} while(0)
#else
#define LOG_DBG_BUFDUMP(flog, buf, len)
#endif

// int init_filelog(logfile_t *pLogFile, char *szLogPath, unsigned long ulMaxLognum);
#define INIT_LOG                  init_filelog
#define FINI_LOG(flog)            ( fclose((flog)->fp) )

#endif
