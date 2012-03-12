#ifndef __INCLUDE_NEFKO_CAMERA_H__
#define __INCLUDE_NEFKO_CAMERA_H__

#include <nefko.h>

/* Forward Declarations */
struct nefko_camera;

/* Header defining data structures containing accessors and required
 * metadata information for post-processing images from different
 * cameras correctly.
 */

typedef struct nefko_camera_state {
    struct nefko_camera *camera;        /* The actual camera state manager */
    void *data;                         /* A private data structure used by the given
                                           camera plugin
                                         */
};

/* Structure used to define the actual camera information (name,
 * sensor information, etc.)
 */
typedef struct nefko_camera {
    const char *name;

    /* Construct and initialize a new Camera State object */
    NEF_STATUS (*initialize)(struct nefko_camera_state **this);

    /* Destroy a Camera State object */
    NEF_STATUS (*destroy)(struct nefko_camera_state **this);

    /* Fill in the Cmeara State object using the given the MakerNote IFD and
     * the model data bytes.
     */
    NEF_STATUS (*read_model_data)(struct nefko_camera_state *this,
                                  tiff_ifd_t *makernote,
                                  void *model_data, size_t model_data_bytes);

    NEF_STATUS (*get_image_attribs)(struct nefko_camera_state *this,
                                    unsigned *shutter_frac,         /* 1/shutter_frac */
                                    float *aperture);               /* f-stops */
} nefko_camera_t;

#endif /* __INCLUDE_NEFKO_CAMERA_H__ */

