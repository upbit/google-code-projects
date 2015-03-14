#ifndef FAY_SIGNAL_C
#define FAY_SIGNAL_C
#include "fay_original.h"
#include <signal.h>

int alarm_sec = 0;

// 消息处理
void sig_handler(int sig)
{
  shmdbuf_wr_rec_t stat_record;
  
  switch (sig) {
    case SIGINT:
      // 处理 Ctrl + C 的操作
      DE("- fay_original STOP -\n");
      
      // break live capture loop
      pcap_breakloop(fayrc->live_handle1);
      if ( IS_DUAL_CAP(fayrc) ) pcap_breakloop(fayrc->live_handle2);
      usleep(100000);
      
      // close pcap handle
      pcap_close(fayrc->live_handle1);
      if ( IS_DUAL_CAP(fayrc) ) pcap_close(fayrc->live_handle2);
      usleep(100000);
      
      // 调用释放函数，释放占用的内存资源
      free_fayoriginal_resource();
      exit(0);
      
    case SIGALRM:   // 处理 ALRAM
      SHM_DBUF_STAT(&stat_record);
      fprintf(stdout, "%14u / %14u >>> %8uMb / %8uMb  |  DBUF[%d]=%12d\n",
          packet_count_tx, packet_count_rx, packet_bytes_tx*16/125000, packet_bytes_rx*16/125000, stat_record.dbuf_index, stat_record.wr_index);
      fflush(stdout);
      break;
    default:
      break;
  }
}

// register signal process handler
void register_handle_signal(int alarm_wait)
{
  struct itimerval itv, oldtv;
  
  // register Ctrl+C event handling callback
  signal(SIGINT, sig_handler);
  // SIGALRM
  signal(SIGALRM, sig_handler);
  
  alarm_sec = alarm_wait;
  itv.it_interval.tv_sec = alarm_sec;
  itv.it_interval.tv_usec = 0;
  itv.it_value.tv_sec = alarm_sec;
  itv.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &itv, &oldtv);
}
#endif
