#ifndef _DIS68K_TUNING_H
#define _DIS68K_TUNING_H

#ifndef DIS68K_NATIVE_ENCODING
#define DIS68K_NATIVE_ENCODING "UTF-8"
#endif

#ifndef DIS68K_READ_SIZE
#define DIS68K_READ_SIZE 1024
#endif

#ifndef DIS68K_MAX_PATH_SIZE
#ifdef PATH_MAX
#define DIS68K_MAX_PATH_SIZE PATH_MAX
#else
#define DIS68K_MAX_PATH_SIZE 256
#endif
#endif

#endif  // _DIS68K_TUNING_H
