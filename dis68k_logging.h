#ifndef __DIS68K_LOGGING_H
#define __DIS68K_LOGGING_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib/libfat-human68k/ff.h"

#ifndef DIS68K_LOG_STREAM
#define DIS68K_LOG_STREAM stderr
#endif

typedef uint8_t dis68k_verbosity;
#define DIS68K_LEVEL_ERROR 0x01
#define DIS68K_LEVEL_WARN 0x02
#define DIS68K_LEVEL_INFO 0x04
#define DIS68K_LEVEL_DEBUG 0x10

/* FATAL log entries can't be masked, and cause an immediate program
 * termination. */
#define DIS68K_LOGF_FATAL(fmt, ...)                                   \
  {                                                                   \
    fprintf(DIS68K_LOG_STREAM, "[FATAL] (%s:%d) " fmt "\n", __FILE__, \
            __LINE__, __VA_ARGS__);                                   \
    exit(1);                                                          \
  }
#define DIS68K_LOG_FATAL(msg) DIS68K_LOGF_FATAL("%s", msg)

#define DIS68K_LOGF_ERROR(fmt, ...)                                     \
  {                                                                     \
    extern dis68k_verbosity dis68k_global_loglevel;                     \
    if (dis68k_global_loglevel & DIS68K_LEVEL_ERROR) {                  \
      fprintf(DIS68K_LOG_STREAM, "[ERROR] (%s:%d) " fmt "\n", __FILE__, \
              __LINE__, __VA_ARGS__);                                   \
      fflush(DIS68K_LOG_STREAM);                                        \
    }                                                                   \
  }
#define DIS68K_LOG_ERROR(msg) DIS68K_LOGF_ERROR("%s", msg)

#define DIS68K_LOGF_WARN(fmt, ...)                                      \
  {                                                                     \
    extern dis68k_verbosity dis68k_global_loglevel;                     \
    if (dis68k_global_loglevel & DIS68K_LEVEL_WARN) {                   \
      fprintf(DIS68K_LOG_STREAM, "[WARN]  (%s:%d) " fmt "\n", __FILE__, \
              __LINE__, __VA_ARGS__);                                   \
      fflush(DIS68K_LOG_STREAM);                                        \
    }                                                                   \
  }
#define DIS68K_LOG_WARN(msg) DIS68K_LOGF_WARN("%s", msg)

#define DIS68K_LOG_INFO(fmt, ...)                                       \
  {                                                                     \
    extern dis68k_verbosity dis68k_global_loglevel;                     \
    if (dis68k_global_loglevel & DIS68K_LEVEL_INFO) {                   \
      fprintf(DIS68K_LOG_STREAM, "[INFO]  (%s:%d) " fmt "\n", __FILE__, \
              __LINE__, __VA_ARGS__);                                   \
      fflush(DIS68K_LOG_STREAM);                                        \
    }                                                                   \
  }

#define DIS68K_LOGF_DEBUG(fmt, ...)                                       \
  {                                                                       \
    extern dis68k_verbosity dis68k_global_loglevel;                       \
    if (dis68k_global_loglevel & DIS68K_LEVEL_DEBUG) {                    \
      fprintf(DIS68K_LOG_STREAM, "[DEBUG] (%s:%-4d) " fmt "\n", __FILE__, \
              __LINE__, __VA_ARGS__);                                     \
      fflush(DIS68K_LOG_STREAM);                                          \
    }                                                                     \
  }

#define DIS68K_LOG_DEBUG(msg) DIS68K_LOGF_DEBUG("%s", msg)

#define DIS68K_ERR_WRAP(fx)                                                \
  {                                                                        \
    FRESULT fr;                                                            \
    fr = (fx);                                                             \
    if (fr) {                                                              \
      DIS68K_LOGF_FATAL("Error %d calling %s: %s", fr, #fx, f_errstr(fr)); \
    }                                                                      \
  }

#endif  // __DIS68K_LOGGING_H
