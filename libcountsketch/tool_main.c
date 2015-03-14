// Linux下用于操作和查看共享内存中数据的工具
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

static inline int ShowUsage(char *szCmdLine)
{
    debug_printf("Usage: %s <args>\n", szCmdLine);
    debug_printf("    -c   1: Dump CountSketch Heap\n");
    debug_printf("         4: Reset CountSketch\n");
    debug_printf("        11: Enable CountSketch\n");
    debug_printf("        12: Disable CountSketch\n");
    debug_printf("    -t  <TopN>: (default=15)\n");
    debug_printf("    -k  <ShmKey>: (default=0x%.8X)\n", CMS_SHM_KEY);
    debug_printf("    -r  --  Set Running=1 when Reset(4)\n");
    
    debug_printf("\n");

    return 0;
}

int main(int argc, char *argv[])
{
    int iRet = 0, iCmd = 0;
    char ch = 0;
    volatile CountSketchShm *pstCMSketch = NULL;
    
    // 输入参数
    int iShmKey = CMS_SHM_KEY;
    int iTopN = CMS_HEAP_NUM;
    int iRunning = 0;

    while ((ch=getopt(argc, argv, "Hhc:t:k:r"))!=EOF)
    {
        switch(ch)
        {
            case 'H':
            case 'h':
                ShowUsage(argv[0]);
                exit(0);

            case 'c':
                iCmd = atoi(optarg);
                break;

            case 't':
                iTopN = atoi(optarg);
                break;
            case 'k':
                iShmKey = atoi(optarg);
                break;

            case 'r':
                iRunning = 1;
                break;

            default:
                break;
        }
    }

    iRet = CMSketch_Init(iShmKey, iTopN, &pstCMSketch);
    if ((iRet != 0) || (NULL == pstCMSketch))
    {
        debug_printf("CMSketch_Init(key:0x%.8X, top:%d) error, iRet=%d\n", iShmKey, iTopN, iRet);
        return -11;
    }

    switch (iCmd)
    {
        case 1:     // Dump CountSketch Heap
            iRet = CMSketch_DumpHeap(pstCMSketch);
            if (iRet != 0)
            {
                debug_printf("[ERROR] CMSketch_DumpHeap() error, iRet=%d\n", iRet);
            }
            break;
        case 4:     // Reset CountSketch
            iRet = CMSketch_Reset(pstCMSketch, iTopN, iRunning);
            if (iRet != 0)
            {
                debug_printf("[ERROR] CMSketch_Reset(Top:%d, %d) error, iRet=%d\n", iTopN, iRunning, iRet);
            }
            debug_printf("CMSketch_Reset() success!\n");
            break;
        
        case 11:
            if (CMSketch_Enable(pstCMSketch) == 0)
                debug_printf("CMSketch_Enable() success!\n");
            break;
        case 12:
            if (CMSketch_Disable(pstCMSketch) == 0)
                debug_printf("CMSketch_Disable() success!\n");
            break;

        default:
            ShowUsage(argv[0]);
            break;
    }
    
    return CMSketch_Free(pstCMSketch);
}
 