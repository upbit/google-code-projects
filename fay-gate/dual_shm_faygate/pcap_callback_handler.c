#ifndef PCAP_CALLBACK_HANDLE_C
#define PCAP_CALLBACK_HANDLE_C
#include "fay_original.h"
#include <pcap.h>

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
DWORD packet_count_rx = 0;
DWORD packet_count_tx = 0;
DWORD packet_bytes_rx = 0;              // 实际字节数/16
DWORD packet_bytes_tx = 0;              // 实际字节数/16

//////////////////////////////////////

INLINE_FUNC
int cache_packet_to_dbuf(struct pcap_pkthdr * pkthdr, u_char *pkt_buf, int direction)
{
  ring_pkg_hdr_t *ring_pkg;
  BYTE pro_type;
  int buf_len, data_offset;
  
  // 首先检查是否为 IP 包，不是则退出
  if ( *(WORD*)(&pkt_buf[12]) != 0x0008 ) return 0;
  
  ring_pkg = (ring_pkg_hdr_t*) SHM_DBUF_GET_WDPTR;
  ring_pkg->hash_index = 0;
  
  // 写入方向标记
  ring_pkg->flags = 0;
  switch (direction) {
    case TP_FLAG_TX_PKG:    // TP_FLAG_TX_PKG = 0, CLR
      M_FLAG_CLR(ring_pkg->flags, ID_FLAG_TransDirection);
      break;
    case TP_FLAG_RX_PKG:    // TP_FLAG_RX_PKG = 1, SET
      M_FLAG_SET(ring_pkg->flags, ID_FLAG_TransDirection);
      break;
    default:
      M_FLAG_SET(ring_pkg->flags, ID_FLAG_IgnoreDirection);
      break;
  };
  
  //
  pro_type = (BYTE)pkt_buf[23];
  switch (pro_type) {
    case _Protocol_TCP:
      M_FLAG_SET_Protocol(ring_pkg->flags, TP_FLAG_Protoctl_TCP);
      // 计算TCP的负载偏移 (14 + IP头长度 + TCP偏移)
      data_offset = 14 + (((int)pkt_buf[14]&0x0F)<<2) + (((int)pkt_buf[46]&0xF0)>>2);
      break;
    case _Protocol_UDP:
      M_FLAG_SET_Protocol(ring_pkg->flags, TP_FLAG_Protoctl_UDP);
      data_offset = 14 + (((int)pkt_buf[14]&0x0F)<<2) + 8;
      break;
    default:
      // 其他协议类型或数据包类型，不处理
      return 0;
  };
  
  // 记录原始长度和缓冲长度
  ring_pkg->length = pkthdr->len;
  buf_len = pkthdr->len - data_offset;
  if (buf_len <= 0) return 0;           // 防止出错，去掉负载为0的包
  if (buf_len > RP_DATA_SIZE) buf_len = RP_DATA_SIZE;
  
  // 记录数据包相关的信息
  *(DWORD*)(&ring_pkg->src_ip[0]) = *(DWORD*)(&pkt_buf[26]);
  *(DWORD*)(&ring_pkg->dest_ip[0]) = *(DWORD*)(&pkt_buf[30]);
  *(WORD*)(&ring_pkg->src_port) = *(WORD*)(&pkt_buf[34]);
  *(WORD*)(&ring_pkg->dest_port) = *(WORD*)(&pkt_buf[36]);
  
  ring_pkg->time_stamp = pkthdr->ts.tv_sec;
  //DD("%d packet, %.8X->%.8X, len=%d, off=%d, buf_len=%d\n", M_FLAG_GET_Protoctl(ring_pkg->flags), (DWORD)&ring_pkg->src_ip[0], (DWORD)&ring_pkg->dest_ip[0], pkthdr->len, data_offset, buf_len);
  
  memcpy((void*)&ring_pkg->data[0], (void*)&pkt_buf[data_offset], buf_len);
  return 1;
}

/*****************************************************************************/
/* Functions - single interface capture                                      */
/*****************************************************************************/
void
process_ethernet_packet(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt)
{
  if (cache_packet_to_dbuf(pkthdr, pkt, -1))
  {
    // 仅仅累计包数和流量(只有一个回调时，默认使用RX的计数器)
    packet_count_rx++;
    packet_bytes_rx += pkthdr->len / 16;
    if (packet_bytes_rx > 0xFFFFFF00) packet_bytes_rx -= 0xFFFFFF00;
  }
}

/*****************************************************************************/
/* Functions - dual interface capture                                        */
/*****************************************************************************/
void
process_ethernet_packet_rx(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt)
{
  if (cache_packet_to_dbuf(pkthdr, pkt, TP_FLAG_RX_PKG))
  {
    packet_count_rx++;
    packet_bytes_rx += pkthdr->len / 16;
    if (packet_bytes_rx > 0xFFFFFF00) packet_bytes_rx -= 0xFFFFFF00;
  }
}

void
process_ethernet_packet_tx(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt)
{
  if (cache_packet_to_dbuf(pkthdr, pkt, TP_FLAG_TX_PKG))
  {
    packet_count_tx++;
    packet_bytes_tx += pkthdr->len / 16;
    if (packet_bytes_tx > 0xFFFFFF00) packet_bytes_tx -= 0xFFFFFF00;
  }
}

#endif
