#ifndef __INCLUDE_NEFKO_PRIV_H__
#define __INCLUDE_NEFKO_PRIV_H__

#include <nefko.h>
#include <nefko_priv_tags.h>
#include <ghetto.h>

#include <stdio.h>
#include <stdint.h>

struct nef {
    tiff_t *tiff_fp;

    nef_image_t *images;

    tiff_ifd_t *makernote;
    tiff_ifd_t *exif;

    /* Parameters for deobfuscating various MakerNote params. */
    uint8_t key;
    uint8_t iv;

    size_t image_count;
};

struct nef_image_reader;

struct nef_image {
    tiff_ifd_t *ifd;
    unsigned type;
    unsigned width;
    unsigned height;
    unsigned chans;
    unsigned data_type;

    unsigned compression;
    unsigned photo_interp;

    struct nef_image_reader *reader;
    void *reader_state;
};

struct nef_image_reader {
    /* A human-readable name for the image type */
    const char *format_name;

    /* Determine if the given nef_image is an image type supported by this
     * nef_image_reader */
    int (*can_open)(struct nef_image *image);

    /* Initialize nef_image's reader_state member from the IFD contents */
    void (*init_state)(struct nef_image *image, tiff_ifd_t *makernote);

    /* Read a tile of imagery */
    void (*read_image_tile)(struct nef_image *image,
        unsigned x_off, unsigned y_off, unsigned w, unsigned h,
        void *buf);

    /* Get the tile size for use when reading */
    void (*image_tile_size)(struct nef_image *image);

    /* Destroy the reader_state in the given nef_image */
    void (*clean_state)(struct nef_image *image);
};

#ifdef _DEBUG
#define NEF_TRACE(x, ...) \
    printf("%s:%d - " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define NEF_TRACE(...)
#endif

#define NEF_CHECK_ARG(x) \
    if (!(x)) { \
        NEF_TRACE("argument is NULL: " # x "\n"); \
        return NEF_BAD_ARGUMENT; \
    }

#define _CHECK_RETURN(x, r, ret) \
    if ((x) != (r)) { \
        NEF_TRACE("error: " #x " did not return " #r "\n"); \
        return (ret); \
    }

#define GHETTO_CHECK(x) \
    _CHECK_RETURN(x, TIFF_OK, NEF_BAD_ARGUMENT)

#define NEFKO_CHECK(x, ret) \
    _CHECK_RETURN(x, NEF_OK, ret)

#define NEF_MAKERNOTE_OFF       18

#define BYTE(dw, n) \
    (uint8_t)((((dw) >> (n * 8)) & 0xff))

/* Retrieve a tag from an IFD */
NEF_STATUS nef_get_tag(nef_t *nef, nef_image_t *img, unsigned tag_id,
                       void *dest);

/* Retrieve a tag from a specific IFD */
NEF_STATUS nef_get_tag_low(nef_t *nef, tiff_ifd_t *ifd, unsigned tag_id,
                           void *dest);

/* Get parameters for deobfuscating MakerNote contents */
NEF_STATUS nef_get_obfuscation_params(nef_t *nef);

/* Get a tag and alocate the buffer to store the tag data */
NEF_STATUS nef_get_tag_alloc(nef_t *nef, tiff_ifd_t *ifd, unsigned tag_id,
                             void **dest, int *item_type, int *item_count);

#endif /* __INCLUDE_NEFKO_PRIV_H__ */

