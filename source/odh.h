#ifndef ODH_H
#define ODH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int decode(uint8_t *cdbfile, uint32_t attachment_size, uint32_t attachment_offset, char *outpath);

#ifdef __cplusplus
}
#endif

#endif // ODH_H