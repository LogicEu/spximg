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

int spxImageColorTypeToChannels(int channels)
{
    switch (channels) {
        case PNG_COLOR_TYPE_GRAY: return SPXI_COLOR_GRAY;
        case PNG_COLOR_TYPE_GRAY_ALPHA: return SPXI_COLOR_GRAY_ALPHA;
        case PNG_COLOR_TYPE_RGB: return SPXI_COLOR_RGB;
        case PNG_COLOR_TYPE_RGBA: return SPXI_COLOR_RGBA;
        case PNG_COLOR_TYPE_PALETTE: return SPXI_COLOR_RGBA;
    }
    
    fprintf(stderr, "PNG does not support %d number of channels per pixel.\n", channels);
    return EXIT_FAILURE;
}

int spxImageChannelsToColorType(int channels)
{
    switch (channels) {
        case SPXI_COLOR_GRAY: return PNG_COLOR_TYPE_GRAY;
        case SPXI_COLOR_GRAY_ALPHA: return PNG_COLOR_TYPE_GRAY_ALPHA;
        case SPXI_COLOR_RGB: return PNG_COLOR_TYPE_RGB;
        case SPXI_COLOR_RGBA: return PNG_COLOR_TYPE_RGBA;
    }
    
    fprintf(stderr, "PNG does not support %d number of channels per pixel.\n", channels);
    return EXIT_FAILURE;
}

Img2D spxImageLoadPng(const char* path)
{
    int i;
    size_t rowSize;
    Img2D img = {NULL, 0, 0, 0};
    png_byte bitDepth, colorType;
    png_structp png;
    png_infop info;
    png_bytep* rows;
    
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "spximg could not findfile: '%s'\n", path);
        return img;
    }
    
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "spximg could not read PNG file: '%s'\n", path);
        return img;
    }

    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "spximg detected a problem with the PNG file '%s'\n", path);
        return img;
    }

    png_init_io(png, file);
    png_read_info(png, info);
    
    colorType = png_get_color_type(png, info);
    bitDepth = png_get_bit_depth(png, info);

    img.width = png_get_image_width(png, info);
    img.height = png_get_image_height(png, info);
    img.channels = spxImageColorTypeToChannels(colorType);

    fprintf(stdout, "PNG Color Type Before: %s\n", spxImageColorTypeString(colorType));

    if (bitDepth == 16) {
        png_set_strip_16(png);
    }

    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        ++img.channels;
    }

    png_read_update_info(png, info);
    colorType = png_get_color_type(png, info);
    fprintf(stdout, "PNG Color Type After: %s\n", spxImageColorTypeString(colorType));

    rowSize = img.channels * img.width;
    assert(rowSize == png_get_rowbytes(png, info));

    rows = (png_bytep*)malloc(img.height * sizeof(png_bytep));
    img.pixbuf = (uint8_t*)malloc(img.height * rowSize);
    
    for (i = 0; i < img.height; i++) {
        rows[i] = img.pixbuf + i * rowSize;
    }

    png_read_image(png, rows);

    free(rows);
    fclose(file);
    return img;
}

void spxImageSavePng(const Img2D img, const char* path) 
{
    int i, colorType;
    png_infop info;
    png_structp png;
    png_bytep* rows;

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "spximg could not write file: '%s'\n", path);
        return;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "spximg had a problem creating PNG struct '%s'\n", path);
        return;
    }

    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "spximg detected a problem saving PNG file: '%s'\n", path);
        return;
    }

    colorType = spxImageChannelsToColorType(img.channels);

    png_init_io(png, file);
    png_set_IHDR(
        png, info, img.width, img.height, SPXI_BIT_DEPTH, colorType, 
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );

    rows = (png_bytep*)malloc(img.height * sizeof(png_bytep));
    for (i = 0; i < img.height; i++) {
        rows[i] = img.pixbuf + i * img.width * img.channels;
    }

    png_write_info(png, info);
    png_write_image(png, rows);
    
    png_write_end(png, NULL);
    fclose(file);
    free(rows);
}

static Img2D spxImageReshape4to3(const Img2D img)
{
    Img2D ret;
    int i, inRowSize = img.width * img.channels, outRowSize = img.width * 3;
    assert(img.channels == 4);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 3;
    ret.pixbuf = (uint8_t*)malloc(img.height * outRowSize);

    for (i = 0; i < img.height; ++i) {
        memcpy(
            ret.pixbuf + i * outRowSize,
            img.pixbuf + i * inRowSize,
            3
        );
    }

    return ret;
}

static Img2D spxImageReshape4or3to1(const Img2D img)
{
    Img2D ret;
    int i, x, y;
    assert(img.channels == 4 || img.channels == 3);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 1;
    ret.pixbuf = (uint8_t*)malloc(img.height * img.width);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            uint8_t* px = img.pixbuf + i * img.channels;
            ret.pixbuf[i++] = (px[0] + px[1] + px[2]) / 3;
        }
    }

    return ret;
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

    img = spxImageCreate(400, 300, 2);
    spxImageSavePng(img, "images/bmp.png");

    if (image.channels == 4) {
        Img2D tmp = spxImageReshape4to3(image, 3);
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
