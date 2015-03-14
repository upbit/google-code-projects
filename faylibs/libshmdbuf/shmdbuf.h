#ifndef SHARED_DOUBLE_BUFFER_H
#define SHARED_DOUBLE_BUFFER_H
/**
 * - Shared Memory of Double-Buffer (shmdbuf) -
 *                  @Version: 0.88
 *                  @Author: 木桩(2010)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#if defined (__LP64__) || defined (__64BIT__) || defined (_LP64) || (__WORDSIZE == 64)
#define _IA64
#endif

#ifndef __FreeBSD__
//#define USE_POSIX_MUTEX           // 使用POSIX共享互斥代替文件锁 (FreeBSD下无法使用)
#endif

/* Debug output macros */
#define shm_output(fmt, ...)	    fprintf(stdout, "" fmt, ## __VA_ARGS__)
// if set -DNO_DEBUG, disable DD/DE output
#ifdef NO_DEBUG
#define DD(fmt, ...)
#define DE(fmt, ...)
#else
#define DD(fmt, ...)	            fprintf(stdout, "[DEBUG] " fmt, ## __VA_ARGS__)
#define DE(fmt, ...)	            fprintf(stderr, "" fmt, ## __VA_ARGS__)
#endif

#ifdef PAGE_SIZE
#define SHM_PAGE_SIZE             PAGE_SIZE
#else
#define SHM_PAGE_SIZE             0x1000
#endif

#define INLINE_FUNC               inline

// 双缓冲结构中每个缓冲区的描述结构
// 映射内存的第一个页用于存储这个结构的数组
typedef struct _shmdbuf_hdr {
#ifdef USE_POSIX_MUTEX
  pthread_mutex_t g_mutex;        // 全局互斥结构，用于在块首等待块写入完毕 ( sizeof()=24 )
#endif
  size_t wr_sign;                 // 块写入标记，为1时表示该块正在被写入线程写入
  size_t record_size;             // 每个结构体大小
} shmdbuf_hdr_t;

// 读写状态记录结构，记录读取/写入位置和当前块等信息
typedef struct _shmdbuf_wr_rec {
  volatile size_t dbuf_index;     // 当前所处理的缓冲块
  volatile size_t wr_index;       // 当前的读/写位置(读写完毕后此位置+1)
} shmdbuf_wr_rec_t;

/**
 *  - 内部函数导出 -
 *   建议使用SHM_DBUF_INIT这样的函数别名来操作，以防后续版本更变函数参数和名称
 */
INLINE_FUNC int link_shm_doublebuffer(char *shm_name, size_t shm_buffer_size, size_t user_record_size, int force_write);
INLINE_FUNC void free_shm_doublebuffer(char *shm_name);
INLINE_FUNC int destory_shm_memory(char *shm_name); // 强制删除一个共享内存映射 (此函数用于在创建进程异常终止时删除shm映射)
INLINE_FUNC void lock_shm_doublebuffer(int index);
INLINE_FUNC void unlock_shm_doublebuffer(int index);
INLINE_FUNC void read_shm_doublebuffer(void *user_buffer);
INLINE_FUNC void write_shm_doublebuffer(void *user_buffer, int user_buf_size);
INLINE_FUNC void* get_shm_write_ptr(void);
INLINE_FUNC int is_shm_readonly_mode(void);
INLINE_FUNC void state_shm_doublebuffer(shmdbuf_wr_rec_t *stat_record);


///
///  函数别名的宏定义
///

/**
 * SHM_DBUF_INIT - 初始化共享双缓冲 (应该由数据写入方调用)
 * 
 * 此宏用于初始化共享双缓冲。当双缓冲不存在时将创建；当指定共享双缓冲存在时，
 * 重新连接到其上并继续向其中写入数据
 *
 *  input: shm_name - 共享内存名。其命名方式为 / 加上一个内存名。
 *           更详细的命名规则请参考 man shm_open
 *         shm_buffer_size - 单块共享内存的大小 (最终分配内存是此大小的两倍)
 *         user_record_size - 保存内存结构的大小 (即每次read/write的数据大小)
 * return: 1 - success, 0 - error
 * notice: 读取方不要调用此函数，该方法仅限于需要获得写入权限的情况，
 *         如果是读取进程，请参看 SHM_DBUF_LINK() 函数
 *
 *  usage: SHM_DBUF_INIT("/test", 64*0x100000, 128);
 */
#define SHM_DBUF_INIT(n,s,us)     link_shm_doublebuffer(n,s,us,1)
/**
 * SHM_DBUF_LINK - 连接到共享双缓冲
 * 
 * 此宏用于初始化/连接到共享双缓冲。当双缓冲不存在时将创建；
 * 与SHM_DBUF_INIT不同的是，当指定共享双缓冲存在时，此函数将默认以只读方式连接到共享内存
 *
 *  usage: if ( SHM_DBUF_LINK("/dbuf_name", 128*0x100000, 256) ) ...
 */
#define SHM_DBUF_LINK(n,s,us)     link_shm_doublebuffer(n,s,us,0)
/**
 * SHM_DBUF_FREE - 删除内存映射，并释放共享双缓冲(仅写入方可以释放共享缓冲区)
 * 
 * notice: 仅写入方可以释放共享缓冲区！当写入进程异常终止时，建议使用SHM_DBUF_INIT函数
 *         重新连接到共享内存，然后调用此函数释放。
 *
 *  input: shm_name - 共享内存名。
 *  usage: SHM_DBUF_FREE("/test");
 */
#define SHM_DBUF_FREE             free_shm_doublebuffer
/**
 * SHM_DBUF_READ - 从缓冲区中读取一个数据
 * 
 * 当有数据时，函数会将缓冲区中对应位置的数据复制到提供的用户缓冲区中
 * 如果没有可以读取的数据，函数会阻塞
 *
 *  input: user_buffer - 指向用户缓冲区的指针。读取出的数据会复制到此处
 *                       注意函数不检查缓冲区大小，请确保缓冲区长度不小于 user_record_size
 * notice: 每个进程共享同一个读取指针，也就是说同调进程中多线程调用此函数，每次取得的数据都是不同的
 *         另外，此函数是线程安全的。
 *  usage: char local_buffer[RECORD_SIZE];
 *         SHM_DBUF_READ(&local_buffer[0]);
 */
#define SHM_DBUF_READ             read_shm_doublebuffer
/**
 * SHM_DBUF_WRITE - 向缓冲区中写入一个数据
 * 
 * 当有数据时，函数会将缓冲区中对应位置的数据复制到提供的用户缓冲区中
 * 如果没有可以读取的数据，函数会阻塞
 *
 *  input: user_buffer - 指向用户缓冲区的指针
 *         user_buf_size - 用户提供数据的大小
 *            (如果不足user_record_size，后面的数据会用0x00填充)
 *            (如果输入非法数据，按照user_record_size长度填充)
 * notice: 每个进程共享同一个写入指针，不同线程写入数据会依次存放于缓冲区中
 *         此函数是线程安全的。
 *  usage: SHM_DBUF_WRITE(&write_buffer, -1);
 */
#define SHM_DBUF_WRITE            write_shm_doublebuffer
/**
 * SHM_DBUF_GET_WDPTR - 取得缓冲区中可以写入数据的位置指针
 * 
 * SHM_DBUF_WRITE() 的高级构造函数，用于取得可以写入数据的指针，并用于手动填充数据
 * 当调用此函数后，系统会默认写入操作已完成，并将写入指针+1
 *
 * return: 指向当前写入位置的指针
 * notice: 这个宏没有参数，注意后面不要加括号。
 *         此函数是线程安全的。
 *  usage: ring_pkg = (ring_pkg_hdr_t*) SHM_DBUF_GET_WDPTR;
 */
#define SHM_DBUF_GET_WDPTR        get_shm_write_ptr()
/**
 * SHM_IS_RDONLY_MODE - 检查当前共享内存的连接模式
 *
 * return: 0 - 写入模式, 1 - 只读模式
 *  usage: if ( SHM_IS_RDONLY_MODE ) ...
 */
#define SHM_IS_RDONLY_MODE        is_shm_readonly_mode()
/**
 * SHM_DBUF_STAT - 查看当前双缓冲的读写状态
 *
 *  input: stat_record - 状态记录结构
 *  usage: shmdbuf_wr_rec_t stat_record;
 *         SHM_DBUF_STAT(&stat_record);
 *         printf("DBUF[%d]=%10d\n", stat_record.dbuf_index, stat_record.wr_index);
 */
#define SHM_DBUF_STAT             state_shm_doublebuffer

#endif
