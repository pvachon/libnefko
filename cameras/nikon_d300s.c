#include <nefko.h>
#include <nefko_camera.h>

static
NEF_STATUS d300s_initialize(struct nefko_camera_state **this)
{
    return NEF_OK;
}

static
NEF_STATUS d300s_destroy(struct nefko_camera_state **this)
{
    return NEF_OK;
}

static
NEF_STATUS d300s_read_model_data(struct nefko_camera_state *this,
                                 tiff_ifd_t *makernote,
                                 void *model_data, size_t model_data_bytes)
{
    return NEF_OK;
}

static
NEF_STATUS d300s_get_image_attribs(struct nefko_camera_state *this,
                                   unsigned *shutter_frac,         /* 1/shutter_frac */
                                   float *aperture)
{
    return NEF_OK;
}

struct nefko_camera d300s_camera_methods = {
    .name = "NIKON D300S",
    .initialize = d300s_initialize,
    .destroy = d300s_destroy,
    .read_model_data = d300s_read_model_data,
    .get_image_attribs = d300s_get_image_attribs
};

