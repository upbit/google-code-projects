#include "rbtree2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
#define MAX_QUEUE_LEN   (256)
typedef struct
{
  unsigned long st, ed;
  void * queue[MAX_QUEUE_LEN];
} TinyQueue;

void tinyqueue_init(TinyQueue * ty_queue)
{
  memset(ty_queue, 0, sizeof(TinyQueue));
}

int tinyqueue_put(TinyQueue * ty_queue, void * p)
{
  if (p == NULL) return -1;
  
  unsigned long next_ed = (ty_queue->ed+1) % MAX_QUEUE_LEN;
  if (ty_queue->st == next_ed) return 0;          // 队列已满
  
  ty_queue->queue[ty_queue->ed] = p;
  //printf("put[%lu-%lu] = 0x%.8X\n", ty_queue->st, next_ed, (unsigned int)ty_queue->queue[ty_queue->ed]);
  ty_queue->ed = next_ed;
  
  return 1;
}

void * tinyqueue_get(TinyQueue * ty_queue)
{
  void * ret = NULL;
  //printf("get[%lu-%lu] = 0x%.8X\n", ty_queue->st, ty_queue->ed, (unsigned int)ty_queue->queue[ty_queue->st]);
  
  if (ty_queue->st != ty_queue->ed) {
    ret = ty_queue->queue[ty_queue->st];
    ty_queue->st = (ty_queue->st+1) % MAX_QUEUE_LEN;
  }
  return ret;
}
*/

struct rb_root st_rbtree = RB_ROOT;

// 用户自定义结构，此结构体头部必须与 rbtree2.h: struct inode {} 保持一致
struct test_node
{
  struct rb_node node;
  unsigned long key;
  // char data[]
  char str_data[64];
};

/*
void rbtree_hierarchy_traversal(struct rb_root * root)
{
  TinyQueue queue;
  struct rb_node * n = root->rb_node;
  struct inode * unode;
  
  tinyqueue_init(&queue);
  tinyqueue_put(&queue, n);
  
  while (n)
  {
    n = tinyqueue_get(&queue);
    if (n == NULL) break;
    if (n->rb_left != NULL) tinyqueue_put(&queue, n->rb_left);
    if (n->rb_right != NULL) tinyqueue_put(&queue, n->rb_right);
    
    unode = rb_entry(n, struct inode, node);
    if (unode == NULL)
    {
      printf(">> ERROR: inode is null at %.8X", (unsigned int)n);
      break;
    }
    printf("  (%lu: 0x%.8X)\n", unode->key, (unsigned int)unode);
  }
}
*/

int main()
{
  int i;
  struct test_node *pnode, *psearch;
  
  srand((unsigned)time(NULL));
  
  for (i = 0; i < 10; i++)
  {
    pnode = calloc(1, sizeof(struct test_node));
    pnode->key = rand() % 10;             // 随机指定key值，key决定节点被插入红黑树的哪个位置
    snprintf(pnode->str_data, 63, "this text key is %lu", pnode->key);
    
    // 向红黑树中插入用户节点。参数1是要插入的rb_node，参数2是红黑树的根节点
    psearch = (struct test_node*) rb_insert_user_node(&pnode->node, &st_rbtree);
    //rbtree_hierarchy_traversal(&st_rbtree);
    if (psearch == NULL)
    {
      printf("insert(%lu, '%s')\n", pnode->key, pnode->str_data);
    }
    else                                  // 返回不为空说明节点已经存在，psearch为树中同key节点的指针
    {
      printf("%lu is exists, search key: %lu\n", pnode->key, psearch->key);
      free(pnode);
    }
  }
  
  printf("-----------------------------------\n");
  
  for (i = 0; i < 10; i++)
  {
    psearch = NULL;
    // 按key在红黑树中查找节点
    psearch = (struct test_node*) rb_search_user_node(i, &st_rbtree);
    if (psearch == NULL) {
      printf("key %d is null\n", i);
    }else {
      printf("search(%d/%lu) = '%s'\n", i, psearch->key, psearch->str_data);
    }
  }
  
  // 注意：暂时没有删除函数，这里存在内存泄漏 :(
  
  return 0;
}
 