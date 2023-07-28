#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>

const char* spxImageFormatName(int format)
{
    static const char* table[] = {"Unknown", "PNG", "JPEG", "GIF", "PPM"};
    return table[format > 4 ? 0 : format];
}

const char* spxImageColorName(int channels)
{

    static const char* table[] = {"Unknown", "Gray", "GrayAlpha", "RGB", "RGBA"};
    return table[(channels < 0 || channels > 4) ? 0 : channels];
}

int main(const int argc, const char** argv)
{
    int format;
    Img2D image;

    if (argc < 2) {
        fprintf(stderr, "%s: missing input argument\n", argv[0]);
        return EXIT_FAILURE;
    }

    format = spxParseFormat(argv[1]);
    if (format == SPXI_FORMAT_NULL) {
        fprintf(stderr, "%s: could not open file: %s\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    image = spxImageLoad(argv[1]);
    if (!image.pixbuf) {
        fprintf(stderr, "%s: could not interpret file: %s\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    fprintf(
        stdout, 
        "file: '%s'\nformat: %s\nwidth: %d\nheight: %d\nchannels: %d - '%s'\n",
        argv[1],
        spxImageFormatName(format),
        image.width,
        image.height,
        image.channels,
        spxImageColorName(image.channels)
    );
    
    spxImageFree(&image);
    return 0;
}
