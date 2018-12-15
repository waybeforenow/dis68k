#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

#include "dis68k_io.h"
#include "dis68k_main.h"

int offset = 0;
dis68k_fs *dis68k_global_fs;

DSTATUS disk_initialize(BYTE pdrv) { return 0; }

DSTATUS disk_status(BYTE pdrv) { return 0; }

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
  if (sector == 0) {  // Fake boot sector here
    memcpy(buff, fake_bootsect, 1024);
  } else {
    fseek(dis68k_global_fs->fp, sector * _MIN_SS + offset, SEEK_SET);
    fread(buff, _MIN_SS, count, dis68k_global_fs->fp);
  }
  return 0;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  return 0;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) { return 0; }

