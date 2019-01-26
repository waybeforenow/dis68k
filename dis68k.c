#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/libfat-human68k/diskio.h"
#include "lib/libfat-human68k/ff.h"

#include "dis68k.h"
#include "dis68k_logging.h"
#include "dis68k_tuning.h"

typedef struct {
  char* dir_name; /* Shift-JIS */
  void* dir;      /* could be (FATFS) F_DIR or (POSIX) DIR */
  dis68k_bool dir_open;
  void* prev;
} dis68k__dir_frame;

void* dis68k_malloc(dis68k_status* s, dis68k_bool zero, size_t t, size_t size) {
  void* ret = NULL;

  if (zero) {
    ret = calloc(t, size);
  } else {
    ret = malloc(t * size);
  }

  if (s != NULL) *s = (ret == NULL) ? DIS68K_ERR_MEMORY : DIS68K_OK;
  return ret;
}

dis68k_status dis68k_list(human68k_fs* fs, dis68k_fileentry** flp) {
  FRESULT res;
  FILINFO fno = {0};
  F_DIR dir;
  char* path;

  dis68k__dir_frame root_frame = {"", &dir, 0, NULL};
  dis68k__dir_frame* this_frame = &root_frame;

  dis68k_fileentry* last;
  *flp = NULL;

  if (fs->hints & FS_USE_LFN) {
    char lfn[DIS68K_MAX_PATH_SIZE + 1]; /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = DIS68K_MAX_PATH_SIZE + 1;
  }

  DIS68K_LOG_DEBUG("Beginning directory traversal");
  do {
    if (!this_frame->dir_open) {
      /* Open the directory */
      DIS68K_LOGF_DEBUG("Opening dir %s", this_frame->dir_name);
      DIS68K_ERR_WRAP(f_opendir((F_DIR*)this_frame->dir, this_frame->dir_name));
      DIS68K_LOG_DEBUG("Done opening dir");

      this_frame->dir_open = 1;
    }

    while (1) {
      DIS68K_LOG_DEBUG("Reading from dir");
      res = f_readdir(this_frame->dir, &fno);
      DIS68K_LOGF_DEBUG("New entry: %s", (res != FR_OK || fno.fname[0] == 0)
                                             ? "(invalid)"
                                             : fno.fname);

      if (fno.fname[0] == '.') continue; /* Ignore dot entry */
      if (res != FR_OK || fno.fname[0] == 0) {
        /* Unwind stack on error or end of dir */
        DIS68K_LOGF_DEBUG("Closing dir %s", (*(this_frame->dir_name) == '\0')
                                                ? "(root)"
                                                : this_frame->dir_name);
        f_closedir(this_frame->dir);

        dis68k__dir_frame* old_frame = this_frame;
        this_frame = this_frame->prev;

        if (this_frame != NULL) {
          DIS68K_LOG_DEBUG("Freeing current frame and returning");
          free(old_frame->dir_name);
          free(old_frame);

          DIS68K_LOGF_DEBUG("New current directory: %s",
                            (*(this_frame->dir_name) == '\0')
                                ? "(root)"
                                : this_frame->dir_name);
        }

        break;
      }

      if (fs->hints & FS_USE_LFN) {
        path = *fno.lfname ? fno.lfname : fno.fname;
      } else {
        path = fno.fname;
      }

      if (fno.fattrib & AM_DIR) {
        /* Traverse the directory. */
        DIS68K_LOG_DEBUG("It's a directory; traversing it.");

        dis68k__dir_frame* new_frame = (dis68k__dir_frame*)dis68k_malloc(
            NULL, 1, 1, sizeof(dis68k__dir_frame));

        char* dir_name = (char*)dis68k_malloc(
            NULL, 1, strlen(this_frame->dir_name) + strlen(path) + 2,
            sizeof(char));
        *dir_name = '\0';
        if (this_frame->prev != NULL) {
          strcat(dir_name, this_frame->dir_name);
          strcat(dir_name, "/");
        }
        new_frame->dir_name = strcat(dir_name, path);
        new_frame->dir = (F_DIR*)dis68k_malloc(NULL, 1, 1, sizeof(F_DIR));
        new_frame->dir_open = 0;
        new_frame->prev = this_frame;
        this_frame = new_frame;

        DIS68K_LOGF_DEBUG("New current directory: %s", new_frame->dir_name);
        break;
      }

      DIS68K_LOG_DEBUG("It's a file; adding it to the list.");
      dis68k_fileentry* new_entry = (dis68k_fileentry*)dis68k_malloc(
          NULL, 0, 1, sizeof(dis68k_fileentry));
      new_entry->fdate = fno.fdate;
      new_entry->ftime = fno.ftime;
      new_entry->next = NULL;

      size_t full_path_len = strlen(this_frame->dir_name) + strlen(path) + 2;
      char* file_name = dis68k_malloc(NULL, 0, full_path_len, sizeof(char));
      *file_name = '\0';
      if (this_frame->prev != NULL) {
        strcat(file_name, this_frame->dir_name);
        strcat(file_name, "/");
      }
      new_entry->file_name = strcat(file_name, path);
      DIS68K_LOGF_DEBUG("Filename is %s", new_entry->file_name);

      /* add the entry */
      if (*flp == NULL) {
        DIS68K_LOG_DEBUG(
            "Replacing NULL head pointer with current file entry.");
        *flp = new_entry;
        last = new_entry;
      } else {
        DIS68K_LOG_DEBUG(
            "Appending current file entry to the end of the list.");
        last->next = new_entry;
        last = new_entry;
      }
    }
  } while (this_frame != NULL);

  DIS68K_LOG_DEBUG("Directory traversal finished");
  return DIS68K_OK;
}

#ifndef _WIN32
#define mkdir(D) mkdir(D, S_IRWXU | S_IRWXG | S_IRWXO)
#endif

dis68k_status dis68k_extract(human68k_fs* fs, dis68k_extract_info* hints) {
  dis68k_fileentry* list = NULL;

  // create the dst_path directory
  if (mkdir(hints->dst_path) < 0) {
    /* XXX handle errno */
  }

  char oldwd[DIS68K_MAX_PATH_SIZE];
  (void)getcwd(oldwd, DIS68K_MAX_PATH_SIZE);
  if (chdir(hints->dst_path) < 0) {
    /* XXX handle errno */
  }

  char oldroot[DIS68K_MAX_PATH_SIZE];
  (void)getcwd(oldroot, DIS68K_MAX_PATH_SIZE);

  // traverse the disk image filesystem tree
  DIS68K_ERR_WRAP(dis68k_list(fs, &list));

  // extract each file from the list
  while (list != NULL) {
    if (chdir(oldroot) < 0) {
      /* XXX handle errno */
    }

    // create destination folder, if it doesn't already exist
    char* ds = list->file_name;
    char* pds = ds;
    while ((ds = strchr(pds + 1, '/')) != NULL) {
      size_t len = sizeof(char) * (ds - pds + 1);
      char* new_dir = (char*)dis68k_malloc(NULL, 1, len, sizeof(char));
      strncpy(new_dir, pds, len - 1);
      if (mkdir(new_dir) < 0) {
        /* XXX handle errno */
      }
      if (chdir(new_dir) < 0) {
        /* XXX handle errno */
      }
      pds = ds;
    }

    if (*pds == '/') {
      pds += sizeof(char);
    }
    if (*pds == '/') {
      DIS68K_LOG_FATAL("Tried to write a filename with a leading slash.");
    }

    FILE* f = fopen(pds, "wb");
    if (!f) {
      fprintf(stderr, "Could not open %s: %s\n", list->file_name,
              strerror(errno));
      return DIS68K_ERR_FILE;
    }

    FIL fp;
    BYTE* buf = (BYTE*)dis68k_malloc(NULL, 1, DIS68K_READ_SIZE, sizeof(BYTE));
    UINT r = 0;

    DIS68K_ERR_WRAP(f_open(&fp, list->file_name, FA_READ));
    printf("Writing %s/%s... ", hints->dst_path, list->file_name);
    fflush(stdout);
    do {
      DIS68K_ERR_WRAP(f_read(&fp, buf, DIS68K_READ_SIZE, &r));
      fwrite(buf, r, 1, f);
    } while (r == DIS68K_READ_SIZE);
    DIS68K_ERR_WRAP(f_close(&fp));
    puts("Success");
    fflush(stdout);

    fclose(f);
    free(buf);

    list = list->next;
  }

  if (chdir(oldwd) < 0) {
    /* XXX handle errno */
  }

  return dis68k_free_filelist(list);
}

#ifndef _WIN32
#undef mkdir
#endif

#ifdef _WIN32
#define lstat stat
#define S_ISLNK(fx) 0 && fx
#endif

dis68k_status dis68k_pack(human68k_fs* fs, dis68k_pack_info* hints) {
  struct dirent* de;
  struct stat st;
  dis68k__dir_frame root_frame = {hints->src_path, NULL, 0, NULL};
  dis68k__dir_frame* this_frame = &root_frame;
  if (root_frame.dir == NULL) {
    /* XXX handle errno */
  }

  DIS68K_LOG_DEBUG("Beginning directory traversal");
  while (this_frame != NULL) {
    if (!this_frame->dir_open) {
      DIS68K_LOGF_DEBUG("Opening dir %s", this_frame->dir_name);
      if (!(this_frame->dir = opendir(this_frame->dir_name))) {
        DIS68K_LOGF_WARN("can't open %s", this_frame->dir_name);
        return DIS68K_ERR_DIRECTORY;
      }
      DIS68K_LOG_DEBUG("Done opening dir");

      this_frame->dir_open = 1;
    }

    char* dst_dirname = this_frame->dir_name +
                        (sizeof(char) * (strlen(root_frame.dir_name) + 1));
    if (dst_dirname[-1] != '\0') {
      printf("Creating directory %s... ", dst_dirname);
      FRESULT fr = f_mkdir(dst_dirname);
      if (fr != FR_OK && fr != FR_EXIST) {
        printf("error %s\n", f_errstr(fr));
        return DIS68K_ERR_FS;
      }
      puts("Created.");
    }

    while (1) {
      DIS68K_LOG_DEBUG("Reading from dir");
      de = readdir(this_frame->dir);
      if (de == NULL) {
        /* Unwind stack on end of dir */
        DIS68K_LOGF_DEBUG("Closing dir %s", this_frame->dir_name);
        closedir(this_frame->dir);

        dis68k__dir_frame* old_frame = this_frame;
        this_frame = this_frame->prev;

        if (this_frame != NULL) {
          DIS68K_LOG_DEBUG("Freeing current frame and returning");
          free(old_frame->dir_name);
          free(old_frame);
          DIS68K_LOGF_DEBUG("New current directory: %s", this_frame->dir_name);
        }

        break;
      }

      DIS68K_LOGF_DEBUG("New entry: %s", de->d_name);
      char* src_path = (char*)dis68k_malloc(
          NULL, 1, strlen(this_frame->dir_name) + strlen(de->d_name) + 2,
          sizeof(char));
      strcat(src_path, this_frame->dir_name);
      strcat(src_path, "/");
      strcat(src_path, de->d_name);

      if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
      if (lstat(src_path, &st) == -1) {
        DIS68K_LOGF_WARN("Can't stat %s", src_path);
        return DIS68K_ERR_DIRECTORY;
      }

      /* only follow symlinks if specified */
      if (S_ISLNK(st.st_mode) && !(fs->hints & FS_FOLLOW_SYMLINKS)) continue;

      /* recursively follow dirs */
      if (S_ISDIR(st.st_mode)) {
        DIS68K_LOG_DEBUG("It's a directory; traversing it.");
        dis68k__dir_frame* new_frame = (dis68k__dir_frame*)dis68k_malloc(
            NULL, 1, 1, sizeof(dis68k__dir_frame));

        char* dir_name = dis68k_malloc(
            NULL, 1, strlen(this_frame->dir_name) + strlen(de->d_name) + 2,
            sizeof(char));
        strcat(dir_name, this_frame->dir_name);
        strcat(dir_name, "/");
        new_frame->dir_name = strcat(dir_name, de->d_name);
        new_frame->dir = (DIR*)dis68k_malloc(NULL, 1, 1, sizeof(F_DIR));
        new_frame->dir_open = 0;
        new_frame->prev = this_frame;
        this_frame = new_frame;

        DIS68K_LOGF_DEBUG("New current directory: %s", new_frame->dir_name);
        break;
      }

      /* write files to the archive */
      DIS68K_LOG_DEBUG("It's a file; adding it to the list.");
      int srcfd;
      FIL dstfd;
      BYTE buffer[DIS68K_READ_SIZE];
      size_t bs = 0;
      UINT bd = 0;
      char* dst_path =
          src_path + sizeof(char) * (strlen(root_frame.dir_name) + 1);

      srcfd = open(src_path, O_RDONLY, 0);
      printf("Packing %s... ", dst_path);
      DIS68K_ERR_WRAP(f_open(&dstfd, dst_path, FA_WRITE | FA_CREATE_ALWAYS));
      do {
        bs = read(srcfd, buffer, DIS68K_READ_SIZE);
        DIS68K_ERR_WRAP(f_write(&dstfd, buffer, (UINT)bs, &bd));
      } while (bs != 0 && bd != 0 && bs == bd);
      DIS68K_ERR_WRAP(f_close(&dstfd));
      puts("Success");
      close(srcfd);
    }
  }

  DIS68K_LOG_DEBUG("Directory traversal finished");
  return DIS68K_OK;
}

dis68k_status dis68k_open_fs(human68k_fs* fs, const char* path,
                             dis68k_bool write) {
  fs->fp = fopen(path, (write) ? "wb" : "rb");
  if (!fs->fp) {
    DIS68K_LOGF_WARN("Could not open %s: %s", path, strerror(errno));
    return DIS68K_ERR_FILE;
  }

  fs->fs = (FATFS*)dis68k_malloc(NULL, 1, 1, sizeof(FATFS));
  fs->hints |= FS_REPLACE_SECTOR0;
  if (!write) {
    char dim_head[256];
    fread(dim_head, 256, 1, fs->fp);
    if (!strncmp(dim_head + 0xab, "DIFC HEADER", 11)) {
      fs->hints |= FS_HAS_DIFC_HEADER;
    }
  }

  // find the last occurence of a period in the basename
  char* bn = basename((char*)path);
  char* ext = strrchr(bn, '.');
  if (ext == NULL) {
    return DIS68K_ERR_FILENAME;
  }

  size_t bn_len = sizeof(char) * (ext - bn + 1);
  char* bn_copy = (char*)dis68k_malloc(NULL, 1, bn_len, sizeof(char));
  strncpy(bn_copy, bn, bn_len - 1);
  fs->basename = bn_copy;

  extern human68k_fs* human68k_global_fs;
  human68k_global_fs = fs;

  DIS68K_ERR_WRAP(f_mount(fs->fs, "", 1));
  if (write) {
    DIS68K_ERR_WRAP(f_mkfs("", 0, 0));
  } else {
    fs->info.label = (TCHAR*)dis68k_malloc(NULL, 1, 24, sizeof(TCHAR));
    DIS68K_ERR_WRAP(f_getlabel("", fs->info.label, &(fs->info.vsn)));
  }

  return DIS68K_OK;
}

#ifdef _WIN32
#undef lstat
#undef S_ISLNK
#endif

dis68k_status dis68k_close_fs(human68k_fs* fs) {
  fclose(fs->fp);
  DIS68K_ERR_WRAP(f_mount(NULL, "", 0));
  free(fs->info.label);
  free(fs->basename);

  return DIS68K_OK;
}

dis68k_status dis68k_free_filelist(dis68k_fileentry* fl) {
  dis68k_fileentry* tmp;
  while (fl != NULL) {
    tmp = fl->next;
    free(fl);
    fl = tmp;
  }

  return DIS68K_OK;
}

