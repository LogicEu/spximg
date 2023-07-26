#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>
#include <png.h>

Img2D spxImageLoadPng(const char* path)
{
    int i;
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
    
    img.width = png_get_image_width(png, info);
    img.height = png_get_image_height(png, info);

    colorType = png_get_color_type(png, info);
    bitDepth = png_get_bit_depth(png, info);

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
    }

    if (colorType == PNG_COLOR_TYPE_RGB ||
        colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    img.channels = 4;
    png_read_update_info(png, info);
    rows = (png_bytep*)malloc(img.height * sizeof(png_bytep));
    img.pixbuf = (uint8_t*)malloc(img.height * img.width * img.channels);
    for (i = 0; i < img.height; i++) {
        rows[i] = img.pixbuf + i * img.width * img.channels;
    }

    png_read_image(png, rows);

    free(rows);
    fclose(file);
    return img;
}

void spxImageSavePng(const Img2D img, const char* path) 
{
    int i, channels;
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

    channels = img.channels - 1;
    switch (channels) {
        case 1: channels = 3; break;
        case 3: channels = 6; break;
    }

    png_init_io(png, file);
    png_set_IHDR(
        png, info, img.width, img.height, 8, channels, 
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );

    rows = (png_bytep*)malloc(img.height * sizeof(png_bytep));
    for (i = 0; i < img.height; i++) {
        rows[i] = img.pixbuf + i * img.width * img.channels;
    }

    png_write_info(png, info);
    png_write_image(png, rows);
    
    png_write_end(png, NULL);
    free(rows);
    fclose(file);
}

Img2D spxImageReshape4to3(const Img2D img, const int channels)
{
    int x, y;
    Img2D ret;
    ret.width = img.width;
    ret.height = img.height;
    ret.channels = channels;
    printf("%d\n", img.height);
    printf("HEIGHT: %d\n", ret.height);
    ret.pixbuf = (uint8_t*)malloc(img.width * img.height * channels);
    for (int y = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            memcpy(
                ret.pixbuf + (img.width * y + x) * channels,
                img.pixbuf + (img.width * y + x) * img.channels,
                channels
            );
        }
    }
    printf("HEIGHT: %d\n", ret.height);
    return ret;
}

int main(const int argc, const char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "%s: missing input argument\n", argv[0]);
        return 1;
    }

    Img2D image = spxImageLoadPng(argv[1]), img;
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

    img = spxImageCreate(400, 300, 3);
    spxImageSavePng(img, "bmp.png");

    if (image.channels == 4) {
        Img2D tmp = spxImageReshape4to3(image, 3);
        spxImageFree(&image);
        image = tmp;
        printf("out\n");
    }

    spxImageSave(image, "image.ppm");
    spxImageFree(&image);
    return 0;
}
