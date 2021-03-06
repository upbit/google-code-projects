﻿#ifndef PCAP_COMMON_C
#define PCAP_COMMON_C
#include <pcap.h>
#include "fay_error.h"

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/
// set pcap filter for (pcap_t)
static inline
int set_pcap_filter(pcap_t* fpcap, char *filter)
{
  struct bpf_program fcode;
  bpf_u_int32 compile_net_mask = 0xffffff;
  
  if ((fpcap != NULL) && (filter != NULL))
  {
    // compile the filter
    if(pcap_compile(fpcap, &fcode, filter, 1, compile_net_mask) < 0)
    {
      //fprintf(stderr, "Error compiling filter: wrong syntax.\n");
      return FE_INVAL;
    }

    // set the filter
    if(pcap_setfilter(fpcap, &fcode)<0)
    {
      //fprintf(stderr, "Error setting the filter\n");
      return FE_PERM;
    }
    return FE_SUCCESS;
  }
  return FE_ERROR;
}

static inline
int pcap_filter_openlive(pcap_t **live_handle, char *live_name, char *filter)
{
  char errbuf[PCAP_ERRBUF_SIZE];
  if ( (*live_handle = pcap_open_live(live_name,    // name of the device
                            PCAP_CAPTURE_PACKET_LEN,// portion of the packet to capture
                            1,                      // promiscuous mode
                            500,                    // read timeout
                            errbuf                  // error buffer
                            ) ) == NULL)
  {
    return FE_INVAL;
  }
  return set_pcap_filter(*live_handle, filter);
}

#endif
 