﻿#include "fay_original.h"
#include "pcap_common.c"

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/
void tx_loop_thread(void)
{
  pcap_handler callback_tx;
  
  callback_tx = (pcap_handler)process_ethernet_packet_tx;
  pcap_loop(fayrc->live_handle2, -1, callback_tx, (u_char*)"TX");
  
  // detach，释放线程资源
  pthread_detach(pthread_self());
  return;
}

int open_pcap_livecapture(char *dev_name1, char *dev_name2)
{
  pcap_handler callback;
  int dual_capture;
  
  // 检查抓包参数，是否是多网卡
  dual_capture = IS_DUAL_CAP(fayrc);
  if (dual_capture) {
    // 依次打开两个网卡并设置过滤器 PCAP_CAP_FILTER
    if (pcap_filter_openlive(&fayrc->live_handle1, dev_name1, PCAP_CAP_FILTER) != FE_SUCCESS) {
      fprintf(stderr, "Unable to open the adapter, \"%s\" is not supported by libpcap, or permission denied.\n", dev_name1);
      return FE_ERROR;
    }
    if (pcap_filter_openlive(&fayrc->live_handle2, dev_name2, PCAP_CAP_FILTER) != FE_SUCCESS) {
      fprintf(stderr, "Unable to open the adapter, \"%s\" is not supported by libpcap, or permission denied.\n", dev_name2);
      return FE_ERROR;
    }
  }else {
    // 如果是单网卡模式，只打开1的
    if (pcap_filter_openlive(&fayrc->live_handle1, dev_name1, PCAP_CAP_FILTER) != FE_SUCCESS) {
      fprintf(stderr, "Unable to open the adapter, \"%s\" is not supported by libpcap, or permission denied.\n", dev_name1);
      return FE_ERROR;
    }
  }
  
  DE("- fay_original START -\n");
  
  /* 初始化 */
  
  // 设置回调函数并显示抓包的网卡
  if (dual_capture) {
    fay_debug("start monitor on %s/RX, %s/TX\n", dev_name1, dev_name2);
    // 因为 pcap_loop() 不会返回，需要用线程注册回调
    pthread_t handle_loop_thread;
    if ( pthread_create(&handle_loop_thread, NULL, (void *(*)(void *))&tx_loop_thread, NULL) ) {
      perror("pthread_create(tx_loop_thread) error!");
      return -1;
    }
    
    // 由于RX流量通常较大，放到主线程中注册回调
    callback = (pcap_handler)process_ethernet_packet_rx;
    pcap_loop(fayrc->live_handle1, -1, callback, (u_char*)"RX");
  }else {
    fay_debug("start monitor on %s\n", fayrc->live_name1);
    callback = (pcap_handler)process_ethernet_packet;
    pcap_loop(fayrc->live_handle1, -1, callback, (u_char*)"BOTH");
  }
  
  return 0;
}
 