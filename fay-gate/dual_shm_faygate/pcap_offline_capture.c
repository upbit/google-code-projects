﻿#include "fay_original.h"
#include "pcap_common.c"

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/
// 打开离线文件并进行分析
int open_pcap_offline(char *file_name)
{
	pcap_t *offline_handle;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_handler callback;
	
	// 打开离线文件
	if ((offline_handle = pcap_open_offline(file_name, errbuf)) == NULL) {
		fay_debug("pcap_open_offline('%s') error: %s\n", file_name, errbuf);
		return FE_INVAL;
	}
	// 设置过滤器
	if ( set_pcap_filter(offline_handle, PCAP_CAP_FILTER) != FE_SUCCESS ) {
	  fay_debug("set_pcap_filter(\"%s\") error: wrong syntax.\n", PCAP_CAP_FILTER);
	  return FE_INVAL;
	}
	
	/* 初始化 */
	
	callback = (pcap_handler)process_ethernet_packet;
	
	DD("load dump \"%s\" ...\n", file_name);
	
  pcap_offline_read(offline_handle, -1, callback, NULL);
  pcap_close(offline_handle);
  
  return FE_SUCCESS;
}
 