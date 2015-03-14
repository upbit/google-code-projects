#include <getopt.h>
#include "fay_original.h"

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/
void printUsage();

int main(int argc, char **argv)
{
  char *offline_file_name = NULL;
  int opt;
  const char *shortopts = "i:r:fh";
  const struct option longopts[] = {
      {"interface", 1, NULL, 'i'},
      {"readfile", 1, NULL, 'r'},
      {"full-result", 0, NULL, 'f'},
      {"help", 0, NULL, 'h'},
      {NULL, 0, NULL, 0}
  };
  // initialize fayrc
  init_fayoriginal_resource();
  while((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
  {
    switch(opt) {
      case 'i':
        sscanf(optarg, "%[^,],%[^,]", fayrc->live_name1, fayrc->live_name2);
        if (strchr(optarg, ',') == 0) {
          fayrc->init_flags |= INIT_OPENLIVE;
        }else {
          fayrc->init_flags |= INIT_OPENLIVE_DUAL;
        }
        break;
      case 'r':
        offline_file_name = optarg;
        fayrc->init_flags |= INIT_OFFLINE;
        break;
      case 'f':
        fayrc->init_flags |= INIT_ALL_RESULT;
        break;
      case 'h':
        printUsage(argv[0]);
        return 0;
      case '?':
        fay_debug("usage: %s [options]\ntype \"%s -h\" for more help.\n", argv[0], argv[0]);
        return 0;
    }
  }
  
  if (fayrc->init_flags & INIT_ANALYSIS_MASK)
  {
    if (fayrc->init_flags & INIT_OFFLINE) {
      open_pcap_offline(offline_file_name);           // 分析离线文件
    }else {
      register_handle_signal(1);
      open_pcap_livecapture(fayrc->live_name1, fayrc->live_name2);  // 实时抓包分析
    }
  }
  else  // 没有指定分析数据源，输出使用提示
  {
    fay_debug("usage: %s [options]\ntype \"%s -h\" for more help.\n", argv[0], argv[0]);
  }
  
  free_fayoriginal_resource();
  return 0;
}

void printUsage(char* cmd_line)
{
  fay_debug("usage: %s [options]\n", cmd_line);
  fay_debug("  -r --read <FileName>            *read offline dump\n");
  fay_debug("  -i --interface <ethX>           *listen on interface\n");
  fay_debug("  -i --interface <rx,tx>          *listen on dual-interface, e.g.:\"-i eth2,eth3\"\n");
  fay_debug("                                   Note that the order of device name must be \"rx,tx\"!\n");
  fay_debug("  -f --full-result                output full result\n");
  fay_debug("  -h --help                       show this help\n");
}
