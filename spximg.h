/*

Copyright (c) 2023 Eugenio Arteaga A.

Permission is hereby granted, free of charge, to any 
person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the 
Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice 
shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef SIMPLE_PIXEL_IMAGE_H
#define SIMPLE_PIXEL_IMAGE_H

/******************
***** spximg ******
 Simple PiXel Image
*******************
@Eugenio Arteaga A.
*******************

C89 header only solution to handle, load and save
image files. Supports loading PNG, JPEG, PNM and 
GIF file formats.

****************************************************/

#include <stdint.h>

typedef struct Img2D {
    uint8_t* pixbuf;
    int width;
    int height;
    int channels;
} Img2D;

Img2D spxImageCreate(int width, int height, int channels);
Img2D spxImageLoad(const char* path);
Img2D spxImageCopy(const Img2D img);
Img2D spxImageReshape(const Img2D img, int channels);
int spxImageSave(const Img2D image, const char* path);
void spxImageFree(Img2D* image);

#ifdef SPXI_APPLICATION

/******************
***** spximg ******
 Simple PiXel Image
*******************
* IMPLEMENTATION  *
*******************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* Core Simple Pixel Image Functions */

#define SPXI_FORMAT_NULL        (-1)
#define SPXI_FORMAT_UNKNOWN     0
#define SPXI_FORMAT_PNG         1
#define SPXI_FORMAT_JPEG        2
#define SPXI_FORMAT_GIF         3
#define SPXI_FORMAT_PNM         4

#define SPXI_COLOR_UNKNOWN      0
#define SPXI_COLOR_GRAY         1
#define SPXI_COLOR_GRAY_ALPHA   2
#define SPXI_COLOR_RGB          3
#define SPXI_COLOR_RGBA         4

#define SPXI_BIT_DEPTH          8

#ifndef SPXI_PADDING
#define SPXI_PADDING 0xFF
#endif /* SPXI_PADDING */

#if defined SPXI_ONLY_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_GIF
    #define SPXI_NO_PNM
#elif defined SPXI_ONLY_JPEG
    #define SPXI_NO_PNG
    #define SPXI_NO_GIF
    #define SPXI_NO_PNM
#elif defined SPXI_ONLY_GIF
    #define SPXI_NO_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_PNM
#elif defined SPXI_ONLY_PNM
    #define SPXI_NO_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_GIF
#endif /* SPXI_ONLY_FORMAT */

/* Parsing Name Extensions and File Headers */

#define SPXI_HEADER_SIZE 8

#define spxParseHeaderPng(h) (!memcmp(h, "\211PNG\r\n\032\n", 8))
#define spxParseHeaderJpeg(h) (h[0] == 0xFF && h[1] == 0xD8 && h[2] == 0xFF)
#define spxParseHeaderGif(h) (!memcmp(h, "GIF87a", 6) || !memcmp(h, "GIF89a", 6))
#define spxParseHeaderPnm(h) ((char)h[0] == 'P' && (char)h[1] >= '1' && (char)h[1] <= '6')

static int spxStrcmpLower(const char* s1, const char* s2)
{
    int i;
    for (i = 0; s1[i]; ++i) {
        const int c = tolower(s1[i]);
        if (c != s2[i]) {
            return 0;
        }
    }
    return 1;
}

static int spxParseExtension(const char* path)
{
    size_t i, dotpos = 0;
    for (i = 0; path[i]; ++i) {
        if (path[i] == '.') {
            dotpos = i;
        }
    }

    if (dotpos) {
        const char* ext = path + dotpos + 1;
        if (spxStrcmpLower(ext, "png")) {
            return SPXI_FORMAT_PNG;
        } else if (spxStrcmpLower(ext, "jpeg") || spxStrcmpLower(ext, "jpg")) {
            return SPXI_FORMAT_JPEG;
        } else if (spxStrcmpLower(ext, "gif")) {
            return SPXI_FORMAT_GIF;
        } else if (spxStrcmpLower(ext, "ppm")) {
            return SPXI_FORMAT_PNM;
        }
    }

    return SPXI_FORMAT_UNKNOWN;
}

static int spxParseHeader(const uint8_t* header)
{
    if (spxParseHeaderPng(header)) {
        return SPXI_FORMAT_PNG;
    } else if (spxParseHeaderJpeg(header)) {
        return SPXI_FORMAT_JPEG;
    } else if (spxParseHeaderGif(header)) {
        return SPXI_FORMAT_GIF;
    } else if (spxParseHeaderPnm(header)) {
        return SPXI_FORMAT_PNM;
    }

    return SPXI_FORMAT_UNKNOWN;
}


static int spxParseFormat(const char* path)
{
    int format;
    uint8_t header[SPXI_HEADER_SIZE];
    FILE* file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "spximg could not open file: %s\n", path);
        return SPXI_FORMAT_NULL;
    }

    fread(header, SPXI_HEADER_SIZE, sizeof(uint8_t), file);
    fclose(file);
    
    format = spxParseExtension(path);
    if ((format == SPXI_FORMAT_PNG && spxParseHeaderPng(header)) ||
        (format == SPXI_FORMAT_JPEG && spxParseHeaderJpeg(header)) ||
        (format == SPXI_FORMAT_GIF && spxParseHeaderGif(header)) ||
        (format == SPXI_FORMAT_PNM && spxParseHeaderPnm(header))) {
        return format;
    }

    return spxParseHeader(header);
}

/* Image Reshape Implementation */

static Img2D spxImageReshape4to3(const Img2D img)
{
    Img2D ret;
    int i, inStride = img.width * 4, outStride = img.width * 3;
    assert(img.channels == 4);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 3;
    ret.pixbuf = (uint8_t*)malloc(img.height * outStride);

    for (i = 0; i < img.height; ++i) {
        memcpy(
            ret.pixbuf + i * outStride,
            img.pixbuf + i * inStride,
            3
        );
    }

    return ret;
}

static Img2D spxImageReshape2to3(const Img2D img)
{ 
    Img2D ret;
    uint8_t* px, p;
    int i, j, x, y, stride = img.width * 3;
    assert(img.channels == 1);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 3;
    ret.pixbuf = (uint8_t*)malloc(img.height * stride);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            j = i << 1;
            p = (uint8_t)((int)img.pixbuf[j] * (int)img.pixbuf[j + 1] / 255);
            px = ret.pixbuf + i++ * stride;
            px[0] = p;
            px[1] = p;
            px[2] = p;
        }
    }

    return ret;  
}

static Img2D spxImageReshape1to3(const Img2D img)
{
    Img2D ret;
    int i, x, y, stride = img.width * 3;
    assert(img.channels == 1);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 3;
    ret.pixbuf = (uint8_t*)malloc(img.height * stride);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            uint8_t* px = ret.pixbuf + i++ * stride;
            px[0] = img.pixbuf[i];
            px[1] = img.pixbuf[i];
            px[2] = img.pixbuf[i];
        }
    }

    return ret;
}

static Img2D spxImageReshape4to2(const Img2D img)
{
    Img2D ret;
    int i, j, x, y;
    assert(img.channels == 4);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 2;
    ret.pixbuf = (uint8_t*)malloc(img.height * img.width << 1);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            uint8_t* px = img.pixbuf + i * img.channels;
            j = i << 1;
            ret.pixbuf[j] = (px[0] + px[1] + px[2]) / 3;
            ret.pixbuf[j + 1] = px[4];
            ++i;
        }
    }

    return ret;
}

static Img2D spxImageReshape3to2(const Img2D img)
{
    Img2D ret;
    int i, j, x, y;
    assert(img.channels == 3);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 2;
    ret.pixbuf = (uint8_t*)malloc(img.height * img.width << 1);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            uint8_t* px = img.pixbuf + i * img.channels;
            j = i << 1;
            ret.pixbuf[j] = (px[0] + px[1] + px[2]) / 3;
            ret.pixbuf[j + 1] = SPXI_PADDING;
            ++i;
        }
    }

    return ret;
}

static Img2D spxImageReshape1to2(const Img2D img)
{
    Img2D ret;
    int i, j, x, y;
    assert(img.channels == 1);

    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 2;
    ret.pixbuf = (uint8_t*)malloc(img.height * img.width << 1);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x) {
            j = i << 1;
            ret.pixbuf[j] = img.pixbuf[i++];
            ret.pixbuf[j + 1] = SPXI_PADDING;
        }
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

static Img2D spxImageReshape2to1(const Img2D img)
{
    Img2D ret;
    int i, x, y;
    const int stride = img.width * img.channels;
    
    assert(img.channels == 2);
    ret.width = img.width;
    ret.height = img.height;
    ret.channels = 1;
    ret.pixbuf = (uint8_t*)malloc(img.width * img.height);

    for (y = 0, i = 0; y < img.height; ++y) {
        for (x = 0; x < img.width; ++x, ++i) {
            ret.pixbuf[i * img.width] = img.pixbuf[i * stride];
        }
    }

    return ret;
}

Img2D spxImageReshape(const Img2D img, const int channels)
{
    Img2D ret = {NULL, 0, 0, 0};

    assert(img.channels > 0 && img.channels <= 4);
    if (img.channels == channels) {
        return spxImageCopy(img);
    }

    switch (channels) {
    case 1:
        switch (img.channels) {
        case 2: ret = spxImageReshape2to1(img); break;
        case 3: 
        case 4: ret = spxImageReshape4or3to1(img);
        }
        break;
    case 2:
        switch (img.channels) {
        case 1: ret = spxImageReshape1to2(img); break;
        case 3: ret = spxImageReshape3to2(img); break;
        case 4: ret = spxImageReshape4to2(img); break;
        }
        break;
    case 3:
        switch (img.channels) {
        case 1: ret = spxImageReshape1to3(img); break;
        case 2: ret = spxImageReshape2to3(img); break;
        case 4: ret = spxImageReshape4to3(img); break;
        }
        break;
    case 4:
        switch (img.channels) {
        case 1: ret = spxImageReshape1to4(img); break;
        case 2: ret = spxImageReshape2to4(img); break;
        case 3: ret = spxImageReshape3to4(img); break;
        }
        break;
    default:
        fprintf(stderr, "spximg cannot reshape to channel count: %d\n", channels);
    }

    return ret;
}

/* Image Formats Saver and Loaders */

#ifndef SPXI_NO_PNG
#include <png.h>

static int spxPngColorTypeToChannels(int channels)
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

static int spxPngChannelsToColorType(int channels)
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
    img.channels = spxPngColorTypeToChannels(colorType);

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

int spxImageSavePng(const Img2D img, const char* path) 
{
    int i, colorType;
    png_infop info;
    png_structp png;
    png_bytep* rows;

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "spximg could not write file: '%s'\n", path);
        return EXIT_FAILURE;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "spximg had a problem creating PNG struct '%s'\n", path);
        return EXIT_FAILURE;
    }

    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "spximg detected a problem saving PNG file: '%s'\n", path);
        return EXIT_FAILURE;
    }

    colorType = spxPngChannelsToColorType(img.channels);

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
    free(rows);
    return fclose(file);
}

#endif /* SPXI_NO_PNG */
#ifndef SPXI_NO_JPEG

#include <jpeglib.h>

#ifndef SPXI_JPEG_QUALITY 
#define SPXI_JPEG_QUALITY 100
#endif /* SPXI_JPEG_QUALITY */

Img2D spxImageLoadJpeg(const char* path)
{
    int i;
    void* fbuffer;
	size_t fsize, stride;
	Img2D img = {NULL, 0, 0, 0};
    
    struct jpeg_decompress_struct info;
	struct jpeg_error_mgr err;

	FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "could not open file '%s'\n", path);
        return img;
    } 

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fbuffer = malloc(fsize);
    
    fseek(file, 0, SEEK_SET);
	fread(fbuffer, fsize, sizeof(uint8_t), file);
	fclose(file);  

	info.err = jpeg_std_error(&err);	
	jpeg_create_decompress(&info);
	jpeg_mem_src(&info, fbuffer, fsize);

	if (jpeg_read_header(&info, 1) != 1) {
		fprintf(stderr, "file '%s' is not a standard JPEG\n", path);
		return img;
	}

	jpeg_start_decompress(&info);

    img.width = info.output_width;
	img.height = info.output_height;
	img.channels = info.output_components;

	stride = img.width * img.channels;
	img.pixbuf = (uint8_t*)malloc(img.height * stride);

    for (i = 0; i < img.height; ++i) {
        uint8_t* rowptr = img.pixbuf + i * stride;
		jpeg_read_scanlines(&info, &rowptr, 1);
	}

	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	free(fbuffer);

    return img;
}

int spxImageSaveJpeg(const Img2D img, const char* path, const int quality) 
{
    FILE* file;
    int i, stride;
    struct jpeg_compress_struct info;
    struct jpeg_error_mgr err;

    /* REPLACE WITH spxImageReshape(); */
    if (img.channels == 2 || img.channels == 4) {
        int ret;
        Img2D tmp;
        
        switch (img.channels) {
            case 2: tmp = spxImageReshape2to1(img); break;
            case 4: tmp = spxImageReshape4to3(img); break;
        }
        
        ret = spxImageSaveJpeg(tmp, path, quality);
        spxImageFree(&tmp);
        return ret;
    }

    assert(img.channels == 1 || img.channels == 3);
    info.err = jpeg_std_error(&err);
    jpeg_create_compress(&info);

    file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "spximg could not write file '%s'\n", path);
        return EXIT_FAILURE;
    }

    jpeg_stdio_dest(&info, file);

    info.image_width = img.width;
    info.image_height = img.height;
    info.input_components = img.channels;
    info.in_color_space = img.channels == 1 ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults(&info);
    jpeg_set_quality(&info, quality, 1);
    jpeg_start_compress(&info, 1);

    stride = img.width * img.channels;
    for (i = 0; i < img.height; ++i) {
        uint8_t* rowptr = img.pixbuf + i * stride;
        jpeg_write_scanlines(&info, &rowptr, 1);
    }

    jpeg_finish_compress(&info);
    jpeg_destroy_compress(&info);
    return fclose(file);
}

#endif /* SPXI_NO_JPEG */
#ifndef SPXI_NO_GIF

#endif /* SPXI_NO_GIF */
#ifndef SPXI_NO_PNM

Img2D spxImageLoadPnm(const char* path)
{
    size_t size;
    char header[256];
    Img2D image = {NULL, 0, 0, 3};
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "file could not be opened: '%s'\n", path);
        return image;
    }

    fgets(header, 256, file);
    sscanf(header, "P6 %d %d 255", &image.width, &image.height);
    size = image.width * image.height * image.channels;
    image.pixbuf = (uint8_t*)malloc(size);
    fread(image.pixbuf, size, sizeof(uint8_t), file);
    
    fclose(file);
    return image;
}

int spxImageSavePnm(const Img2D img, const char* path)
{
    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "spximg could not open filw for writing: %s\n", path);
        return EXIT_FAILURE;
    }

    fprintf(file, "P6 %d %d 255\n", img.width, img.height);
    fwrite(img.pixbuf, img.width * img.height, img.channels, file);
    return fclose(file);
}

#endif /* SPXI_NO_PNM */

/* Generic Saving and Loading */

Img2D spxImageLoad(const char* path)
{
    int format;
    Img2D image = {NULL, 0, 0, 0};
    
    format = spxParseFormat(path);
    switch (format) {
        case SPXI_FORMAT_PNG: image = spxImageLoadPng(path); break;
        case SPXI_FORMAT_JPEG: image = spxImageLoadJpeg(path); break;
        /* case SPXI_FORMAT_GIF: image = spxImageLoadGif(path); break; */
        case SPXI_FORMAT_PNM: image = spxImageLoadPnm(path); break;
        case SPXI_FORMAT_UNKNOWN: fprintf(stderr, "Unknown format: %s\n", path);
    }

    return image;
}

int spxImageSave(const Img2D image, const char* path)
{
    switch (spxParseExtension(path)) {
        case SPXI_FORMAT_PNG: return spxImageSavePng(image, path);
        case SPXI_FORMAT_JPEG: return spxImageSaveJpeg(image, path, SPXI_JPEG_QUALITY);
        /* case SPXI_FORMAT_GIF: return spxImageSaveGif(image, path); */
        case SPXI_FORMAT_PNM: return spxImageSavePnm(image, path);
    }

    fprintf(stderr, "spximg only supports saving .png, .jpeg, .gif and .pnm files\n");
    return EXIT_FAILURE;
}

/* Basic Image Allocation and Deallocation Implementation */

Img2D spxImageCreate(int width, int height, int channels)
{
    size_t size;
    Img2D image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    size = width * height * channels;
    image.pixbuf = malloc(size);
    memset(image.pixbuf, SPXI_PADDING, size);
    return image;
}

Img2D spxImageCopy(const Img2D img)
{
    size_t size;
    Img2D image;
    image.width = img.width;
    image.height = img.height;
    image.channels = img.channels;
    size = img.width * img.height * img.channels;
    image.pixbuf = malloc(size);
    memcpy(image.pixbuf, img.pixbuf, size);
    return image;
}

void spxImageFree(Img2D* image)
{
    if (image->pixbuf) {
        free(image->pixbuf);
        image->pixbuf = NULL;
        image->width = 0;
        image->height = 0;
        image->channels = 0;
    }
}

#endif /* SPXI_APPLICATION */
#endif /* SIMPLE_PIXEL_IMAGE_H */

