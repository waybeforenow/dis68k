#include <iconv.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

#include "dis68k_main.h"
#include "dis68k_tuning.h"

dis68k_status dis68k__list(dis68k_fs* fs, dis68k_fileentry** flp) {
  FRESULT res;
  FILINFO fno;
  DIR dir;
  char* path;

  dis68k__dir_frame root_frame = {"", &dir, 0, NULL};
  dis68k__dir_frame* this_frame = &root_frame;

  dis68k_fileentry* last;
  *flp = NULL;

  if (fs->hints & DIS68K_FS_USE_LFN) {
    char lfn[DIS68K_MAX_PATH_SIZE + 1]; /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = DIS68K_MAX_PATH_SIZE + 1;
  }

  do {
    if (!this_frame->dir_open) {
      res = f_opendir(this_frame->dir,
                      this_frame->dir_name); /* Open the directory */
      if (res != FR_OK) {
        printf("%s (%d)\n", f_errstr(res), res);
        return DIS68K_ERR_FILE;
      }

      this_frame->dir_open = 1;
    }

    while (1) {
      res = f_readdir(this_frame->dir, &fno);
      if (fno.fname[0] == '.') continue; /* Ignore dot entry */
      if (res != FR_OK || fno.fname[0] == 0) {
        /* Unwind stack on error or end of dir */
        f_closedir(this_frame->dir);

        dis68k__dir_frame* old_frame = this_frame;
        this_frame = this_frame->prev;

        if (this_frame != NULL) {
          free(old_frame->dir);
          free(old_frame);
        }

        break;
      }

      if (fs->hints & DIS68K_FS_USE_LFN) {
        path = *fno.lfname ? fno.lfname : fno.fname;
      } else {
        path = fno.fname;
      }

      if (fno.fattrib & AM_DIR) { /* It is a directory */
        dis68k__dir_frame* new_frame =
            (dis68k__dir_frame*)calloc(1, sizeof(dis68k__dir_frame));

        char* dir_name = calloc(strlen(this_frame->dir_name) + strlen(path) + 2,
                                sizeof(char));
        *dir_name = '\0';
        if (this_frame->prev != NULL) {
          strcat(dir_name, this_frame->dir_name);
          strcat(dir_name, "/");
        }
        new_frame->dir_name = strcat(dir_name, path);
        new_frame->dir = (DIR*)calloc(1, sizeof(DIR));
        new_frame->dir_open = 0;
        new_frame->prev = this_frame;
        this_frame = new_frame;
        break;
      }

      dis68k_fileentry* new_entry =
          (dis68k_fileentry*)calloc(1, sizeof(dis68k_fileentry));
      new_entry->fdate = fno.fdate;
      new_entry->ftime = fno.ftime;
      new_entry->next = NULL;

      size_t full_path_len = strlen(this_frame->dir_name) + strlen(path) + 2;
      char* file_name = calloc(full_path_len, sizeof(char));
      *file_name = '\0';
      if (this_frame->prev != NULL) {
        strcat(file_name, this_frame->dir_name);
        strcat(file_name, "/");
      }
      new_entry->file_name = strcat(file_name, path);

      /* add the entry */
      if (*flp == NULL) {
        *flp = new_entry;
        last = new_entry;
      } else {
        last->next = new_entry;
        last = new_entry;
      }
    }
  } while (this_frame != NULL);

  return DIS68K_OK;
}

dis68k_status dis68k__extract(dis68k_fs* fs, dis68k_extract_info* hints) {
  dis68k_fileentry* list = NULL;

  // create the dst_path directory
  if (mkdir(hints->dst_path) < 0) {
    /* XXX handle errno */
  }

  char oldwd[DIS68K_MAX_PATH_SIZE];
  (void*)getcwd(oldwd, DIS68K_MAX_PATH_SIZE);
  if (chdir(hints->dst_path) < 0) {
    /* XXX handle errno */
  }

  char oldroot[DIS68K_MAX_PATH_SIZE];
  (void*)getcwd(oldroot, DIS68K_MAX_PATH_SIZE);

  // traverse the disk image filesystem tree
  DIS68K_ERR_WRAP(dis68k__list(fs, &list));

  // extract each file from the list
  while (list != NULL) {
    if (chdir(oldroot) < 0) {
      /* XXX handle errno */
    }

    iconv_t ic = iconv_open(DIS68K_NATIVE_ENCODING, "Shift_JIS");
    int ib = strlen(list->file_name);
    int ob = ib * 2;
    char* dstpath = (char*)calloc(ob, sizeof(char));
    if (dstpath == NULL) {
      /* XXX handle oom */
    }
    // iconv(ic, &(list->file_name), &ib, &dstpath, &ob);
    if (ib > 0) {
      /* XXX handle this */
    }
    iconv_close(ic);

    // create destination folder, if it doesn't already exist
    char* ds = list->file_name;
    char* pds = ds;
    while ((ds = strchr(pds + 1, '/')) != NULL) {
      size_t len = sizeof(char) * (ds - pds + 1);
      char* new_dir = (char*)calloc(len, sizeof(char));
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
      puts("Fuck.");
      exit(1);
    }

    FILE* f = fopen(pds, "wb");
    if (!f) {
      fprintf(stderr, "Could not open %s: %s\n", list->file_name,
              strerror(errno));
      return DIS68K_ERR_FILE;
    }

    FIL fp;
    BYTE* buf = (BYTE*)calloc(DIS68K_READ_SIZE, sizeof(BYTE));
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
    free(dstpath);

    list = list->next;
  }

  if (chdir(oldwd) < 0) {
    /* XXX handle errno */
  }

  return dis68k__free_filelist(list);
}

dis68k_status dis68k__open_fs(dis68k_fs* fs, const char* path) {
  fs->fp = fopen(path, "rb");
  if (!fs->fp) {
    fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno));
    return DIS68K_ERR_FILE;
  }

  extern int offset;
  char dim_head[256];
  fread(dim_head, 256, 1, fs->fp);
  if (!strncmp(dim_head + 0xab, "DIFC HEADER", 11)) offset = 256;
  fs->fs = (FATFS*)calloc(1, sizeof(FATFS));

  // find the last occurence of a period in the basename
  char* bn = basename((char*)path);
  char* ext = strrchr(bn, '.');
  if (ext == NULL) {
    return DIS68K_ERR_FILENAME;
  }

  size_t bn_len = sizeof(char) * (ext - bn + 1);
  char* bn_copy = (char*)calloc(bn_len, sizeof(char));
  strncpy(bn_copy, bn, bn_len - 1);

  fs->basename = bn_copy;
  extern dis68k_fs* dis68k_global_fs;
  dis68k_global_fs = fs;
  DIS68K_ERR_WRAP(f_mount(fs->fs, "", 1));
  return DIS68K_OK;
}

dis68k_status dis68k__close_fs(dis68k_fs* fs) {
  fclose(fs->fp);
  DIS68K_ERR_WRAP(f_mount(NULL, "", 0));
  free(fs->basename);

  return DIS68K_OK;
}

dis68k_status dis68k__free_filelist(dis68k_fileentry* fl) {
  dis68k_fileentry* tmp;
  while (fl != NULL) {
    tmp = fl->next;
    free(fl);
    fl = tmp;
  }

  return DIS68K_OK;
}

void dis68k_usage(const char* invocation) {
  printf("Usage: %s [operation] [filename]\n", invocation);
  puts("\toperation: either 'l' for list or 'x' for extract");
  puts("\tfilename:  path to the DIM/XDF file");
}

int main(int argc, char** argv) {
  if (argc < 3) {
    dis68k_usage(argv[0]);
    return 1;
  }

  dis68k_fs fs = {0};
  if (dis68k__open_fs(&fs, argv[2]) != DIS68K_OK) {
    puts("cant open fs :/");
    return 4;
  }

  if (*argv[1] == 'x') {
    dis68k_extract_info opts = {"", fs.basename};
    if (dis68k__extract(&fs, &opts) != DIS68K_OK) {
      puts("cant extract.....");
      return 5;
    }

    if (dis68k__close_fs(&fs) != DIS68K_OK) {
      puts("cant free");
      return 3;
    }
  } else if (*argv[1] == 'l') {
    dis68k_fileentry* fl = NULL;
    if (dis68k__list(&fs, &fl) != DIS68K_OK) {
      puts("something went wrong");
      return 2;
    }

    iconv_t ic = iconv_open(DIS68K_NATIVE_ENCODING "//IGNORE", "Shift_JIS");
    while (fl != NULL) {
      size_t ib = sizeof(fl->file_name);
      size_t ob = ib * 2;
      char* gaijin_out = (char*)calloc(ob, sizeof(char));
      puts(fl->file_name);

      iconv(ic, &(fl->file_name), &ib, &gaijin_out, &ob);
      // puts(gaijin_out);
      iconv(ic, NULL, NULL, NULL, NULL);

      fl = fl->next;
    }
    iconv_close(ic);

    if (dis68k__free_filelist(fl) != DIS68K_OK ||
        dis68k__close_fs(&fs) != DIS68K_OK) {
      puts("cant free");
      return 3;
    }
  } else {
    dis68k_usage(argv[0]);
    return 1;
  }

  return 0;
}

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

