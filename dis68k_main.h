#ifndef __DIS68K_MAIN
#define __DIS68K_MAIN

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

typedef int8_t dis68k_status;
#define DIS68K_OK 0
#define DIS68K_ERR_UNSPECIFIED -1
#define DIS68K_ERR_FILE -2
#define DIS68K_ERR_FILENAME -3
#define DIS68K_ERR_FS -3

typedef uint8_t dis68k_fs_hints;
#define DIS68K_FS_USE_LFN 0x01

typedef struct {
  char* path;     /* native encoding */
  char* basename; /* native encoding */
  FILE* fp;
  FATFS* fs;
  dis68k_fs_hints hints;
} dis68k_fs;

typedef struct {
  char* src_path; /* Shift-JIS */
  char* dst_path; /* native encoding */
} dis68k_extract_info;

typedef struct {
  char* file_name; /* Shift-JIS */
  DWORD fsize;
  WORD fdate;
  WORD ftime;
  void* next;
} dis68k_fileentry;

typedef struct {
  char* dir_name; /* Shift-JIS */
  DIR* dir;
  uint8_t dir_open;
  void* prev;
} dis68k__dir_frame;

dis68k_status dis68k__open_fs(dis68k_fs* fs, const char* path);
dis68k_status dis68k__close_fs(dis68k_fs* fs);
dis68k_status dis68k__list(dis68k_fs* fs, dis68k_fileentry** flp);
dis68k_status dis68k__extract(dis68k_fs* fs, dis68k_extract_info* hints);
dis68k_status dis68k__free_filelist(dis68k_fileentry* fl);

void dis68k_usage(const char* invocation);

const char* f_errstr(FRESULT r);
#define DIS68K_ERR_WRAP(fx)                                                   \
  {                                                                           \
    FRESULT fr;                                                               \
    fr = (fx);                                                                \
    if (fr) {                                                                 \
      printf("%s:%d: Error %d calling %s: %s\n", __FILE__, __LINE__, fr, #fx, \
             f_errstr(fr));                                                   \
      exit(1);                                                                \
    }                                                                         \
  }

#endif  // __DIS68K_MAIN
