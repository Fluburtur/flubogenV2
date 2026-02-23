/**
 * Pretends to be the FatFS API to the SD card.
 * It's actually backed by read-only data in the firmware.
 */

#include <stdint.h>

#define FA_READ 0

typedef uint32_t UINT;

/** File object structure (FIL) */
typedef struct {
    /** File read position (Zeroed on file open) */
    UINT offset;
} FIL;

typedef enum {
    FR_OK = 0,  /* (0) Succeeded */
} FRESULT;

/** Open or create a file */
FRESULT f_open (FIL* fp, const char* path, uint8_t mode);

/** Read data from the file */
FRESULT f_read (FIL* fp, void* buff, UINT bytes_to_read, UINT* bytes_read);

/** Move file pointer of the file object */
FRESULT f_lseek (FIL* fp, UINT ofs);

/** Get the current position within the file */
UINT f_tell(FIL* fp);
