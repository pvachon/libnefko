#include <nefko.h>
#include <nefko_priv.h>

#include <stdlib.h>
#include <string.h>

/* A little helper bit iterator to assist with traversing buffers o'
 * bits
 */
struct biterator {
    uint8_t cached;

    uint8_t *buf_ptr;
    size_t buf_off; /* offset in buffer, in bytes */
    size_t bit_off; /* offset in current byte */
    size_t buf_max; /* maximum bytes in the buffer */
};

struct nef_npc_huff {
    int predictor[4];
    int edge_pred[2];
    struct nef_huff_leaf *root;
};

struct nef_huff_leaf *nef_new_huff_node()
{
    struct nef_huff_leaf *branch = NULL;

    branch = (struct nef_huff_leaf*)malloc(sizeof(struct nef_huff_leaf));

    if (branch != NULL) {
        memset(branch, 0, sizeof(struct nef_huff_leaf));
    }

    branch->leaf = 0xfffffffful;

    return branch;
}


NEF_STATUS nef_huff_append_node(struct nef_huff_leaf *root,
                                unsigned size,
                                unsigned value,
                                unsigned entrynum)
{
    unsigned code = value >> (8 - size);
    int i;

    struct nef_huff_leaf *branch = root;

    if (size == 0) return NEF_OK;

    for (i = 0; i < size; i++) {
        int dir = (code >> (size - i - 1)) & 1;

        if (branch->branch[dir] == NULL) {
            branch->branch[dir] = nef_new_huff_node();
        }

        branch = branch->branch[dir];
    }

    branch->leaf = entrynum;

    return NEF_OK;
}


static struct biterator *nef_npc_new_biterator(uint8_t *buffer,
                                              size_t byte_size)
{
    struct biterator *bit = NULL;

    bit = (struct biterator *)malloc(sizeof(struct biterator));

    if (bit == NULL) return NULL;

    bit->cached = *buffer;
    bit->buf_ptr = buffer;
    bit->buf_off = 0;
    bit->bit_off = 0;
    bit->buf_max = byte_size;

    return bit;
}

static inline int nef_npc_biterator_advance(struct biterator *bit)
{
    if (bit->bit_off == 8) {
        if (bit->buf_max == bit->buf_off + 1) {
            NEF_TRACE("reached the end of biterator. This is bad.");
            return -1;
        }
        bit->cached = *(++bit->buf_ptr);
        bit->bit_off = 0;
        bit->buf_off++;
    }

    return (bit->cached >> (8 - bit->bit_off++ - 1)) & 0x1;
}


int nef_npc_huff_get_value(struct nef_huff_leaf *root,
                           struct biterator *bit)
{
    struct nef_huff_leaf *node = root;
    int val;

    while (node->branch[0] != NULL || node->branch[1] != NULL) {
        int bit_val = nef_npc_biterator_advance(bit);
        if (bit_val == -1) {
            printf ("failed to get bit\n");
            return -33939;
        }

        node = node->branch[bit_val];

        if (node == NULL) {
            NEF_TRACE("Busted - got an unexpected bit");
            return -33939;
        }
    }

    val = node->leaf;

    if (val != 0) {
        uint8_t first = nef_npc_biterator_advance(bit);
        int i;
        int out = 0;

        out = first;

        for (i = 1; i < val; i++)
            out = (out << 1) | nef_npc_biterator_advance(bit);

        if (!first) {
            out -= (1 << val) - 1;
        }

        val = out;
    }

    return val;
}

static NEF_STATUS nef_npc_can_open(struct nef_image *image)
{
    uint32_t comp_type = 0;

    NEF_CHECK_ARG(image);

    /* Get the compression type */
    if (nef_get_tag_alloc(image->nef_file, image->ifd, TIFF_TAG_COMPRESSION,
                          (void *)&comp_type, NULL, NULL) != NEF_OK)
    {
        NEF_TRACE("Failed to get compression tag.\n");
        return NEF_NOT_FOUND;
    }

    if (comp_type != TIFF_COMPRESSION_NIKON) {
        NEF_TRACE("Compression type is not NIKON.\n");
        return NEF_FAILURE;
    }

    return NEF_OK;
}

static NEF_STATUS nef_npc_init_state(struct nef_image *image, tiff_ifd_t *makernote)
{
    return NEF_OK;
}

static NEF_STATUS nef_npc_read_image_tile(struct nef_image *image,
                                    unsigned x_off, unsigned y_off,
                                    unsigned w, unsigned h, void *buf)
{
    return NEF_OK;
}

static NEF_STATUS nef_npc_get_image_tile_size(struct nef_image *image,
                                              unsigned *w, unsigned *h)
{
    return NEF_OK;
}

static NEF_STATUS nef_npc_clean_up(struct nef_image *image)
{
    return NEF_OK;
}


struct nef_image_reader nef_huff = {
    .format_name = "NIKON Proprietary Compression",
    .can_open = nef_npc_can_open,
    .init_state = nef_npc_init_state,
    .read_image_tile = nef_npc_read_image_tile,
    .image_tile_size = nef_npc_get_image_tile_size,
    .clean_state = nef_npc_clean_up
};
