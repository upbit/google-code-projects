#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <shmdbuf.h>

#define SHMBUF_NAME   "/dbuf"
//#define SHMBUF_SIZE   64*0x100000
#define SHMBUF_SIZE   1024
#define RECORD_SIZE   128

typedef u_int8_t BYTE;
typedef u_int16_t WORD;
typedef u_int32_t DWORD;
typedef u_int64_t UINT64;

DWORD process_count = 0;
DWORD running = 0;

void call_process_packet(BYTE *pkg_buf)
{
  process_count++;
  /*fprintf(stdout, "[%14d]  WD=%14d / RD=%14d  | %.2X-%.2X-%.2X-%.2X-%.2X-%.2X  ->  %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
                        process_count, ring_hdr->wd_index, ring_hdr->rd_index,
                        pkg_buf[0], pkg_buf[1], pkg_buf[2], pkg_buf[3], pkg_buf[4], pkg_buf[5],
                        pkg_buf[6], pkg_buf[7], pkg_buf[8], pkg_buf[9], pkg_buf[10], pkg_buf[11]);*/
}

void write_data_thread(void)
{
  char local_buffer2[RECORD_SIZE];
  
  while (running) {
    call_process_packet(&local_buffer2[0]);
    SHM_DBUF_WRITE(&local_buffer2[0], -1);
    sleep(3);
  }
  // detach，释放线程资源
  pthread_detach(pthread_self());
}

void sig_handler(int sig)
{
  shmdbuf_wr_rec_t stat_record;
  
  if (SIGINT == sig) {
    // 处理 Ctrl + C 的操作
    static int called = 0;
    if(called) return; else called = 1;
    
    running = 0;
    usleep(500000);
    
    SHM_DBUF_FREE(SHMBUF_NAME);
    exit(0);
  }else if (SIGALRM == sig) {
    SHM_DBUF_STAT(&stat_record);
    fprintf(stdout, "%14u  |  DBUF[%d]=%12d\n", process_count, stat_record.dbuf_index, stat_record.wr_index);
    fflush(stdout);
    process_count = 0;
  }
}

int main(int argc, char *argv[])
{
  int fd;
  char local_buffer[RECORD_SIZE];
  
  if (argc == 2) {
    if (!destory_shm_memory(argv[1])) {
      printf("destory_shm_memory() '%s' ok!\n", argv[1]);
    }else {
      printf("error destory_shm_memory() '%s', try later.\n", argv[1]);
    }
    return 0;
  }
  
  if ( SHM_DBUF_LINK(SHMBUF_NAME, SHMBUF_SIZE, RECORD_SIZE) ) {
    
    sleep(2);
    
    // register Ctrl+C event handling callback
    signal(SIGINT, sig_handler);
    signal(SIGALRM, sig_handler);
    
    struct itimerval itv, oldtv;
    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, &oldtv);
    
    running = 1;
    if ( SHM_IS_RDONLY_MODE ) {
      // 读取进程
      while (running) {
        SHM_DBUF_READ(&local_buffer[0]);
        call_process_packet(&local_buffer[0]);
        usleep(500000);
      }
    }else {
      // 写入进程
      pthread_t write_thread;
      if ( pthread_create(&write_thread, NULL, (void *(*)(void *))&write_data_thread, NULL) ) {
        perror("pthread_create");
        return -1;
      }
      
      while (running) {
        call_process_packet(&local_buffer[0]);
        SHM_DBUF_WRITE(&local_buffer[0], -1);
        sleep(3);
      }
    }
    
  }
  
  return 0;
}
