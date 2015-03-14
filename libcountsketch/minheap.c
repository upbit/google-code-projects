#include "minheap.h"

// 上浮指定位置的节点(用于插入后调整节点位置)
#ifdef MIN_HEAP
inline int Heap_MinFilterUp(MinHeap *pstHeap, int iStart)
#else
inline int Heap_MaxFilterUp(MinHeap *pstHeap, int iStart)
#endif
{
    if (NULL == pstHeap)
        return -1;
    if ((iStart < 0) || (iStart >= pstHeap->iCount))
        return -2;

    int i = 0, j = 0;
    MH_HeapNode stTmpNode = {0};

    j = iStart;
    i = M_HEAP_PARENT(j);
    stTmpNode = pstHeap->stNode[j];
    if (i < 0) i = 0;
    
    while (j > 0)
    {
        //DD("(%d > 0) ?? H[%d]=%d <= %d", j, i, pstHeap->stNode[i].dwKey, stTmpNode.dwKey);
#ifdef MIN_HEAP
        if (pstHeap->stNode[i].dwKey >= stTmpNode.dwKey)
#else
        if (pstHeap->stNode[i].dwKey <= stTmpNode.dwKey)
#endif
        {
            pstHeap->stNode[j] = pstHeap->stNode[i];
            j = i;
            i = M_HEAP_PARENT(i);
            if (i < 0) i = 0;
        }
        else
        {
            break;
        }
    }
    
    //DD("H[%d]=%d <-- %d", j, pstHeap->stNode[j].dwKey, stTmpNode.dwKey);
    pstHeap->stNode[j] = stTmpNode;
    return 0;
}

// 下沉指定位置节点(删除节点后需要用到此操作)
#ifdef MIN_HEAP
inline int Heap_MinFilterDown(MinHeap *pstHeap, int iStart)
#else
inline int Heap_MaxFilterDown(MinHeap *pstHeap, int iStart)
#endif
{
    if (NULL == pstHeap)
        return -1;
    if ((iStart < 0) || (iStart >= pstHeap->iCount))
        return -2;

    int i = 0, j = 0;
    MH_HeapNode stTmpNode = {0};

    i = iStart;
    j = M_HEAP_LEFTCHILD(i);
    stTmpNode = pstHeap->stNode[i];

    while (j < pstHeap->iCount)
    {
#ifdef MIN_HEAP
        //DD("(%d < %d) && (H[%d]=%u > H[%d]=%u)", j, pstHeap->iCount-1, j, pstHeap->stNode[j].dwKey, j+1, pstHeap->stNode[j+1].dwKey);
        if ((j < pstHeap->iCount-1) && (pstHeap->stNode[j].dwKey > pstHeap->stNode[j+1].dwKey)) j++;
        
        //DD("? temp=%u <= H[%d]=%u ?", stTmpNode.dwKey, j, pstHeap->stNode[j].dwKey);
        if (stTmpNode.dwKey >= pstHeap->stNode[j].dwKey)
#else
        if ((j < pstHeap->iCount-1) && (pstHeap->stNode[j].dwKey < pstHeap->stNode[j+1].dwKey)) j++;
        
        if (stTmpNode.dwKey <= pstHeap->stNode[j].dwKey)
#endif
        {
            pstHeap->stNode[i] = pstHeap->stNode[j];
            i = j;
            j = M_HEAP_LEFTCHILD(i);
        }
        else
        {
            break;
        }
    }
    
    //DD("H[%d]=%d <-- %d", i, pstHeap->stNode[i].dwKey, stTmpNode.dwKey);
    pstHeap->stNode[i] = stTmpNode;
    return 0;
}

// 入堆，按dwKey的大小，插入到二叉树对应位置
inline int InHeap(MinHeap *pstHeap, uint32_t dwData, uint32_t dwKey)
{
    if (NULL == pstHeap)
        return -1;
    if (0 == pstHeap->iMaxHeap)
        return -2;
    
    int iRet = 0;
    int iNewElement = pstHeap->iCount;

    if (pstHeap->iCount+1 > pstHeap->iMaxHeap)
        return -100;
    
    pstHeap->stNode[iNewElement].dwKey = dwKey;
    pstHeap->stNode[iNewElement].dwData = dwData;
    pstHeap->iCount++;

#ifdef MIN_HEAP
    iRet = Heap_MinFilterUp(pstHeap, iNewElement);
#else
    iRet = Heap_MaxFilterUp(pstHeap, iNewElement);
#endif
    if (iRet != 0)
    {
        //DE("Heap_FilterUp(%d) error, iRet=%d", iNewElement, iRet);
        return (-100+iRet);
    }

    return 0;
}

// 出堆，弹出堆顶元素并重新建堆
inline int OutHeap(MinHeap *pstHeap, uint32_t *pdwData, uint32_t *pdwKey)
{
    if ((NULL == pstHeap) || (NULL == pdwData))
        return -1;

    int i, iRet = 0;
    int iLastElement = pstHeap->iCount-1;
    
    *pdwData = 0;
    if (pdwKey != NULL)
        *pdwKey = 0;
    
    if (pstHeap->iCount <= 0)
        return -100;

    if (iLastElement > 0)
    {
        *pdwData = pstHeap->stNode[0].dwData;
        if (pdwKey != NULL)
            *pdwKey = pstHeap->stNode[0].dwKey;
        
        pstHeap->stNode[0] = pstHeap->stNode[iLastElement];
        pstHeap->iCount--;
        
        for (i = (pstHeap->iCount/2); i >= 0; i--)
        {
#ifdef MIN_HEAP
            iRet = Heap_MinFilterDown(pstHeap, i);
#else
            iRet = Heap_MaxFilterDown(pstHeap, i);
#endif
            if (iRet != 0)
            {
                //DE("Heap_FilterDown(%d) error, iRet=%d", i, iRet);
                return (-100+iRet);
            }
        }
    }
    else
    {
        *pdwData = pstHeap->stNode[0].dwData;
        if (pdwKey != NULL)
            *pdwKey = pstHeap->stNode[0].dwKey;
        
        //pstHeap->iCount--;
        pstHeap->iCount = 0;
    }

    return 0;
}

// 动态调整堆的大小
inline int SetHeapSize(MinHeap *pstHeap, int iNewSize)
{
    if (NULL == pstHeap)
        return -1;
    if ((iNewSize < 7) || (iNewSize > MAX_HEAP_SIZE))
        return -2;
    
    int i;
    uint32_t dwTmpUin = 0;
    
    if (pstHeap->iMaxHeap > iNewSize)
    {
        // 调小，需要出堆
        for (i = pstHeap->iCount; i > iNewSize; i--)
        {
            OutHeap(pstHeap, &dwTmpUin, NULL);
        }
        
        pstHeap->iMaxHeap = iNewSize;
    }
    else
    {
        // 调大，直接设置
        pstHeap->iMaxHeap = iNewSize;
    }

    return 0;
}
