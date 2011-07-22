#include <nefko.h>
#include <ghetto.h>
#include <nefko_priv.h>

#include <string.h>
#include <stdint.h>

/* Extendable test for whether or not this is an NEF file. Eventually,
 * this should also cover identifying whether or not this is a sub-
 * variant of NEF that is supported by libnefko.
 */
static NEF_STATUS nef_identify(tiff_t *fp, tiff_ifd_t *parent)
{
    tiff_tag_t *maker = NULL;
    char *maker_tag = NULL;
    int maker_count;
    NEF_STATUS ret;

    /* Attempt to retrieve the TIFF Maker tag */
    if (tiff_get_tag(fp, parent, TIFF_TAG_MAKER, &maker) != TIFF_OK) {
        NEF_TRACE("Failed to get Maker tag!\n");
        return NEF_NOT_NEF;
    }

    if (tiff_get_tag_info(fp, maker, NULL, NULL, &maker_count) != TIFF_OK) {
        NEF_TRACE("Failed to get attribs of Maker tag!\n");
        return NEF_NOT_NEF;
    }

    maker_tag = (char *)calloc(1, maker_count + 1);
    if (maker_tag == NULL) {
        return NEF_NO_MEMORY;
    }

    if (tiff_get_tag_data(fp, parent, maker, maker_tag) != TIFF_OK) {
        NEF_TRACE("Could not get Maker tag data.\n");
        ret = NEF_NOT_NEF;
        goto fail;
    }

    NEF_TRACE("Got Maker tag \"%s\"\n", maker_tag);

    if (strncmp(maker_tag, "NIKON CORPORATION", 18)) {
        NEF_TRACE("Vendor not NIKON, aborting\n");
        ret = NEF_NOT_NEF;
        goto fail;
    }

    free(maker_tag);

    return NEF_OK;

fail:
    if (maker_tag) free(maker_tag);

    return ret;
}

NEF_STATUS nef_get_tag_low(nef_t *nef, tiff_ifd_t *ifd, unsigned tag_id,
                           void *dest)
{
    tiff_tag_t *taginfo = NULL;
    GHETTO_CHECK(tiff_get_tag(nef->tiff_fp, ifd, tag_id, &taginfo));

    if (taginfo == NULL) {
        return NEF_NOT_FOUND;
    }

    GHETTO_CHECK(tiff_get_tag_data(nef->tiff_fp, ifd, taginfo, dest));

    return NEF_OK;
}

NEF_STATUS nef_get_tag_alloc(nef_t *nef, tiff_ifd_t *ifd, unsigned tag_id,
                             void **dest, int *item_type, int *item_count)
{
    tiff_tag_t *taginfo = NULL;
    int count, type;
    void *dest_ptr = NULL;
    size_t tysz;
    NEF_STATUS stat;

    NEF_CHECK_ARG(dest);
    NEF_CHECK_ARG(item_type);
    NEF_CHECK_ARG(item_count);

    *dest = NULL;
    *item_type = 0;
    *item_count = 0;

    GHETTO_CHECK(tiff_get_tag(nef->tiff_fp, ifd, tag_id, &taginfo));

    if (taginfo == NULL) {
        return NEF_NOT_FOUND;
    }

    GHETTO_CHECK(tiff_get_tag_info(nef->tiff_fp, taginfo,
                                   NULL, &type, &count));

    if ( (tysz = tiff_get_type_size(type)) == 0 ) {
        return NEF_RANGE_ERROR;
    }

    dest_ptr = calloc(tysz, count);

    if (dest_ptr == NULL) {
        return NEF_NO_MEMORY;
    }

    if ((stat = tiff_get_tag_data(nef->tiff_fp, ifd, taginfo, dest_ptr)) != NEF_OK)
    {
        NEF_TRACE("Failed to get tag data.\n");
        free(dest_ptr);
        return stat;
    }

    *dest = dest_ptr;
    *item_type = type;
    *item_count = count;

    return NEF_OK;
}

NEF_STATUS nef_get_tag(nef_t *nef, nef_image_t *img, unsigned tag_id,
                       void *dest)
{
    return nef_get_tag_low(nef, img->ifd, tag_id, dest);
}

#ifdef _DEBUG
static char *nef_data_types[] = {
        "unknown",
        "uint",
        "int",
        "float",
        "void",
        "complex int",
        "complex float"
    };
#endif

static NEF_STATUS nef_populate_image_info(nef_t *nef, nef_image_t *img)
{
    unsigned int type = 0;

    NEFKO_CHECK(nef_get_tag(nef, img, TIFF_TAG_IMAGELENGTH, &(img->height)),
        NEF_NOT_FOUND);
    NEFKO_CHECK(nef_get_tag(nef, img, TIFF_TAG_IMAGEWIDTH, &(img->width)),
        NEF_NOT_FOUND);
    NEFKO_CHECK(nef_get_tag(nef, img, TIFF_TAG_SAMPLESPERPIXEL, &(img->chans)),
        NEF_NOT_FOUND);

    img->data_type = NEF_DATATYPE_UINT;

    NEFKO_CHECK(nef_get_tag(nef, img, TIFF_TAG_NEWSUBFILETYPE, &type),
        NEF_NOT_FOUND);

    NEFKO_CHECK(nef_get_tag(nef, img, TIFF_TAG_COMPRESSION, &(img->compression)),
        NEF_NOT_FOUND);

    if (type & 0x1) {
        img->type = NEF_IMAGE_REDUCED;
    } else if (type == 0) {
        img->type = NEF_IMAGE_FULL;
    } else {
        NEF_TRACE("Unknown NewSubfileType: %08x\n", type);
    }

    if (nef_get_tag(nef, img, TIFF_TAG_PHOTOMETRICINTERP, &(img->photo_interp)) !=
        NEF_OK)
    {
        NEF_TRACE("Assuming planar Photometric Interpretation\n");
        img->photo_interp = 0;
    }

    NEF_TRACE("Image: %d x %d, %d channels (data type: '%s') (%s) Comp: 0x%08x PhInterp: %08x\n",
        img->width, img->height, img->chans, nef_data_types[img->data_type],
        img->type == NEF_IMAGE_REDUCED ? "thumbnail" : "full-resolution",
        (unsigned)img->compression, img->photo_interp);

    return NEF_OK;
}

static NEF_STATUS nef_find_images(nef_t *nef, tiff_t *fp, tiff_ifd_t *root)
{
    tiff_tag_t *subifds = NULL;
    uint32_t *subifd_offs = NULL;
    int count, type, i;
    NEF_STATUS ret;

    /* The algorithm for gathering images associated with an NEF file
       is simplistic - the root IFD will have a SubIFDs tag. Each SubIFD
       entry, as well as the root entry, will contain one of the
       associated images - two thumbnails, 1 full-resolution image. */

    if (tiff_get_tag(fp, root, TIFF_TAG_SUBIFDS, &subifds) != TIFF_OK ) {
        NEF_TRACE("Couldn't find SubIFDs tag!\n");
        return NEF_NOT_NEF;
    }

    if (tiff_get_tag_info(fp, subifds, NULL, &type, &count) != TIFF_OK) {
        NEF_TRACE("Couldn't get SubIFDs tag info.\n");
        return NEF_NOT_NEF;
    }

    subifd_offs = (uint32_t *)calloc(1, tiff_get_type_size(type) * count);

    if (tiff_get_tag_data(fp, root, subifds, subifd_offs) != TIFF_OK) {
        goto fail_free_offs;
    }

    /* Allocate an array for storing each image, including the image stored
     * in the root IFD.
     */
    nef->images = (nef_image_t *)calloc(1, sizeof(nef_image_t) * (count + 1));
    if (nef->images == NULL) {
        NEF_TRACE("Failed to allocate %zd bytes for image data\n",
            sizeof(nef_image_t) * (count + 1));
        ret = NEF_NO_MEMORY;
        goto fail_free_offs;
    }

    nef->images[0].ifd = root;

    /* Populate images[0] with the contents of the root IFD */
    if (nef_populate_image_info(nef, &nef->images[0]) != NEF_OK) {
        NEF_TRACE("Failed to populate root image data!\n");
        ret = NEF_NOT_NEF;
        goto fail_free_images;
    }

    /* Populate the remaining images with the contents of the SubIFDs */
    for (i = 1; i < count + 1; i++) {
        tiff_ifd_t *sub_ifd = NULL;
        if (tiff_read_ifd(fp, subifd_offs[i - 1], &sub_ifd) != TIFF_OK) {
            goto fail_free_images;
        }
        nef->images[i].ifd = sub_ifd;

        if (nef_populate_image_info(nef, &nef->images[i]) != NEF_OK) {
            NEF_TRACE("Failed to populate IFD %d's attributes\n", i);
            tiff_free_ifd(fp, sub_ifd);
            nef->images[i].ifd = NULL;
            continue;
        }
    }

    nef->image_count = count + 1;

    return NEF_OK;

fail_free_images:
    for (i = 0; i < count; i++) {
        if (nef->images[i].ifd != NULL) {
            tiff_free_ifd(fp, nef->images[i].ifd);
        }
    }
    if (nef->images) free(nef->images);
    nef->images = NULL;

fail_free_offs:
    if (subifd_offs) free(subifd_offs);

    return NEF_NOT_NEF;
}

static NEF_STATUS nef_destroy_image(nef_t *nef, nef_image_t *img)
{
    if (tiff_free_ifd(nef->tiff_fp, img->ifd) != TIFF_OK) {
        return NEF_FAILURE;
    }

    memset(img, 0, sizeof(nef_image_t));

    return NEF_OK;
}

NEF_STATUS nef_open(const char *file, nef_t **fp)
{
    tiff_t *tiff_fp = NULL;
    tiff_ifd_t *root_ifd = NULL;
    tiff_off_t root_ifd_off, makernote_off, exif_off;
    tiff_tag_t *exif_off_tag = NULL, *makernote_off_tag = NULL;
    TIFF_STATUS ret = TIFF_OK;
    NEF_STATUS nret = NEF_OK;

    int maker_count = 0, maker_type = 0, maker_type_size = 0;
    uint8_t *maker_buf = NULL;

    nef_t *nef_fp = NULL;

    NEF_CHECK_ARG(file);
    NEF_CHECK_ARG(fp);

    if (*file == '\0') {
        return NEF_BAD_ARGUMENT;
    }

    *fp = NULL;

    /* Open the NEF file */
    if ( (ret = tiff_open(&tiff_fp, file, "r")) != TIFF_OK ) {
        NEF_TRACE("Failed to open file '%s'\n", file);
        if (ret == TIFF_NOT_TIFF) {
            return NEF_NOT_NEF;
        } else {
            return NEF_NOT_FOUND;
        }
    }

    if ( (ret = tiff_get_base_ifd_offset(tiff_fp, &root_ifd_off)) != TIFF_OK ) {
        nret = NEF_NOT_NEF;
        goto fail_close_file;
    }

    if ( (ret = tiff_read_ifd(tiff_fp, root_ifd_off, &root_ifd)) != TIFF_OK ) {
        nret = NEF_NOT_NEF;
        goto fail_close_file;
    }

    if (nef_identify(tiff_fp, root_ifd) != NEF_OK) {
        nret = NEF_NOT_NEF;
        NEF_TRACE("This is not an NEF file...\n");
        goto fail_close_file;
    }

    /* This is likely a NEF file. Open the IFDs and store them. */
    nef_fp = (nef_t *)calloc(1, sizeof(nef_t));

    if (nef_fp == NULL) {
        nret = NEF_NO_MEMORY;
        goto fail_close_file;
    }

    nef_fp->tiff_fp = tiff_fp;

    if (nef_find_images(nef_fp, tiff_fp, root_ifd) != NEF_OK) {
        nret = NEF_NOT_NEF;
        goto fail_free_fptr;
    }

    /* Load the EXIF IFD */
    if (tiff_get_tag(nef_fp->tiff_fp, root_ifd, TIFF_TAG_EXIFIFD, &exif_off_tag)
        != TIFF_OK)
    {
        NEF_TRACE("Could not get EXIF IFD.\n");
        nret = NEF_NOT_NEF;
        goto fail_free_fptr;
    }

    if (tiff_get_raw_tag_field(nef_fp->tiff_fp, exif_off_tag, &exif_off) != TIFF_OK)
    {
        NEF_TRACE("Could not get offset to EXIF IFD.\n");
        nret = NEF_NOT_NEF;
        goto fail_free_fptr;
    }

    if (tiff_read_ifd(nef_fp->tiff_fp, exif_off, &nef_fp->exif) != TIFF_OK) {
        NEF_TRACE("Could not read the EXIF IFD.\n");
        nret = NEF_NOT_NEF;
        goto fail_free_fptr;
    }

    /* Load the MakerNote IFD */
    if (tiff_get_tag(nef_fp->tiff_fp, nef_fp->exif, TIFF_TAG_EXIF_MAKERNOTE,
                     &makernote_off_tag) != TIFF_OK)
    {
        NEF_TRACE("Unable to find MakerNote tag\n");
        nret = NEF_NOT_NEF;
        goto fail_free_exif;
    }

    if (tiff_get_raw_tag_field(nef_fp->tiff_fp, makernote_off_tag, &makernote_off))
    {
        NEF_TRACE("Failed to get offset of MakerNote IFD.\n");
        nret = NEF_NOT_NEF;
        goto fail_free_exif;
    }

    if (tiff_get_tag_info(nef_fp->tiff_fp, makernote_off_tag,
                          NULL, &maker_type, &maker_count) != TIFF_OK)
    {
        NEF_TRACE("Failed to get MakerNote tag info.\n");
        nret = NEF_NOT_NEF;
        goto fail_free_exif;
    }

    if ( (maker_type_size = tiff_get_type_size(maker_type)) == 0 ) {
        NEF_TRACE("Unexpected data type for MakerNote type.\n");
        nret = NEF_NOT_NEF;
        goto fail_free_exif;
    }

    maker_buf = (uint8_t *)calloc(maker_type_size, maker_count);

    if (maker_buf == NULL) {
        NEF_TRACE("Failed to allocate memory. Aborting.\n");
        nret = NEF_NO_MEMORY;
        goto fail_free_exif;
    }

    if (tiff_get_tag_data(nef_fp->tiff_fp, nef_fp->exif, makernote_off_tag,
                          maker_buf) != TIFF_OK)
    {
        NEF_TRACE("Failed to get tag data.\n");
        nret = NEF_NOT_NEF;
        free(maker_buf);
        goto fail_free_exif;
    }

    if (strncmp("Nikon", (char *)maker_buf, 5)) {
        NEF_TRACE("This is not a Nikon MakerNote... aborting.\n");
        nret = NEF_NOT_NEF;
        free(maker_buf);
        goto fail_free_exif;
    }

    /* Create an IFD for the makernote */
    if (tiff_make_ifd(nef_fp->tiff_fp, maker_buf + NEF_MAKERNOTE_OFF,
            maker_type_size * maker_count, makernote_off + NEF_MAKERNOTE_OFF - 8,
            &nef_fp->makernote) != TIFF_OK)
    {
        NEF_TRACE("Failed to create NEF MakerNote structure. Aborting.\n");
        nret = NEF_NOT_NEF;
        free(maker_buf);
        goto fail_free_exif;
    }

    free(maker_buf);

    if (nef_get_obfuscation_params(nef_fp) != NEF_OK) {
        NEF_TRACE("Failed to get crypto params\n");
        nret = NEF_NOT_NEF;
        goto fail_free_makernote;
    }

    *fp = nef_fp;

    return NEF_OK;

fail_free_makernote:
    if (nef_fp->makernote) tiff_free_ifd(tiff_fp, nef_fp->makernote);

fail_free_exif:
    if (nef_fp->exif) tiff_free_ifd(tiff_fp, nef_fp->exif);

fail_free_fptr:
    if (nef_fp) free(nef_fp);

fail_close_file:
    if (root_ifd) tiff_free_ifd(tiff_fp, root_ifd);
    if (tiff_fp) tiff_close(tiff_fp);
    return nret;
}

NEF_STATUS nef_close(nef_t *fp)
{
    int i = 0;

    NEF_CHECK_ARG(fp);

    if (fp->image_count > 0 && fp->images != NULL) {
        for (i = 0; i < fp->image_count; i++) {
            nef_destroy_image(fp, &fp->images[i]);
        }
        free(fp->images);
    }

    if (fp->exif) tiff_free_ifd(fp->tiff_fp, fp->exif);
    if (fp->makernote) tiff_free_ifd(fp->tiff_fp, fp->makernote);

    tiff_close(fp->tiff_fp);

    memset(fp, 0, sizeof(nef_t));
    free(fp);
    return NEF_OK;
}

