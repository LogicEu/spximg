#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <png.h>

const char* spxImageChannelsString(int colorType)
{
    switch (colorType) {
        case SPXI_COLOR_RGBA: return "RGBA";
        case SPXI_COLOR_RGB: return "RGB";
        case SPXI_COLOR_GRAY_ALPHA: return "GrayAlpha";
        case SPXI_COLOR_GRAY: return "Gray";                        
    }
    return "Unknown";
}

const char* spxImageColorTypeString(int colorType)
{
    switch (colorType) {
        case PNG_COLOR_TYPE_RGBA: return "RGBA";
        case PNG_COLOR_TYPE_RGB: return "RGB";
        case PNG_COLOR_TYPE_GRAY_ALPHA: return "GrayAlpha";
        case PNG_COLOR_TYPE_GRAY: return "Gray";                        
        case PNG_COLOR_TYPE_PALETTE: return "Palette";
    }
    return "Unknown";
}

int main(const int argc, const char** argv)
{
    Img2D image, img;
    if (argc < 2) {
        fprintf(stderr, "%s: missing input argument\n", argv[0]);
        return 1;
    }

    image = spxImageLoadPng(argv[1]);
    if (!image.pixbuf) {
        fprintf(stderr, "%s: could not load file: %s\n", argv[0], argv[1]);
        return 2;
    }

    fprintf(
        stdout, 
        "file: '%s'\nformat: %s\nwidth: %d\nheight: %d\nchannels: %d\n",
        argv[1],
        "PNG",
        image.width,
        image.height,
        image.channels
    );

    img = spxImageCreate(400, 300, 1);
    spxImageSavePng(img, "images/bmp.png");

    if (image.channels == 4) {
        Img2D tmp = spxImageReshape4to3(image);
        spxImageFree(&image);
        image = tmp;
    } else if (image.channels < 3) {
        fprintf(stderr, "Image channels is less than 3!\n");
        spxImageFree(&image);
        return EXIT_FAILURE;
    }

    spxImageSave(image, "images/image.ppm");
    spxImageFree(&image);
    return 0;
}
