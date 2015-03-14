/*
 * - Shared Memory of Double-Buffer (shmdbuf) - 
 */
#include "shmdbuf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>          /* for pthread_spinlock */
#include <fcntl.h>                /* for O_* constants */
#include <errno.h>
#include <string.h>

/*****************************************************************************/
/*  Variable                                                                 */
/*****************************************************************************/
#define SHM_BUF_COUNT         2   // 分配的两个缓冲区

// 共享内存的fd
int shm_fd;

// 多线程读写时的自旋锁
pthread_spinlock_t shm_rd_spinlock;
pthread_spinlock_t shm_wr_spinlock;

size_t shm_alloc_size;            // 分配的整个映射内存大小
size_t shm_record_size;           // 每个缓冲的结构大小
size_t shm_buffer_record;         // 每个缓冲结构内可存储的结构数

void *shm_buffers[SHM_BUF_COUNT]; // 缓冲区地址数组
int shm_read_only;                // 如果不是创建此shared memory的一方，这个会置1
                                  // 这是为了保证只有一方可以写入数据

// 注意，下面这个数组指向映射内存的最后一个页
// 但不能让其存储大小超过SHM_PAGE_SIZE
shmdbuf_hdr_t *shm_buffer_hdr[SHM_BUF_COUNT];

// 读写结构，用于记录当前进程的读写状况
//  [0] -- 读指针
//  [1] -- 写指针
// 注意，由于此结构定义在这里，所以一个进程内只能有1个共享内存
shmdbuf_wr_rec_t *shm_wr_index[2];

/*****************************************************************************/
/*  Macro                                                                    */
/*****************************************************************************/
// 锁定指定内存区
#ifdef USE_POSIX_MUTEX
// POSIX互斥体，需要PTHREAD_PROCESS_SHARED，但FreeBSD没有实现
#define M_SHM_DBUF_LOCK(hdr)      ( pthread_mutex_lock(&(hdr->g_mutex)) )
#define M_SHM_DBUF_UNLOCK(hdr)    ( pthread_mutex_unlock(&(hdr->g_mutex)) )
#else
#define M_SHM_DBUF_LOCK(i,hdr)    ( set_flock(i, F_WRLCK, F_SETLKW) )
#define M_SHM_DBUF_UNLOCK(i,hdr)  ( set_flock(i, F_UNLCK, F_SETLKW) )
#endif
// 读写锁
#define M_SHM_RD_LOCK             ( pthread_spin_lock(&shm_rd_spinlock) )
#define M_SHM_RD_UNLOCK           ( pthread_spin_unlock(&shm_rd_spinlock) )
#define M_SHM_WD_LOCK             ( pthread_spin_lock(&shm_wr_spinlock) )
#define M_SHM_WD_UNLOCK           ( pthread_spin_unlock(&shm_wr_spinlock) )


/*****************************************************************************/
/*  Function                                                                 */
/*****************************************************************************/
#ifndef USE_POSIX_MUTEX
// 文件锁操作
//  F_GETLK   取得文件锁定的状态
//  F_SETLK   设置文件锁定的状态 (lock_type = F_RDLCK, F_WRLCK, F_UNLCK)
//  F_SETLKW  与F_SETLK作用相同，但是无法建立锁定时，此调用会一直等到锁定动作成功为止
// set_flock(0, F_WRLCK, F_SETLKW);
// set_flock(1, F_UNLCK, F_SETLKW);
int set_flock(int index, int lock_type, int lock_cmd)
{
  struct flock slock;
  size_t buffer_len;
  
  buffer_len = shm_record_size*shm_buffer_record;
  slock.l_type = lock_type;
  slock.l_whence = SEEK_SET;
  slock.l_start = index*buffer_len;     // 从第index个缓冲区位置开始操作锁
  slock.l_len = buffer_len;
  slock.l_pid = getpid();
  
  return fcntl(shm_fd, lock_cmd, &slock);
}
#endif

///
///  初始化共享缓冲区
///
///    input: (映射名"/xxx", 每个缓冲区的大小, 每个存储结构大小, 是否强制写模式)
///           force_write = 1 时，如果共享缓冲区已存在，则会重新连接并继续写入
///   return: 失败返回0并输出原因，成功返回1
///
/// need more help about mmap/shm_open, check
///   http://www.unix.com/programming/125156-questions-about-mmap-shm_open.html
///
INLINE_FUNC
int link_shm_doublebuffer(char *shm_name, size_t shm_buffer_size, size_t user_record_size, int force_write)
{
  int i;
  void *alloc_memory;
#ifdef USE_POSIX_MUTEX
  pthread_mutexattr_t attr;
#endif

  shm_read_only = 1;
  // try link to exist Posix shared memory
  shm_fd = shm_open(shm_name, O_RDWR, S_IRUSR|S_IWUSR);     // 注意必须是O_RDWR，因为需要操作互斥锁
  if ((shm_fd < 0) && (errno == ENOENT)) {
    // create a Posix shared memory
    shm_fd = shm_open(shm_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    shm_read_only = 0;
  }
  if (shm_fd < 0) {
    DE("shm_open failed, errormsg='%s' errno=%d\n", strerror(errno), errno);
    return 0;
  }
  
  if (shm_read_only) {
    if (force_write) {
      // 强制写模式
      DD("Link to exist shared memory, FORCE_WRITE_MOD.\n");
    }else {
      // 非创建方，只读模式开启
      DD("Link to exist shared memory, READONLY_MOD.\n");
    }
  }else {
    // O_CREAT成功，需要初始化头部的数据
    DD("Init shared memory, CREATE_MOD.\n");
  }
  
  // 计算实际需要分配的映射内存大小
  // 留出最后一个页作为存放共享头部的位置
  shm_alloc_size = (shm_buffer_size * SHM_BUF_COUNT) + SHM_PAGE_SIZE;
  if (!shm_read_only) {   // 这里不用处理 force_write，因为如果不是第一次创建，不需要ftruncate
    if (ftruncate(shm_fd, shm_alloc_size) < 0) {
      DE("ftruncate failed, file size 0x%X is too large.\n", shm_alloc_size);
      return 0;
    }
  }
  
  // 建立映射并使用 mlock 锁定
  alloc_memory = mmap(NULL, shm_alloc_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (alloc_memory == NULL) {
    DE("mmap failed, errormsg='%s' errno=%d\n", strerror(errno), errno);
    return 0;
  }
  if (mlock((void*)alloc_memory, shm_alloc_size) < 0) {
    DE("mlock failed, errormsg='%s' errno=%d\n", strerror(errno), errno);
    return 0;
  }
  
  // 初始化读写时用到的自旋锁
  pthread_spin_init(&shm_rd_spinlock, 0);
  pthread_spin_init(&shm_wr_spinlock, 0);
  
  // 计算实际存储的结构块大小(将user_record_size按四字节对其)
  if (user_record_size % 4 == 0) {
    shm_record_size = user_record_size;
  }else {
    shm_record_size = (user_record_size - (user_record_size%4) + 4);
  }
  // 计算每个结构块内的数据个数
  shm_buffer_record = shm_buffer_size / shm_record_size;
  
  // 如果是创建过程，需要初始化 shm_buffer_hdr[] 中的变量
  if (shm_read_only) {
    // 首先计算指针地址
    for (i = 0; i < SHM_BUF_COUNT; i++)
    {
      shm_buffers[i] = (void*)((size_t)alloc_memory + (i*shm_buffer_size));
      shm_buffer_hdr[i] = (void*)((size_t)alloc_memory + (SHM_BUF_COUNT*shm_buffer_size) + i*sizeof(shmdbuf_hdr_t));
      // 检查链接到的共享内存中record大小是否和当前指定一致
      if (shm_buffer_hdr[i]->record_size != shm_record_size) {
        shm_output("[WARRING] set shared buffer[%d] record size=%d, but linked shm_record_size=%d\n", i, shm_record_size, shm_buffer_hdr[i]->record_size);
      }
    }
  }else {
#ifdef USE_POSIX_MUTEX
    // 第一次创建共享内存，需要初始化互斥体
    // 由于需要进程间互斥，设置attr为PTHREAD_PROCESS_SHARED
    pthread_mutexattr_init(&attr);
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
      DE("pthread_mutexattr_setpshared failed, errormsg='%s' errno=%d\n", strerror(errno), errno);
      return 0;
    }
#endif
    
    // 初始化 shm_buffers[] 和 shm_buffer_hdr
    for (i = 0; i < SHM_BUF_COUNT; i++)
    {
      shm_buffers[i] = (void*)((size_t)alloc_memory + (i*shm_buffer_size));
      shm_buffer_hdr[i] = (void*)((size_t)alloc_memory + (SHM_BUF_COUNT*shm_buffer_size) + i*sizeof(shmdbuf_hdr_t));
#ifdef USE_POSIX_MUTEX
      pthread_mutex_init(&(shm_buffer_hdr[i]->g_mutex), &attr);   // 初始化互斥体
#endif
      shm_buffer_hdr[i]->wr_sign = 0;
      shm_buffer_hdr[i]->record_size = shm_record_size;
    }
    
    // 写入模式开始时，将要写入的块加锁
    lock_shm_doublebuffer(0);
    shm_buffer_hdr[0]->wr_sign = 1;
  }
  
  // 初始化读写位置
  shm_wr_index[0] = (shmdbuf_wr_rec_t*) calloc(1, sizeof(shmdbuf_wr_rec_t));
  shm_wr_index[1] = (shmdbuf_wr_rec_t*) calloc(1, sizeof(shmdbuf_wr_rec_t));
  
  // 如果是强制写模式，并且已存在共享内存，连接到已存在映射并重新找回可写位置
  if ((shm_read_only) && (force_write)) {
    // 检查写入标记，如果为1说明找到该块
    for (i = 0; i < SHM_BUF_COUNT; i++)
      if (shm_buffer_hdr[i]->wr_sign) break;
    shm_output("locate blocked shared memory at [%d], restart write process...\n", i);
    
    // 重置写入位置到该块的开头
    shm_wr_index[1]->dbuf_index = i;
    shm_wr_index[1]->wr_index = 0;
    
    shm_read_only = 0;
  }
  
#ifdef _IA64
  shm_output("[%d] mmap buffer at 0x%.16lX, size=0x%.8X, shm_hdr start at 0x%.16lX, record_size=%d(%d), record_count=0x%X(x%d)\n", shm_read_only,
#else
  shm_output("[%d] mmap buffer at 0x%.8X, size=0x%.8X, shm_hdr start at 0x%.8X, record_size=%d(%d), record_count=0x%X(x%d)\n", shm_read_only,
#endif
      (size_t)alloc_memory, shm_alloc_size, (size_t)shm_buffer_hdr[0], shm_record_size, user_record_size, shm_buffer_record, SHM_BUF_COUNT);
  
  return 1;
}

///
///
///  删除映射的内存
///
///   注意：对于非创建者(shm_read_only)，无法直接删除这个映射
///
INLINE_FUNC
void free_shm_doublebuffer(char *shm_name)
{
  munlock(shm_buffers[0], shm_alloc_size);
  munmap(shm_buffers[0], shm_alloc_size);
  if (!shm_read_only) {
    // 只有创建者可以删除 shared memory
    if (shm_unlink(shm_name) == -1)
      DE("shm_unlink failed, errormsg='%s' errno=%d\n", strerror(errno), errno);
  }
  if (shm_wr_index[0] != NULL) free(shm_wr_index[0]);
  if (shm_wr_index[1] != NULL) free(shm_wr_index[1]);
}
// 强制删除一个共享内存映射 (此函数用于在创建进程异常终止时删除shm映射)
INLINE_FUNC
int destory_shm_memory(char *shm_name)
{
  return shm_unlink(shm_name);
}

///
///  锁定/解锁指定内存块
///
INLINE_FUNC
void lock_shm_doublebuffer(int index)
{
#ifdef USE_POSIX_MUTEX
  M_SHM_DBUF_LOCK(shm_buffer_hdr[index]);
#else
  M_SHM_DBUF_LOCK(index, shm_buffer_hdr[index]);
#endif
}
INLINE_FUNC
void unlock_shm_doublebuffer(int index)
{
#ifdef USE_POSIX_MUTEX
  M_SHM_DBUF_UNLOCK(shm_buffer_hdr[index]);
#else
  M_SHM_DBUF_UNLOCK(index, shm_buffer_hdr[index]);
#endif
}

///
///  结构块普通读写操作
///

// 切换当前块到另一个块(注意，多线程时外部需要锁操作)
INLINE_FUNC
void shm_wr_changebuffer(shmdbuf_wr_rec_t *wr_index)
{
  size_t next_index;
  
  next_index = wr_index->dbuf_index + 1;
  if (next_index == SHM_BUF_COUNT) next_index = 0;
  wr_index->dbuf_index = next_index;
  if ((shm_wr_index[0]->wr_index != 0) && (shm_wr_index[0]->wr_index < shm_buffer_record)) {
    // 这里计算丢包数
  }
  wr_index->wr_index = 0;
}

// 从其中读取一个数据块
INLINE_FUNC
void read_shm_doublebuffer(void *user_buffer)
{
  size_t cur_dbuf_index;
  size_t read_address;
  
  M_SHM_RD_LOCK;
  
  cur_dbuf_index = shm_wr_index[0]->dbuf_index;
  
  // 首先检查当前块的写入标记 wr_sign 是否为0 (可读状态)
  // 若为1，表示写入方已经准备写入数据了，此时应该放弃当前块的数据，转到下一个块开始读取
  if (shm_buffer_hdr[cur_dbuf_index]->wr_sign) {
    if (shm_wr_index[0]->wr_index != 0) {
      // 如果不是第一次读取，指向下一个数据块
      shm_wr_changebuffer(shm_wr_index[0]);
      cur_dbuf_index = shm_wr_index[0]->dbuf_index;   // 更新块编号
    }
  }
  // 如果在块首第一次读取，首先尝试对块进行lock操作，以检查是否已经写入完毕
  if (shm_wr_index[0]->wr_index == 0) {
    // 如果没有写入完毕，将阻塞在此处
    lock_shm_doublebuffer(cur_dbuf_index);
    // 得到了这个锁，说明目标块已经准备好，立即释放锁并开始块内读取
    unlock_shm_doublebuffer(cur_dbuf_index);
  }
  
  // 从 shm_wr_index[0]->wr_index 处取出 shm_record_size 长度的数据到 user_buffer
  read_address = (size_t)shm_buffers[cur_dbuf_index] + (shm_wr_index[0]->wr_index * shm_record_size);
#ifdef _IA64
  DD("read buffer at %.16lX, start=%.16lX, index=%d, size=%d\n", read_address, (size_t)shm_buffers[cur_dbuf_index], shm_wr_index[0]->wr_index, shm_record_size);
#else
  DD("read buffer at %.8X, start=%.8X, index=%d, size=%d\n", read_address, (size_t)shm_buffers[cur_dbuf_index], shm_wr_index[0]->wr_index, shm_record_size);
#endif
  
  // wr_index+1
  shm_wr_index[0]->wr_index++;
  if (shm_wr_index[0]->wr_index >= shm_buffer_record) {
    // 整个块数据读取完毕，此时切换读取的块
    shm_wr_changebuffer(shm_wr_index[0]);
  }
  
  M_SHM_RD_UNLOCK;
  
  memcpy(user_buffer, (void*)read_address, shm_record_size);
  return;
}

// 向其中写入指定数据，不足 shm_record_size 的将被填充00
INLINE_FUNC
void write_shm_doublebuffer(void *user_buffer, int user_buf_size)
{
  size_t cur_dbuf_index;
  size_t write_address;
  size_t data_size;
  
  M_SHM_WD_LOCK;
  
  cur_dbuf_index = shm_wr_index[1]->dbuf_index;
  
  // 计算写入地址和需要memcpy数据的长度
  write_address = (size_t)shm_buffers[cur_dbuf_index] + (shm_wr_index[1]->wr_index * shm_record_size);
  if ((user_buf_size > 0) && (user_buf_size < shm_record_size)) {
    data_size = user_buf_size;
  }else {
    data_size = shm_record_size;
  }
#ifdef _IA64
  DD("write buffer at %.16lX, start=%.16lX, index=%d, data_size=%d\n", write_address, (size_t)shm_buffers[cur_dbuf_index], shm_wr_index[1]->wr_index, data_size);
#else
  DD("write buffer at %.8X, start=%.8X, index=%d, data_size=%d\n", write_address, (size_t)shm_buffers[cur_dbuf_index], shm_wr_index[1]->wr_index, data_size);
#endif
  
  // wr_index+1
  shm_wr_index[1]->wr_index++;
  if (shm_wr_index[1]->wr_index >= shm_buffer_record) {
    // 整个块写入完毕，此时释放当前块供读取并切换到下一块
    shm_buffer_hdr[cur_dbuf_index]->wr_sign = 0;
    unlock_shm_doublebuffer(cur_dbuf_index);
    shm_wr_changebuffer(shm_wr_index[1]);
    // 对下一个块加锁
    cur_dbuf_index = shm_wr_index[1]->dbuf_index;
    lock_shm_doublebuffer(cur_dbuf_index);
    shm_buffer_hdr[cur_dbuf_index]->wr_sign = 1;
  }
  
  M_SHM_WD_UNLOCK;
  
  // memcpy数据到指定位置
  // 这里不用考虑读写互斥问题，由于缓冲区够大，写入的数据位置在短时间内不会被读取到
  memcpy((void*)write_address, user_buffer, data_size);
  if (data_size != shm_record_size) {
    // 用0填充剩余的字节
    memset( (void*)(write_address+data_size), 0, (shm_record_size-data_size) );
  }
  return;
}

///
///  高级写入函数，返回指针给用户，由用户自己向其中写入内容
///
// 此函数返回一个可供用户写入的指针，用户可以自行向其中填充不超过shm_record_size的数据
INLINE_FUNC
void* get_shm_write_ptr(void)
{
  size_t cur_dbuf_index;
  size_t write_address;
  
  M_SHM_WD_LOCK;
  
  cur_dbuf_index = shm_wr_index[1]->dbuf_index;
  
  write_address = (size_t)shm_buffers[cur_dbuf_index] + (shm_wr_index[1]->wr_index * shm_record_size);
#ifdef _IA64
  DD("write buffer at %.16lX, start=%.16lX, index=%d\n", write_address, (size_t)shm_buffers[cur_dbuf_index], shm_wr_index[1]->wr_index);
#else
  DD("write buffer at %.8X, start=%.8X, index=%d\n", write_address, (size_t)shm_buffers[cur_dbuf_index], shm_wr_index[1]->wr_index);
#endif
  
  shm_wr_index[1]->wr_index++;
  if (shm_wr_index[1]->wr_index >= shm_buffer_record) {
    // 整个块写入完毕，此时释放当前块供读取并切换到下一块
    shm_buffer_hdr[cur_dbuf_index]->wr_sign = 0;
    unlock_shm_doublebuffer(cur_dbuf_index);
    shm_wr_changebuffer(shm_wr_index[1]);
    // 对下一个块加锁
    cur_dbuf_index = shm_wr_index[1]->dbuf_index;
    lock_shm_doublebuffer(cur_dbuf_index);
    shm_buffer_hdr[cur_dbuf_index]->wr_sign = 1;
  }
  
  M_SHM_WD_UNLOCK;
  
  return (void*)write_address;
}

///
///  状态函数，查询缓冲区的状态
///
// 是否为只读模式运行，是则返回1
INLINE_FUNC
int is_shm_readonly_mode(void)
{
  return shm_read_only;
}
// 取得读写指针的状态
INLINE_FUNC
void state_shm_doublebuffer(shmdbuf_wr_rec_t *stat_record)
{
  if (shm_read_only) {
    *stat_record = *shm_wr_index[0];
  }else {
    *stat_record = *shm_wr_index[1];
  }
  return;
}
