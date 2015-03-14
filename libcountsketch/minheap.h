#ifndef __MIN_HEAP_H__
#define __MIN_HEAP_H__

///
///  最小(最大)堆简单实现，提供基本的入堆/出堆操作
///

#include <stdint.h>
#include <unistd.h>

#define MIN_HEAP (1)                // 使用最小堆，注释后按最大堆实现

// [二叉树节点数对照表]
//   节点数      效率           次/s
// 3 - 7        0.9463          317w
// 4 - 15       1.0000          300w
// 5 - 31       1.1103          270w
// 6 - 63       1.3544          221w
// 7 - 127      1.8497          162w
// 8 - 255      2.8300          106w
// 9 - 511      4.7618           63w
// 10- 1023     8.6343           34w
#define MAX_HEAP_SIZE (255)         // 建议范围按照 (2^n - 1) 定义，n越大效率越低

#define M_HEAP_LEFTCHILD(i) ((int)(2*i+1))
#define M_HEAP_PARENT(i) ((int)((int)(i/2)-1))

typedef struct {
    uint32_t dwKey;                 // 堆排序用Key，存储期望的估计值
    uint32_t dwData;                // 对应Key的数据内容
} MH_HeapNode;

typedef struct {
    int iMaxHeap;                   // 堆的最大大小(可以在7-MAX_HEAP_SIZE动态调整)
    int iCount;                     // 当前堆的大小
    MH_HeapNode stNode[MAX_HEAP_SIZE];
} MinHeap;

// 动态调整堆的大小(7-MAX_HEAP_SIZE之间)
int SetHeapSize(MinHeap *pstHeap, int iNewSize);
// 入堆，按dwKey的大小，插入到二叉树对应位置
int InHeap(MinHeap *pstHeap, uint32_t dwData, uint32_t dwKey);
// 出堆，弹出堆顶元素并重新建堆
int OutHeap(MinHeap *pstHeap, uint32_t *pdwData, uint32_t *pdwKey);

#ifdef MIN_HEAP
inline int Heap_MinFilterUp(MinHeap *pstHeap, int iStart);
inline int Heap_MinFilterDown(MinHeap *pstHeap, int iStart);
#else
inline int Heap_MaxFilterUp(MinHeap *pstHeap, int iStart);
inline int Heap_MaxFilterDown(MinHeap *pstHeap, int iStart);
#endif

#endif
