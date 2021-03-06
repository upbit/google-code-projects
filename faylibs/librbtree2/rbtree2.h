﻿#ifndef _LIB_RBTREE_2_
#define _LIB_RBTREE_2_

#include <stdio.h>
#include <stddef.h>

/*
  This code is modified from Linux Kernel 2.6.36 @ include\linux\Rbtree.h
*/

struct rb_node
{
  unsigned long  rb_parent_color;
#define RB_RED    0
#define RB_BLACK  1
  struct rb_node *rb_right;
  struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
    /* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
  struct rb_node *rb_node;
};

// 父节点指针，因为4字节对其，末尾比特位用于标记颜色
#define rb_parent(r)   ((struct rb_node *)((r)->rb_parent_color & ~3))    // 预留2个比特位
#define rb_color(r)    ((r)->rb_parent_color & 1)                         // 实际只用了最低位
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->rb_parent_color |= 1; } while (0)

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
  rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rb_node *rb, int color)
{
  rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define RB_ROOT	(struct rb_root) { NULL, }
#define	rb_entry(ptr, type, member) container_of(ptr, type, member)		// 通过结构中成员变量member的指针ptr，取得结构的指针

#define RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)	(rb_parent(node) == node)
#define RB_CLEAR_NODE(node)	(rb_set_parent(node, node))

extern void rb_insert_color(struct rb_node *, struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
// node是要链接的节点，rb_link是parent左右指针中的一个，也就是node要连接的位置
static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
				struct rb_node ** rb_link)
{
	node->rb_parent_color = (unsigned long )parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

struct inode {
  struct rb_node node;
  unsigned long key;        // 决定结构体被放在红黑树中的哪个位置
  char data[];
};
#define rb_key(r)   (*(unsigned long*)(&((struct inode *)r)->key))


// 在proot中查找ulkey对应的inode节点，rtn_parent用于返回父节点的指针
struct inode * rb_search_user_node(unsigned long ulkey, struct rb_root *root);
// 将node插入root中
struct inode * rb_insert_user_node(struct rb_node *node, struct rb_root *root);

#endif
 