#ifndef __DIS68K_H
#define __DIS68K_H

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
#define DIS68K_ERR_DIRECTORY -4
#define DIS68K_ERR_MEMORY -5

typedef uint8_t dis68k_fs_hints;
#define DIS68K_FS_USE_LFN 0x01
#define DIS68K_FS_FOLLOW_SYMLINKS 0x02
#define DIS68K_FS_REPLACE_SECTOR0 0x04
#define DIS68K_FS_HAS_DIFC_HEADER 0x10

typedef uint8_t dis68k_bool;

typedef struct {
  TCHAR* label;
  DWORD vsn;
} dis68k_fs_info;

typedef struct {
  char* path;     /* native encoding */
  char* basename; /* native encoding */
  FILE* fp;
  FATFS* fs;
  dis68k_fs_info info;
  dis68k_fs_hints hints;
} dis68k_fs;

typedef struct {
  char* src_path; /* Shift-JIS */
  char* dst_path; /* native encoding */
} dis68k_extract_info;

typedef dis68k_extract_info dis68k_pack_info;

typedef struct {
  char* file_name; /* Shift-JIS */
  DWORD fsize;
  WORD fdate;
  WORD ftime;
  void* next;
} dis68k_fileentry;

dis68k_status dis68k_open_fs(dis68k_fs* fs, const char* path,
                             dis68k_bool write);
dis68k_status dis68k_close_fs(dis68k_fs* fs);
dis68k_status dis68k_list(dis68k_fs* fs, dis68k_fileentry** flp);
dis68k_status dis68k_extract(dis68k_fs* fs, dis68k_extract_info* hints);
dis68k_status dis68k_pack(dis68k_fs* fs, dis68k_pack_info* hints);
dis68k_status dis68k_free_filelist(dis68k_fileentry* fl);

#endif  // __DIS68K_H
