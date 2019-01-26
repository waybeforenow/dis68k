#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/libfat-human68k/diskio.h"
#include "lib/libfat-human68k/ff.h"

#include "dis68k.h"
#include "dis68k_logging.h"
#include "dis68k_tuning.h"

typedef int8_t dis68k_operation;
#define DIS68K_UNKNOWN_OPERATION 0
#define DIS68K_LIST 1
#define DIS68K_EXTRACT 2
#define DIS68K_PACK 3
#define DIS68K_INFO 4

void dis68k_usage(const char* invocation) {
  printf("Usage: %s [operation] [filename]\n", invocation);
  puts("\toperation: either 'l' for list or 'x' for extract");
  puts("\tfilename:  path to the DIM/XDF file");
}

#define DIS68K_OPTCMP(s, o) *(s) == o&&*(s + 1) == '\0'

int main(int argc, char** argv) {
  if (argc < 3) {
    dis68k_usage(argv[0]);
    return 1;
  }

  dis68k_operation opcode = DIS68K_UNKNOWN_OPERATION;
  if (DIS68K_OPTCMP(argv[1], 'x'))
    opcode = DIS68K_EXTRACT;
  else if (DIS68K_OPTCMP(argv[1], 'l'))
    opcode = DIS68K_LIST;
  else if (DIS68K_OPTCMP(argv[1], 'i'))
    opcode = DIS68K_INFO;
  else if (DIS68K_OPTCMP(argv[1], 'p')) {
    // need 3 arguments for PACK
    if (argc < 4) {
      dis68k_usage(argv[0]);
      return 1;
    }
    opcode = DIS68K_PACK;
  } else {
    dis68k_usage(argv[0]);
    return 1;
  }

  human68k_fs fs = {0};
  if (dis68k_open_fs(&fs, (opcode == DIS68K_PACK) ? argv[3] : argv[2],
                     (opcode == DIS68K_PACK) != DIS68K_OK)) {
    DIS68K_LOGF_ERROR("Couldn't mount %s as a DIM/XDF image.", argv[2]);
    return 4;
  }

  switch (opcode) {
    case DIS68K_EXTRACT: {
      dis68k_extract_info opts = {"", fs.basename};
      if (dis68k_extract(&fs, &opts) != DIS68K_OK) {
        DIS68K_LOGF_ERROR("Error extracting %s", argv[2]);
        return 5;
      }

      if (dis68k_close_fs(&fs) != DIS68K_OK) {
        DIS68K_LOG_ERROR("Error closing filesystem.");
        return 3;
      }

      break;
    }
    case DIS68K_LIST:
    case DIS68K_INFO: {
      /* TODO: list info here. */
      if (opcode == DIS68K_INFO) break;

      dis68k_fileentry* fl = NULL;
      if (dis68k_list(&fs, &fl) != DIS68K_OK) {
        DIS68K_LOGF_ERROR("Error listing %s", argv[2]);
      }

      while (fl != NULL) {
        puts(fl->file_name);
        fl = fl->next;
      }

      if (dis68k_free_filelist(fl) != DIS68K_OK ||
          dis68k_close_fs(&fs) != DIS68K_OK) {
        DIS68K_LOG_ERROR("Error cleaning up resources.");
        return 3;
      }

      break;
    }

    case DIS68K_PACK: {
      dis68k_pack_info hints;
      hints.src_path = argv[2];
      hints.dst_path = argv[3];

      if (dis68k_pack(&fs, &hints) != DIS68K_OK) {
        DIS68K_LOGF_ERROR("Couldn't pack %s", argv[2]);
        return 6;
      }

      break;
    }
  }

  return 0;
}

