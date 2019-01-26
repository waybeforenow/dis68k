#include <stdint.h>

#include "dis68k_logging.h"
#include "lib/libfat-human68k/ff.h"

dis68k_verbosity dis68k_global_loglevel = DIS68K_LEVEL_ERROR |
                                          DIS68K_LEVEL_WARN | DIS68K_LEVEL_INFO
#ifdef _DEBUG
                                          | DIS68K_LEVEL_DEBUG
#endif
    ;

