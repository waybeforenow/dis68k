/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "dis68k_io.h"
#include "diskio.h"

// Global filesystem object.
// TODO: this should be a pointer to an array, instead of to a single struct.
dis68k_fs *dis68k_global_fs;

// Initialize a human68k fs.
DSTATUS disk_initialize(BYTE pdrv) {
  DIS68K_LOG_DEBUG("FatFs requested disk initialize");
  // TODO: open the file.

  return RES_OK;
}

// Get fs status.
DSTATUS disk_status(BYTE pdrv) {
  DIS68K_LOG_DEBUG("FatFs requested disk status");
  return RES_OK;
}

// Read data from the fs.
DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
  DIS68K_LOGF_DEBUG("FatFs requested to read %u bytes from sector %lu", count,
                    sector);
  if ((dis68k_global_fs->hints & DIS68K_FS_REPLACE_SECTOR0) && sector == 0) {
    // read from the fake boot-sector instead
    memcpy(buff, fake_bootsect, DIS68K_SECTOR_SIZE);

    count--;
    sector++;
    buff += sizeof(BYTE) * DIS68K_SECTOR_SIZE;
  }

  fseek(dis68k_global_fs->fp,
        (dis68k_global_fs->hints & DIS68K_FS_HAS_DIFC_HEADER)
            ? sector * DIS68K_SECTOR_SIZE + DIS68K_DIFC_HEADER_SIZE
            : sector * DIS68K_SECTOR_SIZE,
        SEEK_SET);
  fread(buff, DIS68K_SECTOR_SIZE, count, dis68k_global_fs->fp);
  return RES_OK;
}

// Write data to the fs.
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  DIS68K_LOGF_DEBUG("FatFs requested to write %u bytes to sector %lu", count,
                    sector);

  // sector 0 is reserved for the fake header XXX
  if (sector == 0) return RES_OK;

  fseek(dis68k_global_fs->fp, ((long)sector) * DIS68K_SECTOR_SIZE, SEEK_SET);
  if (fwrite(buff, count, 1, dis68k_global_fs->fp) < count) {
    return 1; /* XXX */
  }

  return RES_OK;
}

// IOCTL functions.
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
  DIS68K_LOGF_DEBUG("FatFs requested ioctl %u", cmd);
  switch (cmd) {
    case CTRL_SYNC: {
      fflush(dis68k_global_fs->fp);
      break;
    }
    case GET_SECTOR_SIZE: {
      *(UINT *)buff = DIS68K_SECTOR_SIZE;
      break;
    }
    case GET_SECTOR_COUNT: {
      *(UINT *)buff = 0xc800;
      break;
    }
  }

  return RES_OK;
}

DWORD get_fattime(void) {
  time_t t = time(NULL);
  struct tm *tmr = localtime(&t);
  int year = tmr->tm_year < 80 ? 0 : tmr->tm_year - 80;
  return ((DWORD)(year) << 25) | ((DWORD)(tmr->tm_mon + 1) << 21) |
         ((DWORD)tmr->tm_mday << 16) | (WORD)(tmr->tm_hour << 11) |
         (WORD)(tmr->tm_min << 5) | (WORD)(tmr->tm_sec >> 1);
}

