#include <string.h>

#include "ff.h"

/* Embed the .fur file contents in the ROM. */
#include "file.fur.inc"

#define FAKE_FILE_DATA file_2024_10_30T002921_486_fur
#define FAKE_FILE_LEN file_2024_10_30T002921_486_fur_len

FRESULT f_open (FIL* fp, const char* path, uint8_t mode)
{
    (void)path;
    (void)mode;
    fp->offset = 0;
    return FR_OK;
}

FRESULT f_read (FIL* fp, void* buff, UINT bytes_to_read, UINT* bytes_read)
{
    int64_t bytes_available_i = FAKE_FILE_LEN - fp->offset;
    if (bytes_available_i < 0)
    {
        *bytes_read = 0;
        return FR_OK;
    }
    UINT bytes_available = (UINT)bytes_available_i;

    UINT actually_read = (bytes_to_read > bytes_available ? bytes_available : bytes_to_read);
    memcpy(buff, FAKE_FILE_DATA + fp->offset, actually_read);
    *bytes_read = actually_read;
    fp->offset += actually_read;
    return FR_OK;
}

FRESULT f_lseek (FIL* fp, UINT ofs)
{
    fp->offset = ofs;
    return FR_OK;
}

UINT f_tell(FIL* fp)
{
    return fp->offset;
}
