#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include <shmdbuf.h>

typedef u_int8_t BYTE;
typedef u_int16_t WORD;
typedef u_int32_t DWORD;
typedef u_int64_t UINT64;

#define SHMBUF_NAME         "/p2p_ringbuffer"
#define SHMBUF_SIZE         64*0x100000     // 64M
#define RECORD_SIZE         (sizeof(ring_pkg_hdr_t))

DWORD process_count;

// 缓冲的数据包格式(固定128字节)
#define RP_DATA_SIZE      (128-24)
typedef struct _ring_pkg_hdr {
  DWORD hash_index;
  
  /* +4 */
  WORD length;                              // 数据包原始长度
  BYTE buf_length;                          // 数据包缓冲长度(因为小于256，所以以1字节表示)
  BYTE flags;                               // .... .XXX        Protocol          包类型 1-TCP/2-UDP/...
                                            // .... X...        Direction         包的传输方向 0-TX/1-RX
                                            // X... ....        Ignore Direction  1表示忽略方向标记(说明这是Both的流量)
  /* +8 */
  BYTE src_ip[4], dest_ip[4];
  WORD src_port, dest_port;
  
  /* +20 */
  DWORD time_stamp;                         // 数据包的时间戳
  
  BYTE data[RP_DATA_SIZE];
} ring_pkg_hdr_t;


void call_process_packet(BYTE *pkg_buf)
{
  process_count++;
  /*fprintf(stdout, "[%14d]  WD=%14d / RD=%14d  | %.2X-%.2X-%.2X-%.2X-%.2X-%.2X  ->  %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
                        process_count, ring_hdr->wd_index, ring_hdr->rd_index,
                        pkg_buf[0], pkg_buf[1], pkg_buf[2], pkg_buf[3], pkg_buf[4], pkg_buf[5],
                        pkg_buf[6], pkg_buf[7], pkg_buf[8], pkg_buf[9], pkg_buf[10], pkg_buf[11]);*/
}

void sig_handler(int sig)
{
  shmdbuf_wr_rec_t stat_record;
  
  if (SIGINT == sig) {
    // 处理 Ctrl + C 的操作
    static int called = 0;
    if(called) return; else called = 1;
    
    SHM_DBUF_FREE(SHMBUF_NAME);
    exit(0);
  }else if (SIGALRM == sig) {
    SHM_DBUF_STAT(&stat_record);
    
    //pkg_buf = (BYTE*)((DWORD)ring_buffer + ring_hdr->rd_index*RECORD_SIZE);
    //ring_hdr->rd_index = (ring_hdr->rd_index+100) & INDEX_MASK;
    /*fprintf(stdout, "WD=%14d / RD=%14d  | %.2X-%.2X-%.2X-%.2X-%.2X-%.2X  ->  %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", ring_hdr->wd_index, ring_hdr->rd_index,
                        pkg_buf[0], pkg_buf[1], pkg_buf[2], pkg_buf[3], pkg_buf[4], pkg_buf[5],
                        pkg_buf[6], pkg_buf[7], pkg_buf[8], pkg_buf[9], pkg_buf[10], pkg_buf[11]);*/
    fprintf(stdout, "DBUF[%d]=%12d, %14d\n", stat_record.dbuf_index, stat_record.wr_index, process_count);
    process_count = 0;
  }
}

int main(int argc, char *argv[])
{
  int fd;
  BYTE pkg_buf[RECORD_SIZE];
  
  SHM_DBUF_LINK(SHMBUF_NAME, SHMBUF_SIZE, RECORD_SIZE);
  
  // register Ctrl+C event handling callback
  signal(SIGINT, sig_handler);
  signal(SIGALRM, sig_handler);
  
  struct itimerval itv, oldtv;
  itv.it_interval.tv_sec = 1;
  itv.it_interval.tv_usec = 0;
  itv.it_value.tv_sec = 1;
  itv.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &itv, &oldtv);
  
  while (1) {
    SHM_DBUF_READ(&pkg_buf[0]);
    call_process_packet(pkg_buf);
    //usleep(10);
  }
  
  return 0;
}
