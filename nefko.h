#ifndef __INCLUDE_NEFKO_H__
#define __INCLUDE_NEFKO_H__

/* Public Declarations for libnefko - a NEF image reader */


/* Opaque structure used for storing state of an NEF image */
struct nef;
typedef struct nef nef_t;

/* NEF images contained within an NEF file */
struct nef_image;
typedef struct nef_image nef_image_t;

/* NEF image types */
#define NEF_IMAGE_REDUCED   0x0
#define NEF_IMAGE_FULL      0x1

/* NEF pixel data types */
#define NEF_PIXEL_8         0x0 /* 8 bit samples */
#define NEF_PIXEL_16        0x1 /* 16 bit samples */

/* Error handling defines and types */
typedef int NEF_STATUS;

/* Error return values */
#define NEF_OK              0x0     /* Successful completion */
#define NEF_NO_MEMORY       0x1     /* Out of memory (allocation failed) */
#define NEF_NOT_NEF         0x2     /* Not an NEF image */
#define NEF_NOT_FOUND       0x3     /* Specified file not found */
#define NEF_BAD_ARGUMENT    0x4     /* Argument is NULL or invalid */
#define NEF_RANGE_ERROR     0x5     /* Value is out of range */
#define NEF_FAILURE         0x6     /* An unspecified failure occurred */

/* Some error codes, like the following, are "internal" to libnefko. */
#define NEF_INT_ERROR(x)            ((x) | 0x80000000ul)
#define NEF_MN_WRONG_SIZE   NEF_INT_ERROR(1)    /* Makernote data is wrong size for model */

/* Image sample formats */
#define NEF_DATATYPE_UNKNOWN        0 /* unknown, probably not an image IFD */
#define NEF_DATATYPE_UINT           1 /* Unsigned integer */

/* Open an NEF image */
NEF_STATUS nef_open(const char *file, nef_t **fp);

/* Close an NEF image */
NEF_STATUS nef_close(nef_t *fp);

/*******************************************************************/
/* Functions for manipulating NEF image descriptors                */
/*******************************************************************/

/* Get the count of images in an NEF file. Typically 3 or 4 */
NEF_STATUS nef_image_get_count(nef_t *fp, int *count);

/* Get a particular image, by count */
NEF_STATUS nef_image_get_handle(nef_t *fp, int id, nef_image_t **hdl);

/* Get the attributes of a particular image */
NEF_STATUS nef_image_get_attribs(nef_t *fp, nef_image_t *hdl,
                                 int *width, int *height, int *chans,
                                 int *image_type, int *data_type);

/*******************************************************************/
/* Functions for manipulating NEF image metadata                   */
/*******************************************************************/

/* Get the contents of an EXIF tag. Note that calls to this function
 * where data = NULL will just result in the type and tag_count being
 * populated with no extra data being read from the file.
 */
NEF_STATUS nef_exif_get_tag(nef_t *fp, int tag_id,
                            int *tag_type, int *tag_count, void *data);

/* Get the device model.
 */
NEF_STATUS nef_meta_get_model(nef_t *fp, int *count, char *model);

/* Get the white balance coefficient array, of floats.
 */
NEF_STATUS nef_meta_white_balance(nef_t *fp, int *count, float *coeffs);

/*******************************************************************/
/* Functions for decompressing NEF image data                      */
/*******************************************************************/

/* Get the image data contents of a given image represented by an
 * nef_image_t handle. Populates image with a width * height * chans
 * array, chunky pixels (if relevant), with each pixel occupying
 * data_type bytes.
 */
NEF_STATUS nef_image_get_raw(nef_t *fp, nef_image_t *hdl, unsigned bufsize,
                             void *image_buf);

#endif /* __INCLUDE_NEFKO_H__ */

