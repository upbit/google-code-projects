#ifndef FAY_ORIGINAL_H
#define FAY_ORIGINAL_H
/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
/* C Standard Library headers. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* POSIX headers. */
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* user headers */
#include <pcap.h>
#include "fay_error.h"
#include <shmdbuf.h>          // download from http://libshmdbuf.googlecode.com/

/*****************************************************************************/
/* Debug options                                                             */
/*****************************************************************************/
#ifndef NO_DEBUG
// 调试信息开关
#define __DEBUG_RuntimeCount__                        // 显示运行耗时

#endif

/*****************************************************************************/
/* Type define                                                               */
/*****************************************************************************/
#ifdef NO_MEMCHK
#define __malloc malloc
#define __calloc calloc
#define __free free
#else
#include <memchk.h>
#endif

#define INLINE_FUNC               inline

typedef u_int8_t BYTE;
typedef u_int16_t WORD;
typedef u_int32_t DWORD;
typedef u_int64_t UINT64;

/*****************************************************************************/
/* Macros and constants                                                      */
/*****************************************************************************/
#define PCAP_CAPTURE_PACKET_LEN   300
#define PCAP_CAP_FILTER           "tcp or udp"        // libpcap过滤器

#define INIT_OPENLIVE             0x00000001          // 实时抓包分析
#define INIT_OPENLIVE_DUAL        0x00000002          // 实时抓包分析-双网卡
#define INIT_OFFLINE              0x00000004          // 分析离线文件
#define INIT_ALL_RESULT           0x00000010          // 输出完整的调试信息
#define INIT_ANALYSIS_MASK        (INIT_OPENLIVE|INIT_OPENLIVE_DUAL|INIT_OFFLINE)

#define TRAFFIC_TX                0                   // 传输方向 TX
#define TRAFFIC_RX                1                   // 传输方向 RX
#define MAX_RSYN_COUNT            10                  // 记录的最大SYN/ACK周期数


/* Debug output macros */
#define fay_debug(fmt, ...)	      fprintf(stderr, fmt, ## __VA_ARGS__)
// if set -DNO_DEBUG, disable DD/DE output
#ifdef NO_DEBUG
#define DD(fmt, ...)
#define DE(fmt, ...)
#else
#define DD(fmt, ...)	            fprintf(stdout, "[DEBUG] " fmt, ## __VA_ARGS__)
#define DE(fmt, ...)	            fprintf(stderr, "" fmt, ## __VA_ARGS__)
#endif

/*****************************************************************************/
/* Data structures                                                           */
/*****************************************************************************/
#pragma pack(1)                             // 强制1字节对其

// 描述结构，记录了初始化相关的一些信息和一些全局变量的内容
typedef struct fay_rc {
  DWORD init_flags;
/* NOTICE! device name size=16 is too small for windows, if you compile 
       this project under Cygwin/MinGW, you must change 16 to 256 or larger. */
  char live_name1[16];
  char live_name2[16];
  pcap_t *live_handle1;
  pcap_t *live_handle2;
  
} fay_rc_t;
// 宏定义，检查是否是双网卡抓包
#define IS_DUAL_CAP(rc)           (INIT_OPENLIVE_DUAL==(rc->init_flags&INIT_OPENLIVE_DUAL))

// 缓冲环描述头部
typedef struct _ring_hdr {
  DWORD rd_index;
  DWORD wd_index;
  DWORD zero_fill[2];
} ring_hdr_t;

// 缓冲的数据包格式(固定128/256字节)
#define RP_DATA_SIZE              (128-24)
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

// IP哈希表存储结构，每个内网IP对应一个此结构，用于记录IP的分析状态和基本信息
typedef struct {
  BYTE ip[4];
  
  DWORD stream_count;                       // [可选]节点流总数(累计)
  WORD conc_conns;                          // 当前并发连接数 (Concurrent Connections)
                                            // (注意不能超过65536，TCP和UDP分开计算)
  
  WORD syn[MAX_RSYN_COUNT], ack[MAX_RSYN_COUNT];// 节点 SYN/ACK 统计信息
  BYTE period_index;                        // 当前周期序号(循环)
  DWORD period_time;                        // 上次更新周期的时间
  
  BYTE status;                              // IP分析状态(L1, L2, L3, SKIP, ...)
  
  double krc, kep, bsi;
} fay_ip_info_t;

// 流哈希表存储，分别对应三个算法所需要的信息
typedef struct {
  BYTE src_ip[4], dest_ip[4];
  WORD src_port, dest_port;
  DWORD packets[2];                         // TRAFFIC_TX 和 TRAFFIC_RX 方向的数据包数
  DWORD datas[2];                           // 传输字节数 ( /16 - 64G MAX ) ( /8 - 32G MAX )
  
  BYTE flags;                               // .... .XXX        Protocol          流类型 1-TCP/2-UDP/...
                                            // X... ....        Connected         是否已经建立连接
} fay_stream_info_rc_t;

typedef struct {
  //Subnet
} fay_stream_info_ep_t;

typedef struct {
  //BSI
} fay_stream_info_bsi_t;


/*****************************************************************************/
/* Macro define                                                              */
/*****************************************************************************/

#define _Protocol_TCP             6           // Transmission Control Protocol
#define _Protocol_UDP             17          // UDP

// 协议类型
#define TP_FLAG_Protoctl_NULL     0           // 000
#define TP_FLAG_Protoctl_TCP      1           // 001  TCP协议
#define TP_FLAG_Protoctl_UDP      2           // 010  UDP协议

// 方向标志定义
#define TP_FLAG_TX_PKG            0           // TX 方向的数据包
#define TP_FLAG_RX_PKG            1

// id字段取值
#define ID_FLAG_TransDirection    3           // 数据包传输方向
#define ID_FLAG_IgnoreDirection   8           // 忽略3位上的方向标志

// 协议段(0-2)
#define M_FLAG_GET_Protoctl(flag) (flag & 0x7)
#define M_FLAG_SET_Protocol(flag, value) (flag = ((flag & ~0x7)|(value & 0x7)))
// 其他 1 bit 字段
#define M_FLAG_GET(flag, id)      (flag & (1 << id))
#define M_FLAG_SET(flag, id)      (flag |= (1 << id))
#define M_FLAG_CLR(flag, id)      (flag &= ~(1 << id))


/*****************************************************************************/
/* Extern variables and functions                                            */
/*****************************************************************************/
extern fay_rc_t *fayrc;

extern DWORD packet_count_rx;
extern DWORD packet_count_tx;
extern DWORD packet_bytes_rx;
extern DWORD packet_bytes_tx;

extern void *ring_buffer;
extern ring_hdr_t *ring_hdr;

INLINE_FUNC void init_fayoriginal_resource();
INLINE_FUNC void free_fayoriginal_resource();
INLINE_FUNC void register_handle_signal(int alarm_wait);

INLINE_FUNC int open_pcap_offline(char *file_name);
INLINE_FUNC int open_pcap_livecapture(char *dev_name1, char *dev_name2);

INLINE_FUNC void process_ethernet_packet(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt);
INLINE_FUNC void process_ethernet_packet_rx(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt);
INLINE_FUNC void process_ethernet_packet_tx(char *user, struct pcap_pkthdr * pkthdr, u_char * pkt);

#endif
