#include <nefko.h>
#include <nefko_priv.h>
#include <nefko_priv_tags.h>

#include <ghetto.h>

#include <stdio.h>

NEF_STATUS nef_image_get_count(nef_t *fp, int *count)
{
    NEF_CHECK_ARG(fp);
    NEF_CHECK_ARG(count);

    *count = fp->image_count;

    return NEF_OK;
}

NEF_STATUS nef_image_get_handle(nef_t *fp, int id, nef_image_t **hdl)
{
    NEF_CHECK_ARG(fp);
    NEF_CHECK_ARG(hdl);

    *hdl = NULL;

    if (id >= fp->image_count) {
        NEF_TRACE("Invalid image specified: %d\n", id);
        return NEF_RANGE_ERROR;
    }

    *hdl = &fp->images[id];

    return NEF_OK;
}

NEF_STATUS nef_image_get_attribs(nef_t *fp, nef_image_t *hdl,
                                 int *width, int *height, int *chans,
                                 int *image_type, int *data_type)
{
    NEF_CHECK_ARG(fp);
    NEF_CHECK_ARG(hdl);

    if (width) {
        *width = hdl->width;
    }

    if (height) {
        *height = hdl->height;
    }

    if (chans) {
        *chans = hdl->chans;
    }

    if (image_type) {
        *image_type = hdl->type;
    }

    if (data_type) {
        *data_type = hdl->data_type;
    }

    return NEF_OK;
}

/* Decompress a TIFF_COMPRESSION_NIKON type compressed image */
static NEF_STATUS nef_image_get_nikon_raw(nef_t *fp, nef_image_t *hdl,
                                          unsigned bufsize, void *image_buf)
{
    unsigned *stripoffsets = NULL, *stripbytecounts = NULL;
    int type, count, count_off, count_cnts, i;
    NEF_STATUS ret = NEF_OK;

#if _DUMP_IMAGE_DATA
    char *fname = NULL;
    FILE *ffp = NULL;
#endif

    if (nef_get_tag_alloc(fp, hdl->ifd, TIFF_TAG_STRIPBYTECOUNTS,
                          (void**)&stripbytecounts, &type, &count_off) != NEF_OK)
    {
        NEF_TRACE("Failed to get StripByteCounts tag!\n");
        return NEF_NOT_FOUND;
    }

    NEF_TRACE("Got %d strips (type = %d)!\n", count, type);
    count = count_off;

    if (nef_get_tag_alloc(fp, hdl->ifd, TIFF_TAG_STRIPOFFSETS,
                          (void**)&stripoffsets, &type, &count_cnts) != NEF_OK)
    {
        NEF_TRACE("Failed to get StripOffsets tag!\n");
        free(stripbytecounts);
        return NEF_NOT_FOUND;
    }

    if (count_off != count_cnts) {
        NEF_TRACE("StripByteCounts count is not equal to StripOffsets!\n");
        ret = NEF_RANGE_ERROR;
        goto exit;
    }

#if _DUMP_IMAGE_DATA
    /* Load the data and dump it to a temporary file... */
    asprintf(&fname, "%dX%d.dat", hdl->width, hdl->height);
    ffp = fopen(fname, "w+");
    free(fname);
#endif

    for (i = 0; i < count; i++) {
        uint8_t *buf = (uint8_t *)malloc(stripbytecounts[i]);
        size_t count_read = 0;

        if (buf == NULL) {
            ret = NEF_NO_MEMORY;
            goto exit;
        }

        NEF_TRACE("Reading strip %d (offset = %08x count = %u)\n",
            i, stripoffsets[i], stripbytecounts[i]);

        if (tiff_read(fp->tiff_fp, stripoffsets[i], stripbytecounts[i], 1,
                      buf, &count_read) != TIFF_OK)
        {
            NEF_TRACE("Failed to read %u bytes.\n",
                stripbytecounts[i]);
            ret = NEF_RANGE_ERROR;
            free(buf);
            goto exit;
        }

        NEF_TRACE("Read %zd bytes\n", count_read);

#if _DUMP_IMAGE_DATA
        fwrite(buf, stripbytecounts[i], 1, ffp);
#endif

        free(buf);
    }

#if _DUMP_IMAGE_DATA
    fclose(ffp);
#endif

exit:
    free(stripbytecounts);
    free(stripoffsets);

    return ret;
}

NEF_STATUS nef_image_get_raw(nef_t *fp, nef_image_t *hdl, unsigned bufsize,
                             void *image_buf)
{
    NEF_CHECK_ARG(fp);
    NEF_CHECK_ARG(hdl);
    NEF_CHECK_ARG(image_buf);

    NEF_TRACE("Loading unformatted image data...\n");

    nef_image_get_nikon_raw(fp, hdl, bufsize, image_buf);

    return NEF_OK;
}

