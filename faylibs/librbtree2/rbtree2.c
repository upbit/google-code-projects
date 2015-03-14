/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  linux/include/linux/rbtree.h

  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...

  Some example of insert and search follows here. The search is a plain
  normal search over an ordered tree. The insert instead must be implemented
  in two steps: First, the code must insert the element in order as a red leaf
  in the tree, and then the support library function rb_insert_color() must
  be called. Such function will do the not trivial work to rebalance the
  rbtree, if necessary.

-----------------------------------------------------------------------
static inline struct page * rb_search_page_cache(struct inode * inode,
             unsigned long offset)
{
  struct rb_node * n = inode->i_rb_page_cache.rb_node;
  struct page * page;

  while (n)
  {
    page = rb_entry(n, struct page, rb_page_cache);

    if (offset < page->offset)
      n = n->rb_left;
    else if (offset > page->offset)
      n = n->rb_right;
    else
      return page;
  }
  return NULL;
}

static inline struct page * __rb_insert_page_cache(struct inode * inode,
               unsigned long offset,
               struct rb_node * node)
{
  struct rb_node ** p = &inode->i_rb_page_cache.rb_node;
  struct rb_node * parent = NULL;
  struct page * page;

  while (*p)
  {
    parent = *p;
    page = rb_entry(parent, struct page, rb_page_cache);

    if (offset < page->offset)
      p = &(*p)->rb_left;
    else if (offset > page->offset)
      p = &(*p)->rb_right;
    else
      return page;
  }

  rb_link_node(node, parent, p);

  return NULL;
}

static inline struct page * rb_insert_page_cache(struct inode * inode,
             unsigned long offset,
             struct rb_node * node)
{
  struct page * ret;
  if ((ret = __rb_insert_page_cache(inode, offset, node)))
    goto out;
  rb_insert_color(node, &inode->i_rb_page_cache);
 out:
  return ret;
}
-----------------------------------------------------------------------
*/
#include "rbtree2.h"

static void __rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *right = node->rb_right;
  struct rb_node *parent = rb_parent(node);

  if ((node->rb_right = right->rb_left))
    rb_set_parent(right->rb_left, node);
  right->rb_left = node;

  rb_set_parent(right, parent);

  if (parent)
  {
    if (node == parent->rb_left)
      parent->rb_left = right;
    else
      parent->rb_right = right;
  }
  else
    root->rb_node = right;
  rb_set_parent(node, right);
}

static void __rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *left = node->rb_left;
  struct rb_node *parent = rb_parent(node);

  if ((node->rb_left = left->rb_right))
    rb_set_parent(left->rb_right, node);
  left->rb_right = node;

  rb_set_parent(left, parent);

  if (parent)
  {
    if (node == parent->rb_right)
      parent->rb_right = left;
    else
      parent->rb_left = left;
  }
  else
    root->rb_node = left;
  rb_set_parent(node, left);
}

// 在root中插入node结构
void rb_insert_color(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *parent, *gparent;

  while ((parent = rb_parent(node)) && rb_is_red(parent))
  {
    gparent = rb_parent(parent);

    if (parent == gparent->rb_left)
    {
      {
        register struct rb_node *uncle = gparent->rb_right;
        // CASE 1: recolor
        if (uncle && rb_is_red(uncle))
        {
          rb_set_black(uncle);
          rb_set_black(parent);
          rb_set_red(gparent);
          node = gparent;
          continue;
        }
      }

      if (parent->rb_right == node)
      {
        // CASE 2: l-rotate(P)
        register struct rb_node *tmp;
        __rb_rotate_left(parent, root);
        tmp = parent; // 注意此时x和P交换过来了
        parent = node;
        node = tmp;
      }

      // CASE 3: r-rotate(GP) + recolor
      rb_set_black(parent);
      rb_set_red(gparent);
      __rb_rotate_right(gparent, root);
    } else {

      // 在左侧的话，和上面动作相反操作
      {
        register struct rb_node *uncle = gparent->rb_left;
        if (uncle && rb_is_red(uncle))
        {
          rb_set_black(uncle);
          rb_set_black(parent);
          rb_set_red(gparent);
          node = gparent;
          continue;
        }
      }

      if (parent->rb_left == node)
      {
        register struct rb_node *tmp;
        __rb_rotate_right(parent, root);
        tmp = parent;
        parent = node;
        node = tmp;
      }

      rb_set_black(parent);
      rb_set_red(gparent);
      __rb_rotate_left(gparent, root);
    }
  }

  // 不要忘了最后要把root节点标黑
  rb_set_black(root->rb_node);
}

/*
  Simple Red-Black-Tree Function (insert/search)
*/
// 在proot中查找ulkey对应的inode节点，rtn_parent用于返回父节点的指针
struct inode * rb_search_user_node(unsigned long ulkey, struct rb_root *root)
{
  struct rb_node * n = root->rb_node;
  struct inode * unode;

  while (n)
  {
    unode = rb_entry(n, struct inode, node);
    
    if (ulkey < rb_key(unode))
      n = n->rb_left;
    else if (ulkey > rb_key(unode))
      n = n->rb_right;
    else
      return unode;
  }
  
  return NULL;
}

static inline struct inode * __rb_insert_user_node(struct rb_node *node,
             unsigned long ulkey,
             struct rb_root *root)
{
  struct rb_node ** p = &root->rb_node;
  struct rb_node * parent = NULL;
  struct inode * unode;

  while (*p)
  {
    parent = *p;
    unode = rb_entry(parent, struct inode, node);

    if (ulkey < rb_key(unode))
      p = &(*p)->rb_left;
    else if (ulkey > rb_key(unode))
      p = &(*p)->rb_right;
    else
      return unode;
  }

  rb_link_node(node, parent, p);
  
  return NULL;
}

// 将node插入root中
struct inode * rb_insert_user_node(struct rb_node *node, struct rb_root *root)
{
  struct inode * ret;
  
  ret = rb_entry(node, struct inode, node);
  if ((ret = __rb_insert_user_node(node, rb_key(ret), root)))
    goto out;   // ret != NULL, 说明插入节点已经存在，直接返回
  rb_insert_color(node, root);
  
 out:
  return ret;
}
