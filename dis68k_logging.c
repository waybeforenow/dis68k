#include <stdint.h>

#include "dis68k_logging.h"
#include "fatfs/ff.h"

dis68k_verbosity dis68k_global_loglevel = DIS68K_LEVEL_ERROR |
                                          DIS68K_LEVEL_WARN | DIS68K_LEVEL_INFO
#ifdef _DEBUG
                                          | DIS68K_LEVEL_DEBUG
#endif
    ;

const char* f_errstr(FRESULT r) {
  const char* errstrs[] = {
      "Succeeded",
      "A hard error occurred in the low level disk I/O layer",
      "Assertion failed",
      "The physical drive cannot work",
      "Could not find the file",
      "Could not find the path",
      "The path name format is invalid",
      "Access denied due to prohibited access or directory full",
      "Access denied due to prohibited access",
      "The file/directory object is invalid",
      "The physical drive is write protected",
      "The logical drive number is invalid",
      "The volume has no work area",
      "There is no valid FAT volume",
      "The f_mkfs() aborted due to any parameter error",
      "Could not get a grant to access the volume within defined period",
      "The operation is rejected according to the file sharing policy",
      "LFN working buffer could not be allocated",
      "Number of open files > _FS_SHARE",
      "Given parameter is invalid",
  };
  if (r <= 19) return errstrs[r];
  return "Unknown error";
}

