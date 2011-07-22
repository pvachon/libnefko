#ifndef __INCLUDE_NEFKO_CAMERA_H__
#define __INCLUDE_NEFKO_CAMERA_H__

#include <nefko.h>

/* Header defining data structures containing accessors and required
 * metadata information for post-processing images from different
 * cameras correctly.
 */

/* Structure used to define the actual camera information (name,
 * sensor information, etc.)
 */
typedef struct nefko_camera {
    const char *name;
} nefko_camera_t;

#endif /* __INCLUDE_NEFKO_CAMERA_H__ */

