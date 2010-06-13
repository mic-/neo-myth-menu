#ifndef zip_io_g
#define zip_io_g

#include <memory.h>
#include <basetypes.h>

#ifdef __cplusplus
    extern "C" {
#endif

FRESULT f_open_zip (FIL*, const XCHAR*, BYTE);          /* Open or create a file */
FRESULT f_read_zip (FIL*, void*, UINT, UINT*);          /* Read data from a file */
FRESULT f_lseek_zip (FIL*, DWORD);                      /* Move file pointer of a file object */
FRESULT f_close_zip (FIL*);                             /* Close an open file object */


#ifdef __cplusplus
    }
#endif

#endif
