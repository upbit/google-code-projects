#include "CountMinSketch.h"

#ifndef __CYGWIN__
#include <sys/shm.h>
#include <sys/mman.h>
#else
#define IPC_CREAT (1000)
#endif

// 
// 2719x7能够达到99.9%的精确度: http://www.cis.hut.fi/Opinnot/T-61.5060/2007/t615060-l-2007-09-17.pdf
#define COUNTSKETCH_DEPTH_2719 (7)
uint32_t CMS_auMods_2719[COUNTSKETCH_DEPTH_2719] = {
    2719, 2713, 2711, 2707, 2699, 2693, 2689,
};

static inline char* MyGetShmWithFlag(key_t iKey, size_t ulSize, int iFlag)
{
    char* sShm = NULL;
#ifndef __CYGWIN__
    int iShmID = 0;
    char sErrMsg[50] = {0};

    if ((iShmID = shmget(iKey, ulSize, iFlag)) < 0) {
        sprintf(sErrMsg, "shmget %d %lu", iKey, ulSize);
        perror(sErrMsg);
        return NULL;
    }
    if ((sShm = shmat(iShmID, NULL ,0)) == (char *) -1) {
        perror("shmat");
        return NULL;
    }
    if (mlock(sShm, ulSize))
    {
        perror("mlock");
        return NULL;
    }
#else
    if ((iFlag & IPC_CREAT) == IPC_CREAT)
        sShm = malloc(ulSize);
#endif

    return sShm;
}

static inline int MyDetachShm(char *pShmAddr, size_t ulSize)
{
    if ((NULL == pShmAddr) || (ulSize <= 0))
        return -1;
#ifndef __CYGWIN__
    int iRet = 0;

    iRet = munlock(pShmAddr, ulSize);
    if (iRet < 0)
    {
        perror("munlock");
        return (-100+iRet);
    }

    iRet = shmdt(pShmAddr);
    if (iRet < 0)
    {
        perror("shmdt");
        return (-200+iRet);
    }
#else
    free(pShmAddr);
#endif
    return 0;
}

//
inline uint32_t CMSketch_GetSize(uint32_t auMods[], int iDepth)
{
    if ((NULL == auMods) || (iDepth <= 0))
        return 0;

    int i;
    uint32_t dwSize = 0;

    for (i = 0; i < iDepth; i++)
        dwSize += auMods[i];

    return dwSize;
}

// 测试并将指定UIN插入堆的对应位置
static inline int _CMS_TestAddHeap(volatile CountSketchShm *pCMSketch, uint32_t dwData)
{
    // 内部static inline函数不重复判断指针
    
    int i, iRet = 0, iCount = 0;
    uint32_t dwEstimate = 0, dwTmpEstimate = 0;
    uint32_t dwTmpData = 0;
    MinHeap *pstHeap = (MinHeap *)&(pCMSketch->stCMHeap);

    // 取得当前UIN的估计值
    iRet = CMSketch_Estimate(pCMSketch, dwData, &dwEstimate);
    DD("CMSketch_Estimate(%u) iRet=%d, Estimate=%u", dwData, iRet, dwEstimate);

    if (dwEstimate > MAX_CMSKETCH_ESTIMATE)
    {
        // 如果某个计数已经达到最大值，强制停止采集
        pCMSketch->cRunning = 2;
    }

    iCount = MIN(pstHeap->iCount, MAX_HEAP_SIZE);
    
    // 先确定dwData是否已经在Heap中
    for (i = 0; i < iCount; i++)
    {
        if (dwData == pstHeap->stNode[i].dwData)
        {
            //DD("found %u in Heap[%d], dwEstimate=%u -> %u", dwData, i, pstHeap->stNode[i].dwKey, dwEstimate);
            
            // 找到则更新排序用Key
            pstHeap->stNode[i].dwKey = dwEstimate;
            return 0;
        }
    }
    
    // 堆还有空间，直接新增
    if (iCount < MAX_HEAP_SIZE)
    {
        DD("heap data(%d/%d), InHeap(%u, %u)", iCount, MAX_HEAP_SIZE, dwData, dwEstimate);
        InHeap(pstHeap, dwData, dwEstimate);
        return 0;
    }
    
    // 没有空间，与堆顶最小的uin比较，如果新的大则入堆
    dwTmpEstimate = pstHeap->stNode[0].dwKey;
    dwTmpData = pstHeap->stNode[0].dwData;
    DD("Heap[0].dwData=%u, dwEstimate: %u, new dwEstimate=%u", dwTmpData, dwTmpEstimate, dwEstimate);
    
    if (dwEstimate > dwTmpEstimate)
    {
        iRet = OutHeap(pstHeap, &dwTmpData, NULL);
        DD("first-OutHeap() iRet=%d, dwData=%u", iRet, dwTmpData);
    }
    
    iRet = InHeap(pstHeap, dwData, dwEstimate);
    DD("second-InHeap(%u, %u) iRet=%d", dwData, dwEstimate, iRet);
    return 0;
}

// 增加指定UIN在CM中的计数
int CMSketch_Add(volatile CountSketchShm *pCMSketch, uint32_t dwData)
{
    if (NULL == pCMSketch)
        return -1;
    // 非运行状态则直接返回
    if (pCMSketch->cRunning != 1)
        return 0;

    int i;
    uint32_t dwHashIndex = 0;
    uint32_t *pdwCountSketch = NULL, *pdwCMCount = NULL;
    
    pdwCountSketch = (uint32_t*)pCMSketch->adwCount;
    
    for (i = 0; i < COUNTSKETCH_DEPTH_2719; i++)
    {
        dwHashIndex = dwData % CMS_auMods_2719[i];
        pdwCMCount = pdwCountSketch + dwHashIndex;

        *pdwCMCount += 1;
        DD("[%d|%u] Count[%p]=%d", i, dwHashIndex, pdwCMCount, *pdwCMCount);

        pdwCountSketch += CMS_auMods_2719[i];
    }

    return _CMS_TestAddHeap(pCMSketch, dwData);
}

// 计算DataCount的期望
int CMSketch_Estimate(volatile CountSketchShm *pCMSketch, uint32_t dwData, uint32_t *pdwEstValue)
{
    if ((NULL == pCMSketch) || (NULL == pdwEstValue))
        return -1;

    int i;
    uint32_t dwHashIndex = 0, dwMinEstimate = 0xFFFFFFFF;
    uint32_t *pdwCountSketch = NULL, *pdwCMCount = NULL;

    *pdwEstValue = 0;
    pdwCountSketch = (uint32_t*)pCMSketch->adwCount;

    for (i = 0; i < COUNTSKETCH_DEPTH_2719; i++)
    {
        dwHashIndex = dwData % CMS_auMods_2719[i];
        pdwCMCount = pdwCountSketch + dwHashIndex;
        
        dwMinEstimate = MIN(dwMinEstimate, *pdwCMCount);
        //DD("est hash(%u)=%u, value=%u", dwData, CMS_auMods_2719[i], dwMinEstimate);

        pdwCountSketch += CMS_auMods_2719[i];
    }

    *pdwEstValue = dwMinEstimate;
    return 0;
}

// 初始化CMSketch结构
inline int CMSketch_Reset(volatile CountSketchShm *pCMSketch, int iTopN, int iRunning)
{
    if (NULL == pCMSketch)
        return -1;
    if ((iTopN < 7) || (iTopN > MAX_HEAP_SIZE))
        return -2;
    
    int iRet = 0;
    uint32_t dwCMSketchSize = 0;

    dwCMSketchSize = sizeof(CountSketchShm);
    dwCMSketchSize += CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719) * sizeof(uint32_t);
    
    memset((char *)pCMSketch, 0, dwCMSketchSize);
    
    pCMSketch->cInited = 1;
    pCMSketch->dwNodeCount = CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719);
    
    iRet = SetHeapSize((MinHeap*)&(pCMSketch->stCMHeap), iTopN);
    if (iRet != 0)
    {
        DE("SetHeapSize(%d) error, iRet=%d", iTopN, iRet);
        return -11;
    }
    
    if (iRunning)
        pCMSketch->cRunning = 1;

    return 0;
}

// 初始化CountSketch结构的共享内存，返回结构的指针
int CMSketch_Init(int iKey, int iTopN, volatile CountSketchShm **ppCMSketch)
{
    if (iKey <= 0)
        return -1;
    if ((iTopN < 7) || (iTopN > MAX_HEAP_SIZE))
        return -2;

    int iRet = 0, iNewCreate = 0;
    uint32_t dwCMSketchSize = 0;
    volatile CountSketchShm *pTmpCMSketch = NULL;
    
    *ppCMSketch = NULL;
    
    dwCMSketchSize = sizeof(CountSketchShm);
    dwCMSketchSize += CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719) * sizeof(uint32_t);
    
    DD("Count-Min Sketch Struct Size: %0.2f KB\n", (double)dwCMSketchSize/1024);
    
    pTmpCMSketch = (volatile CountSketchShm *)MyGetShmWithFlag(iKey, dwCMSketchSize, 0666);
    if (NULL == pTmpCMSketch)
    {
        pTmpCMSketch = (volatile CountSketchShm *)MyGetShmWithFlag(iKey, dwCMSketchSize, 0666|IPC_CREAT);
        if (pTmpCMSketch != NULL)
        {
            iNewCreate = 1;
        }
    }
    if (NULL == pTmpCMSketch)
        return -13;

    if ((iNewCreate) || (pTmpCMSketch->cInited != 1))
    {
        DD(">> memset new(%d) memory at [0x%.8x - 0x%.8x]", iNewCreate, (uint32_t)pTmpCMSketch, (uint32_t)pTmpCMSketch+dwCMSketchSize);

        iRet = CMSketch_Reset(pTmpCMSketch, iTopN, 0);
        if (iRet != 0)
        {
            DE("CMSketch_Reset(%d) error, iRet=%d", iTopN, iRet);
            
            CMSketch_Free(pTmpCMSketch);
            return -21;
        }
    }
    else
    {
        if (pTmpCMSketch->dwNodeCount != CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719))
        {
            DE("Shm exist but node count %u != Node2719:%u", pTmpCMSketch->dwNodeCount, CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719));
            
            CMSketch_Free(pTmpCMSketch);
            return -26;
        }
        
        DD("Link to exists Shm:0x%.8X", iKey);
    }
    
    *ppCMSketch = pTmpCMSketch;
    
    return 0;
}

int CMSketch_Free(volatile CountSketchShm *pCMSketch)
{
    if (NULL == pCMSketch)
        return -1;

    uint32_t dwCMSketchSize = 0;

    dwCMSketchSize = sizeof(CountSketchShm);
    dwCMSketchSize += CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719) * sizeof(uint32_t);

    DD("Free::MyDetachShm(0x%p) size=%u", pCMSketch, dwCMSketchSize);
    return MyDetachShm((char*)pCMSketch, dwCMSketchSize);
}

inline int CMSketch_Enable(volatile CountSketchShm *pCMSketch)
{
    if (NULL == pCMSketch)
        return -1;
    
    pCMSketch->cRunning = 1;
    return 0;
}

inline int CMSketch_Disable(volatile CountSketchShm *pCMSketch)
{
    if (NULL == pCMSketch)
        return -1;
    
    pCMSketch->cRunning = 0;
    return 0;
}

// 取得CountSketch结构的平均期望(用于降噪)
inline uint32_t CMSketch_GetAverage(volatile CountSketchShm *pCMSketch)
{
    if (NULL == pCMSketch)
        return -1;
    
    int i, j;
    uint64_t ddwAverage = 0;
    uint32_t *pdwCountSketch = NULL, *pdwCMCount = NULL;
    
    pdwCountSketch = (uint32_t*)pCMSketch->adwCount;
    for (i = 0; i < COUNTSKETCH_DEPTH_2719; i++)
    {
        for (j = 0; j  < CMS_auMods_2719[i]; j++)
        {
            pdwCMCount = pdwCountSketch + j;
            ddwAverage += (*pdwCMCount);
        }

        pdwCountSketch += CMS_auMods_2719[i];
    }
    
    return (uint32_t)(ddwAverage / CMSketch_GetSize(CMS_auMods_2719, COUNTSKETCH_DEPTH_2719));
}

// 输出堆中的数据
int CMSketch_DumpHeap(volatile CountSketchShm *pCMSketch)
{
    if (NULL == pCMSketch)
        return -1;
    
    int iRet = 0;
    uint32_t dwData = 0, dwKey = 0, dwEstimate = 0;
    uint32_t dwMinEstimate = 0;
    MinHeap stTmpHeap = {0};
    
    // 复制出来，防止破坏原始堆
    stTmpHeap = pCMSketch->stCMHeap;
    
    // 计算平均期望(堆中低于平均的数据可能不准)
    dwMinEstimate = CMSketch_GetAverage(pCMSketch);
    debug_printf(">>> CountSketch Avg-Estimate: %u, Heap Size(%d/%d)\n", dwMinEstimate, stTmpHeap.iCount, stTmpHeap.iMaxHeap);
    
    while (stTmpHeap.iCount > 0)
    {
        // 依次取出堆顶元素，直到堆空
        iRet = OutHeap(&stTmpHeap, &dwData, &dwKey);
        if (iRet != 0)
        {
            DE("OutHeap() error, iRet=%d, dwData=%u dwKey=%u", iRet, dwData, dwKey);
            return -11;
        }
        
        iRet = CMSketch_Estimate(pCMSketch, dwData, &dwEstimate);
        if (dwEstimate > dwMinEstimate)
        {
            // 输出估计值 dwEstimate-dwMinEstimate
            debug_printf(">> Count[%u] = %u, dwEstimate(%d)=%u(%u)\n", dwData, dwKey, iRet, dwEstimate, dwEstimate-dwMinEstimate);
        }
        else
        {
           debug_printf(" - Count[%u] = %u, dwEstimate(%d)=%u\n", dwData, dwKey, iRet, dwEstimate);
        }
    }

    return 0;
}
 