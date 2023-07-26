#define SPXI_APPLICATION
#include <spximg.h>
#include <stdio.h>

const char* spxFormatName(int format)
{
    static const char* table[] = {"Unknown", "PNG", "JPEG", "GIF", "PPM"};
    return table[format > 4 ? 0 : format];
}

int main(const int argc, const char** argv)
{
    Img2D image;

    if (argc < 2) {
        fprintf(stderr, "%s: missing input argument\n", argv[0]);
        return 1;
    }

    image = spxImageLoad(argv[1]);
    if (!image.pixbuf) {
        fprintf(stderr, "%s: could not load file: %s\n", argv[0], argv[1]);
        return 2;
    }
    
    fprintf(
        stdout, 
        "file: '%s'\nwidth: %d\nheight: %d\nchannels: %d\n",
        argv[1],
        image.width,
        image.height,
        image.channels
    );

    spxImageFree(&image);
    return 0;
}
