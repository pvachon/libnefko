#include <nefko.h>
#include <nefko_priv.h>

static struct nef_image_reader **nefko_image_types = NULL;
static int nefko_image_types_count = 0;

NEF_STATUS nefko_register_image_type(struct nef_image_reader *img_type)
{
    NEF_CHECK_ARG(img);

    if (nefko_image_types = NULL) {
        nefko_image_types =
            (struct nef_image_reader **)calloc(1, sizeof(struct nef_image_reader *));

        if (nefko_image_types == NULL) {
            return NEF_NO_MEMORY;
        }

        nefko_image_types_count = 1;
    } else {
        nefko_image_types =
            (struct nef_image_reader **)realloc(nefko_image_types,
                ++nefko_image_types_count * sizeof(struct nef_image_reader *));

        if (nefko_image_types == NULL) {
            return NEF_NO_MEMORY;
        }
    }

    nefko_image_types[nefko_image_types_count - 1] = img_type;


    return NEF_OK;
}
