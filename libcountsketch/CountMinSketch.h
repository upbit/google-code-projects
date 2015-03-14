#ifndef __COUNT_MIN_SKETCH_H__
#define __COUNT_MIN_SKETCH_H__

///
///  Count-Min Sketch 算法的简单实现，用于挖掘频繁项
///                                木桩@2012
///
// 参考文献：
// Moses Charikar, Kevin Chen: << Finding Frequent Items in Data Streams >>
// Explaining The Count Sketch Algorithm: http://goo.gl/Atmon

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "minheap.h"

/* Debug output macros */
#define debug_printf(fmt, ...)	    fprintf(stdout, fmt, ## __VA_ARGS__)
// if set -DNO_DEBUG, disable DD/DE output
#ifdef NO_DEBUG
#define DD(fmt, ...)
#define DE(fmt, ...)
#else
#define DD(fmt, ...)	            fprintf(stdout, "[DEBUG] " fmt "\n", ## __VA_ARGS__)
#define DE(fmt, ...)	            fprintf(stderr, "[ERROR] " fmt "\n", ## __VA_ARGS__)
#endif

// 常规宏定义
#define ABS(a)   (((a)<0)?-(a):(a))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// 取abc的中位数
#define M_MID3(a, b, c) ( a < b ? \
    ( b < c ? b : (a < c ? c : a) ) : \
    ( b > c ? b : (a > c ? c : a) ) )

#define MAX_CMSKETCH_ESTIMATE (0x1FFFFFFF)          // 达到最大值后则停止采集

// CountSketch的共享内存结构
typedef struct {
    char cInited;                       // 标记共享内存是否初始化
    char cRunning;                      // 标记是否处于运行中(1时CMSketch_Add才有效)
    char cResv[2];
    
    MinHeap stCMHeap;                   // 用于存储UIN的最小堆
    
    uint32_t dwNodeCount;               // 下面 adwCount[] 中的节点总数
    uint32_t adwCount[0];               // 后面连续 dwNodeCount 的空间是实际的CountSketch结构
} CountSketchShm;

int CMSketch_Init(int iKey, int iTopN, volatile CountSketchShm **ppCMSketch);
int CMSketch_Free(volatile CountSketchShm *pCMSketch);
int CMSketch_Reset(volatile CountSketchShm *pCMSketch, int iTopN, int iRunning);

int CMSketch_Enable(volatile CountSketchShm *pCMSketch);
int CMSketch_Disable(volatile CountSketchShm *pCMSketch);

uint32_t CMSketch_GetSize(uint32_t auMods[], int iDepth);
uint32_t CMSketch_GetAverage(volatile CountSketchShm *pCMSketch);
int CMSketch_DumpHeap(volatile CountSketchShm *pCMSketch);

int CMSketch_Add(volatile CountSketchShm *pCMSketch, uint32_t dwData);
int CMSketch_Estimate(volatile CountSketchShm *pCMSketch, uint32_t dwData, uint32_t *pdwEstValue);

#endif
