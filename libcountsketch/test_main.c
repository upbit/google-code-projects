#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "minheap.h"
#include "CountMinSketch.h"

#define CMS_SHM_KEY (0x2A112501)
#define CMS_HEAP_NUM (15)

#ifndef MSDIFF
#define MSDIFF(newpar, old) (1000*(long long)((newpar).tv_sec - (old).tv_sec) \
							+ (newpar).tv_usec/1000 - (old).tv_usec/1000)
#endif

int Benchmark(volatile CountSketchShm *pstCMSketch)
{
    int i;
    
    CMSketch_Reset(pstCMSketch, CMS_HEAP_NUM, 1);
    
    for (i = 0; i < 100; i++)
        CMSketch_Add(pstCMSketch, 12345);
    for (i = 0; i < 1000; i++)
        CMSketch_Add(pstCMSketch, 224466);
    
    for (i = 0; i < 10000; i++)
    {
        CMSketch_Add(pstCMSketch, 369123456);
        CMSketch_Add(pstCMSketch, 469654321);
        CMSketch_Add(pstCMSketch, 369123456);
    }
    
    for (i = 0; i < 2000000; i++)
    {
        CMSketch_Add(pstCMSketch, random() % 100000);
    }

    return 0;
}

int TestCMSketchLoop(volatile CountSketchShm *pstCMSketch)
{
    int i = 0;
    int iRand = 0;

    while ((++i) < 0x001FFFFF)
    {
        iRand = random() % 100;
        
        if (iRand > 50)  // 50%
            CMSketch_Add(pstCMSketch, 369123456);
        if (iRand > 67)  // 33%
            CMSketch_Add(pstCMSketch, 12345);
        if (iRand > 90)  // 10%
            CMSketch_Add(pstCMSketch, 224466);
        if (iRand > 85)  // 15%
            CMSketch_Add(pstCMSketch, 469654321);

        CMSketch_Add(pstCMSketch, random() % 100000);
        usleep(100);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int iRet = 0;
    volatile CountSketchShm *pstCMSketch = NULL;
    struct timeval stStartTime, stEndTime;

    srand(time(NULL));

    iRet = CMSketch_Init(CMS_SHM_KEY, CMS_HEAP_NUM, &pstCMSketch);
    if ((iRet != 0) || (NULL == pstCMSketch))
    {
        debug_printf("CMSketch_Init() error, iRet=%d\n", iRet);
        return -11;
    }
    
    gettimeofday(&stStartTime, NULL);

    Benchmark(pstCMSketch);
    //TestCMSketchLoop(pstCMSketch);
    
    gettimeofday(&stEndTime, NULL);
    debug_printf("  CMSketch Cost: %u ms\n", (uint32_t)MSDIFF(stEndTime, stStartTime));

    CMSketch_DumpHeap(pstCMSketch);
    
    return CMSketch_Free(pstCMSketch);
}
