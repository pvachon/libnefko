#include <nefko.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    nef_t *nfp = NULL;
    nef_image_t *img = NULL;
    char buf[1024];

    if (argc < 2) {
        fprintf(stderr, "no filename provided\n");
        exit(-1);
    }

    printf("Opening '%s'\n", argv[1]);
    nef_open(argv[1], &nfp);

    nef_image_get_handle(nfp, 2, &img);

    nef_image_get_raw(nfp, img, 1024, buf);

    printf("Closing file...\n");
    nef_close(nfp);

    return 0;
}

