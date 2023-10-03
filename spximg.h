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

C89 header only solution to load and save image files easily.
It supports loading PNG, JPEG and PPM file  formats. Instead
of implenting PNG and JPEG from scratch, it uses libpng and 
libjpeg to build a simple and easy interoperability layer
across formats. It recognizes the image format by checking 
file name extension and then comparing the file's header 
bytes against each format specification.

****************************************************/

#include <stdint.h>

#ifndef IMG2D_TYPE_DEFINED
#define IMG2D_TYPE_DEFINED

typedef struct Img2D {
    uint8_t* pixbuf;
    int width;
    int height;
    int channels;
} Img2D;

#endif /* IMG2D_TYPE_DEFINED */

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
#define SPXI_FORMAT_BMP         5

#define SPXI_COLOR_UNKNOWN      0
#define SPXI_COLOR_GRAY         1
#define SPXI_COLOR_GRAY_ALPHA   2
#define SPXI_COLOR_RGB          3
#define SPXI_COLOR_RGBA         4

#define SPXI_BIT_DEPTH          8

#define SPXI_HEADER_SIZE        8

#ifndef SPXI_PADDING
#define SPXI_PADDING            0xFF
#endif /* SPXI_PADDING */

#if defined SPXI_ONLY_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_GIF
    #define SPXI_NO_PNM
    #define SPXI_NO_BMP
#elif defined SPXI_ONLY_JPEG
    #define SPXI_NO_PNG
    #define SPXI_NO_GIF
    #define SPXI_NO_PNM
    #define SPXI_NO_BMP
#elif defined SPXI_ONLY_GIF
    #define SPXI_NO_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_PNM
    #define SPXI_NO_BMP
#elif defined SPXI_ONLY_PNM
    #define SPXI_NO_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_GIF
    #define SPXI_NO_BMP
#elif defined SPXI_ONLY_BMP
    #define SPXI_NO_PNG
    #define SPXI_NO_JPEG
    #define SPXI_NO_GIF
    #define SPXI_NO_PNM
#endif /* SPXI_ONLY_FORMAT */

/* Parsing Name Extensions and File Headers */

#define spxParseHeaderPng(h) (!memcmp(h, "\211PNG\r\n\032\n", 8))
#define spxParseHeaderJpeg(h) (h[0] == 0xFF && h[1] == 0xD8 && h[2] == 0xFF)
#define spxParseHeaderGif(h) (!memcmp(h, "GIF87a", 6) || !memcmp(h, "GIF89a", 6))
#define spxParseHeaderPnm(h) ((h[0] == 0x50 && (h[1] > 0x30 || h[1] < 0x37) &&\
                                isspace(h[2])))
#define spxParseHeaderBmp(h) (h[0] == 0x42 && h[1] == 0x4D)

static int spxStrcmpLower(const char* s1, const char* s2)
{
    while (*s1 && *s2) {
        if (tolower(*s1++) != *s2++) {
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
        } else if (spxStrcmpLower(ext, "pnm") || spxStrcmpLower(ext, "ppm") ||
                    spxStrcmpLower(ext, "pgm") || spxStrcmpLower(ext, "pbm")) {
            return SPXI_FORMAT_PNM;
        } else if (spxStrcmpLower(ext, "bmp")) {
            return SPXI_FORMAT_BMP;
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
    } else if (spxParseHeaderBmp(header)) {
        return SPXI_FORMAT_BMP;
    }

    return SPXI_FORMAT_UNKNOWN;
}

static int spxParseFormat(const char* path)
{
    int format;
    uint8_t header[SPXI_HEADER_SIZE];
    FILE* file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "spximg could not open file: '%s'\n", path);
        return SPXI_FORMAT_NULL;
    }

    fread(header, SPXI_HEADER_SIZE, sizeof(uint8_t), file);
    fclose(file);

    format = spxParseExtension(path);
    if ((format == SPXI_FORMAT_PNG && spxParseHeaderPng(header)) ||
        (format == SPXI_FORMAT_JPEG && spxParseHeaderJpeg(header)) ||
        (format == SPXI_FORMAT_GIF && spxParseHeaderGif(header)) ||
        (format == SPXI_FORMAT_PNM && spxParseHeaderPnm(header)) ||
        (format == SPXI_FORMAT_BMP && spxParseHeaderBmp(header))) {
        return format;
    }

    return spxParseHeader(header);
}

/* Image Reshape Implementation */

static Img2D spxImageReshape1to4(const Img2D img)
{
	Img2D ret;
	int i, size = img.width * img.height;
    assert(img.channels == 1);

	ret.channels = 4;
	ret.width = img.width;
	ret.height = img.height;
	ret.pixbuf = (uint8_t*)malloc(size << 2);

    for (i = 0; i < size; ++i) {
        uint8_t* dst = ret.pixbuf + (i << 2);
        dst[1] = img.pixbuf[i];
        dst[2] = img.pixbuf[i];
        dst[3] = img.pixbuf[i];
        dst[3] = SPXI_PADDING;
	}

	return ret;
}

static Img2D spxImageReshape2to4(const Img2D img)
{ 
	Img2D ret;
	int i, size = img.width * img.height;
    assert(img.channels == 2);

	ret.channels = 4;
	ret.width = img.width;
	ret.height = img.height;
	ret.pixbuf = (uint8_t*)malloc(size << 2);

    for (i = 0; i < size; ++i) {
        uint8_t* src = img.pixbuf + (i << 1);
        uint8_t* dst = ret.pixbuf + (i << 2);
        dst[0] = src[0];
        dst[1] = src[0];
        dst[2] = src[0];
        dst[3] = src[1];
	}

    return ret;
}

static Img2D spxImageReshape3to4(const Img2D img)
{
	Img2D ret;
	int i, size = img.width * img.height;
    assert(img.channels == 3);

	ret.channels = 4;
	ret.width = img.width;
	ret.height = img.height;
	ret.pixbuf = (uint8_t*)malloc(size << 2);

    for (i = 0; i < size; ++i) {
        uint8_t* src = img.pixbuf + i * 3;
        uint8_t* dst = ret.pixbuf + (i << 2);
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = SPXI_PADDING;
	}

	return ret;
}

static Img2D spxImageReshape4to3(const Img2D img)
{
    Img2D ret;
	int i, size = img.width * img.height;
    assert(img.channels == 4);

    ret.channels = 3;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size * 3);

    for (i = 0; i < size; ++i) {
        uint8_t* src = img.pixbuf + (i << 2);
        uint8_t* dst = ret.pixbuf + i * 3;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }

    return ret;
}

static Img2D spxImageReshape2to3(const Img2D img)
{ 
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 2);

    ret.channels = 3;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size * 3);

    for (i = 0; i < size; ++i) {
        uint8_t p = img.pixbuf[i << 1];
        uint8_t* dst = img.pixbuf + i * 3;
        dst[0] = p;
        dst[1] = p;
        dst[2] = p;
    }

    return ret;  
}

static Img2D spxImageReshape1to3(const Img2D img)
{
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 1);

    ret.channels = 3;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size * 3);

    for (i = 0; i < size; ++i) {
        uint8_t* dst = ret.pixbuf + i * 3;
        dst[0] = img.pixbuf[i];
        dst[1] = img.pixbuf[i];
        dst[2] = img.pixbuf[i];
    }

    return ret;
}

static Img2D spxImageReshape4to2(const Img2D img)
{
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 4);

    ret.channels = 2;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size << 1);

    for (i = 0; i < size; ++i) {
        uint8_t* src = img.pixbuf + (i << 2);
        uint8_t* dst = ret.pixbuf + (i << 1);
        dst[0] = (uint8_t)(((int)src[0] + (int)src[1] + (int)src[2]) / 3);
        dst[1] = src[4];
    }

    return ret;
}

static Img2D spxImageReshape3to2(const Img2D img)
{
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 3);

    ret.channels = 2;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size << 1);

    for (i = 0; i < size; ++i) {
        uint8_t* src = img.pixbuf + i * 3;
        uint8_t* dst = ret.pixbuf + (i << 1);
        dst[0] = (uint8_t)(((int)src[0] + (int)src[1] + (int)src[2]) / 3);
        dst[1] = SPXI_PADDING;
    }

    return ret;
}

static Img2D spxImageReshape1to2(const Img2D img)
{
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 1);

    ret.channels = 2;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size << 1);

    for (i = 0; i < size; ++i) {
        uint8_t* dst = ret.pixbuf + (i << 1);
        dst[0] = img.pixbuf[i];
        dst[1] = SPXI_PADDING;
    }

    return ret;
}

static Img2D spxImageReshape4or3to1(const Img2D img)
{
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 4 || img.channels == 3);

    ret.channels = 1;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size);

    for (i = 0; i < size; ++i) {
        uint8_t* src = img.pixbuf + i * img.channels;
        ret.pixbuf[i] = (uint8_t)(((int)src[0] + (int)src[1] + (int)src[2]) / 3);
    }

    return ret;
}

static Img2D spxImageReshape2to1(const Img2D img)
{
    Img2D ret;
    int i, size = img.width * img.height;
    assert(img.channels == 2);
    
    ret.channels = 1;
    ret.width = img.width;
    ret.height = img.height;
    ret.pixbuf = (uint8_t*)malloc(size);
    
    for (i = 0; i < size; ++i) {
        ret.pixbuf[i] = img.pixbuf[i << 1];
    }

    return ret;
}

static Img2D (*spxImageReshapeFunctions[4][4])(const Img2D) = {
    {&spxImageCopy, &spxImageReshape1to2, &spxImageReshape1to3, &spxImageReshape1to4},
    {&spxImageReshape2to1, &spxImageCopy, &spxImageReshape2to3, &spxImageReshape2to4},
    {&spxImageReshape4or3to1, &spxImageReshape3to2, &spxImageCopy, &spxImageReshape3to4},
    {&spxImageReshape4or3to1, &spxImageReshape4to2, &spxImageReshape4to3, &spxImageCopy}
};

Img2D spxImageReshape(const Img2D img, const int channels)
{
    Img2D ret = {NULL, 0, 0, 0};
    if (img.channels > 0 && img.channels <= 4 && channels > 0 && channels <= 4) {
        return spxImageReshapeFunctions[img.channels - 1][channels - 1](img);
    }

    fprintf(
        stderr, "spximg does not support reshape from %d to %d channels\n",
        img.channels, channels
    );

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
    
    return SPXI_COLOR_UNKNOWN;
}

static int spxPngChannelsToColorType(int channels)
{
    switch (channels) {
        case SPXI_COLOR_GRAY: return PNG_COLOR_TYPE_GRAY;
        case SPXI_COLOR_GRAY_ALPHA: return PNG_COLOR_TYPE_GRAY_ALPHA;
        case SPXI_COLOR_RGB: return PNG_COLOR_TYPE_RGB;
        case SPXI_COLOR_RGBA: return PNG_COLOR_TYPE_RGBA;
    }
    
    fprintf(stderr, "spximg does not support %d channels per pixel\n", channels);
    return -1;
}

Img2D spxImageLoadPng(const char* path)
{
    int i, stride;
    Img2D img = {NULL, 0, 0, 0};
    uint8_t **rows, bitDepth, colorType;
    png_structp png;
    png_infop info;
    
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "spximg could not open file: '%s'\n", path);
        return img;
    }
    
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "spximg could not create PNG read struct\n");
        return img;
    }

    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "spximg could not read image as PNG file: '%s'\n", path);
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

    stride = img.channels * img.width;
    assert(stride == (int)png_get_rowbytes(png, info));

    rows = (uint8_t**)malloc(img.height * sizeof(uint8_t*));
    img.pixbuf = (uint8_t*)malloc(img.height * stride);
    
    for (i = 0; i < img.height; i++) {
        rows[i] = img.pixbuf + i * stride;
    }

    png_read_image(png, rows);

    free(rows);
    fclose(file);
    return img;
}

int spxImageSavePng(const Img2D img, const char* path) 
{
    int i, stride;
    uint8_t **rows, colorType;
    png_structp png;
    png_infop info;

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "spximg could not write file: '%s'\n", path);
        return EXIT_FAILURE;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "spximg could not create PNG write struct\n");
        return EXIT_FAILURE;
    }

    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "spximg could not write image as PNG file: '%s'\n", path);
        return EXIT_FAILURE;
    }

    colorType = spxPngChannelsToColorType(img.channels);

    png_init_io(png, file);
    png_set_IHDR(
        png, info, img.width, img.height, SPXI_BIT_DEPTH, colorType, 
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );

    stride = img.width * img.channels;
    rows = (uint8_t**)malloc(img.height * sizeof(uint8_t*));
    
    for (i = 0; i < img.height; i++) {
        rows[i] = img.pixbuf + i * stride;
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
    uint8_t* fbuffer;
	size_t fsize, stride;
	Img2D img = {NULL, 0, 0, 0};
    
    struct jpeg_decompress_struct info;
	struct jpeg_error_mgr err;

	FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "spximg could not open file: '%s'\n", path);
        return img;
    } 

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fbuffer = (uint8_t*)malloc(fsize);
    
    fseek(file, 0, SEEK_SET);
	fread(fbuffer, fsize, sizeof(uint8_t), file);
	fclose(file);  

	info.err = jpeg_std_error(&err);	
	jpeg_create_decompress(&info);
	jpeg_mem_src(&info, fbuffer, fsize);

	if (jpeg_read_header(&info, 1) != 1) {
		fprintf(stderr, "spximg could not read image as JPEG file: '%s'\n", path);
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

    if (img.channels == 2 || img.channels == 4) {
        Img2D tmp = spxImageReshape(img, img.channels - 1);
        i = spxImageSaveJpeg(tmp, path, quality);
        spxImageFree(&tmp);
        return i;
    }

    assert(img.channels == 1 || img.channels == 3);
    info.err = jpeg_std_error(&err);
    jpeg_create_compress(&info);

    file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "spximg could not write image as JPEG file: '%s'\n", path);
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
#ifndef SPXI_NO_PNM

#define LINESIZE 256

static void spxImageLoadPbmNormalize(Img2D* img, int stride)
{
    int x, y, i, j, dif;
    stride <<= 3;
    dif = stride - img->width;
    i = img->height * img->width - 1;
    for (y = img->height - 1; y >= 0; --y) {
        j = (y + 1) * stride - dif - 1;
        for (x = img->width - 1; x >= 0; --x) {
            img->pixbuf[i] = !((img->pixbuf[j / 8] >> (7 - (j % 8))) & 0x01) * 0xFF;
            --i, --j;
        }
    }
}

static void spxImageLoadPnmNormalize(Img2D* img, const int bitdepth)
{
    int i;
    const int size = img->width * img->height * img->channels;
    const int bitsize = 1 + (bitdepth > 0xFF);
    
    if (bitsize == 2) {
        for (i = 0; i < size; ++i) {
            int n = *(((uint16_t*)img->pixbuf) + i);
            img->pixbuf[i] = (uint8_t)(0xFF * n / bitdepth);
        }
        img->pixbuf = realloc(img->pixbuf, size);
    } else {
        for (i = 0; i < size; ++i) {
            int n = img->pixbuf[i];
            img->pixbuf[i] = (uint8_t)(0xFF * n / bitdepth);
        }
    }
}

static Img2D spxImageLoadPnmBinary(FILE* file, const int width,
    const int height, const int channels, const int bitdepth)
{
    Img2D image;
    int bitsize = 1 + (bitdepth > 0xFF);
    size_t size = width * height * channels * bitsize;

    image.pixbuf = (uint8_t*)malloc(size);
    image.width = width;
    image.height = height;
    image.channels = channels;

    if (!bitdepth) {
        int i, stride = (width >> 3) + !!(width % 8);
        for (i = 0; i < image.height; ++i) {
            fread(image.pixbuf + i * stride, stride, sizeof(uint8_t), file);
        }
        spxImageLoadPbmNormalize(&image, stride);
    } else {
        fread(image.pixbuf, size, sizeof(uint8_t), file);
        if (bitdepth != 0xFF) {
            spxImageLoadPnmNormalize(&image, bitdepth);
        }
    }

    return image;
}

static Img2D spxImageLoadPnmASCII(FILE* file, char* line, 
    const int width, const int height, const int channels, const int bitdepth)
{
    static const char* div = " \t\n\r";
    
    Img2D image;
    uint8_t* end, *p;
    char *tok, *key = NULL;
    const size_t size = width * height * channels;

    image.pixbuf = (uint8_t*)malloc(size);
    image.width = width;
    image.height = height;
    image.channels = channels;
    p = image.pixbuf;

    for (end = p + size; p != end; ++p) {
        tok = strtok(key, div);
        key = NULL;
        if (!tok) {
            if (!(key = fgets(line, LINESIZE, file))) {
                spxImageFree(&image);
                return image;
            }
            --p;
        } else {
            *p = (uint8_t)(0xFF * atoi(tok) / bitdepth);
        }
    }

    return image;
}

static Img2D spxImageLoadPbmASCII(FILE* file, const int width, const int height)
{
    Img2D image;
    int c, i = 0;
    const size_t size = width * height;

    image.pixbuf = (uint8_t*)malloc(size);
    image.width = width;
    image.height = height;
    image.channels = 1;

    while ((c = fgetc(file)) != EOF) {
        if (c == '0' || c == '1') {
            image.pixbuf[i++] = 0xFF * (c == '0');
        }
    }

    return image;
}

Img2D spxImageLoadPnm(const char* path)
{
    static const char* div = " \t\n\r";
    
    int params[3] = {0}, paramsize, paramcount = 0, filepos = 0;
    char N, line[LINESIZE], *tok, *key = NULL;
    Img2D image = {NULL, 0, 0, 0};
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "spximg could not open file: %s\n", path);
        return image;
    }

    if (!fgets(line, LINESIZE, file)) {
        fprintf(stderr, "spximg could not parse file: %s\n", path);
        goto spxImageLoadPnmEnd;
    }

    if (line[0] != 0x50) {
        fprintf(stderr, "spximg: file is not PNM file format: %s\n", path);
        goto spxImageLoadPnmEnd;
    }
    
    N = line[1];
    if (N < '1' || N > '6') {
        fprintf(stderr, "spximg does not support this kind of PNM: %s: P%c\n", path, N);
        goto spxImageLoadPnmEnd;
    }

    paramsize = (N == '1' || N == '4') ? 2 : 3;
    tok = strtok(line, div);

    while (paramcount < paramsize) {
        tok = strtok(key, div);
        key = NULL;
        if (!tok || tok[0] == '#') {
            filepos = ftell(file);
            if (!(key = fgets(line, LINESIZE, file))) {
                fprintf(stderr, "spximg could not parse complete PNM in file: %s\n",
                    path
                );
                goto spxImageLoadPnmEnd;
            }
        } else if (!isdigit(tok[0])) {
            fprintf(stderr, "spximg detected invalid token in PNM header: %s: %s\n",
                path, tok
            );
            goto spxImageLoadPnmEnd;
        } else {
            params[paramcount++] = atoi(tok);
        }
    }

    for (paramcount = 0; paramcount < paramsize; ++paramcount) {
         if (!params[paramcount]) {
            fprintf( stderr, 
                "spximg detected illegal PNM with zero value in: %s\n", path
            );
            goto spxImageLoadPnmEnd;
        }
    }

    switch (N) {
        case '1':
            fseek(file, filepos + (tok - line) + strlen(tok) + 1, SEEK_SET);
            image = spxImageLoadPbmASCII(file, params[0], params[1]);
            break;
        case '2':
        case '3':
            image = spxImageLoadPnmASCII(
                file, line, params[0], params[1], (N == '3') ? 3 : 1, params[2]
            );
            break;
        default:
            fseek(file, filepos + (tok - line) + strlen(tok) + 1, SEEK_SET);
            image = spxImageLoadPnmBinary(
                file, params[0], params[1], (N == '6') ? 3 : 1, params[2]
            );
    }

    if (!image.pixbuf) {
        fprintf(stderr,
             "spximg detected incomplete or corrupted PNM file: %s\n", path
        );
    }

spxImageLoadPnmEnd:
    fclose(file);
    return image;
}

int spxImageSavePnm(const Img2D img, const char* path)
{
    FILE* file;
    
    if (img.channels != 3) {
        int ret;
        Img2D tmp = spxImageReshape(img, 3);
        ret = spxImageSavePnm(tmp, path);
        spxImageFree(&tmp);
        return ret;
    }

    file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "spximg could not write file: '%s'\n", path);
        return EXIT_FAILURE;
    }

    fprintf(file, "P6 %d %d 255\n", img.width, img.height);
    fwrite(img.pixbuf, img.width * img.height, img.channels, file);
    return fclose(file);
}

#endif /* SPXI_NO_PNM */
#ifndef SPXI_NO_BMP

Img2D spxImageLoadBmp(const char* path)
{
    FILE* file;
    uint16_t id;
    int dif, stride, rowsize;
    Img2D image = {NULL, 0, 0, 0};
    struct BmpHeader {
        uint32_t size;
        uint16_t reserved1, reserved2;
        uint32_t offset; 
        struct BmpDibHeader {
            uint32_t size;
            int32_t width, height;
            uint16_t planes, bpp;
            uint32_t compression, imgsize;
            int32_t res[2];
            uint32_t colors[2];
        } dib;
        char padding[256];
    } bmp;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "spximg could not open file: %s\n", path);
        return image;
    }

    if (!fread(&id, sizeof(id), 1, file)) {
        fprintf(stderr, "spximg could not parse file: %s\n", path);
        goto spxImageLoadBmpEnd;
    }

    if (id != 0x4D42) {
        fprintf(stderr, "spximg: file is not BMP format: %s\n", path);
        goto spxImageLoadBmpEnd;
    }

    if (!fread(&bmp, offsetof(struct BmpHeader, dib), 1, file)) {
        fprintf(stderr, "spximg could not parse file: %s\n", path);
        goto spxImageLoadBmpEnd;
    }

    if (!fread(&bmp.dib, sizeof(bmp.dib.size), 1, file)) {
        fprintf(stderr, "spximg: file is not BMP format: %s\n", path);
        goto spxImageLoadBmpEnd;
    }

    if (!fread(&bmp.dib.width, bmp.dib.size - sizeof(bmp.dib.size), 1, file)) {
        fprintf(stderr, "spximg could not parse file: %s\n", path);
        goto spxImageLoadBmpEnd;
    }

    if (bmp.dib.planes != 1 || bmp.dib.bpp == 0 || 
        ((bmp.dib.bpp != 32 && bmp.dib.bpp != 16) && bmp.dib.compression != 0)) {
        fprintf(stderr, "spximg does not support this kind of BMP file: %s\n", path);
        goto spxImageLoadBmpEnd;
    }

    rowsize = bmp.dib.width * bmp.dib.bpp;
    for (stride = (rowsize >> 3) + !!(rowsize % 8); stride % 4; ++stride);

    assert(ftell(file) == 14 + bmp.dib.size);

    image.width = bmp.dib.width;
    image.height = bmp.dib.height;

    if (bmp.dib.bpp <= 8 && (bmp.dib.compression == 0 || bmp.dib.compression == 3)) {
        /* remove scanline, read into pixelbuffer */
        uint8_t* palette, *scanline;
        struct bitmask { int r, g, b, a; } bitmask, offset = {0}, maskdif = {0};
        int x, y, i, j, div, colorcount, palette_size;
        dif = bmp.offset - ftell(file);
        palette_size = bmp.dib.colors[0] ? bmp.dib.colors[0] << 2 : dif;

        if (bmp.dib.size > 40) {
            bitmask = *(struct bitmask*)bmp.padding;
        } else {
            struct bitmask bm = {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000};
            bitmask = bm;
        }
        
        while (!((bitmask.r >> offset.r) & 0x01)) ++offset.r;
        while (!((bitmask.g >> offset.g) & 0x01)) ++offset.g;
        while (!((bitmask.b >> offset.b) & 0x01)) ++offset.b;
        while ((bitmask.r >> (offset.r + maskdif.r)) & 0x01) ++maskdif.r;
        while ((bitmask.g >> (offset.g + maskdif.g)) & 0x01) ++maskdif.g;
        while ((bitmask.b >> (offset.b + maskdif.b)) & 0x01) ++maskdif.b;

        maskdif.r = 8 - maskdif.r;
        maskdif.g = 8 - maskdif.g;
        maskdif.b = 8 - maskdif.b;

        colorcount = palette_size >> 2;
        if (bmp.dib.colors[0] > (1 << bmp.dib.bpp)) {
            fprintf(stderr,
                "spximg could not guess size of color pallete in BMP file: %s\n", path
            );
            goto spxImageLoadBmpEnd;
        }

        image.channels = 4;
        image.pixbuf = malloc(image.width * image.height * 4);
        palette = (uint8_t*)malloc(palette_size);
        scanline = (uint8_t*)malloc(stride);
        fread(palette, palette_size, 1, file);

#if 1   /* FILL ALPHA WITH 0xFF FOR EASY DEBUGGING */
        for (i = 0; i < colorcount; ++i) {
            palette[i * 4 + 3] = 0xFF;
        }
#endif

        dif = bmp.offset - ftell(file);
        if (dif) {
            fseek(file, dif, SEEK_CUR);
        }

        div = 8 / bmp.dib.bpp;
        for (y = 0; y < image.height; ++y) {
            i = (image.height - y - 1) * image.width * 4;
            fread(scanline, stride, 1, file);
            for (x = 0; x < image.width; ++x) {
                int ibyte, ibit = x % div, n = 0;
                ibyte = x / div;
                ibit *= bmp.dib.bpp;
                for (j = 0; j < bmp.dib.bpp; ++j) {
                    int bit = (scanline[ibyte] >> ((ibit + j))) & 0x01;
                    n |= bit << j;
                }
 
                n = *(int*)(palette + (n << 2));
                image.pixbuf[i++] = ((n & bitmask.r) >> offset.r) << maskdif.r;
                image.pixbuf[i++] = ((n & bitmask.g) >> offset.g) << maskdif.g;
                image.pixbuf[i++] = ((n & bitmask.b) >> offset.b) << maskdif.b;
                image.pixbuf[i++] = 0xff;
            }
        }

        free(palette);
        free(scanline);
    } else if (bmp.dib.bpp == 24) {
        uint8_t* scanline;
        int x, y, i, n, linesize = image.width * 3;
        dif = bmp.offset - ftell(file);
        if (dif) {
            fseek(file, dif, SEEK_CUR);
        }

        image.channels = 3;
        image.pixbuf = (uint8_t*)malloc(image.height * linesize);
        scanline = (uint8_t*)malloc(stride);
        
        for (y = image.height - 1; y >= 0; --y) {
            fread(scanline, stride, 1, file);
            n = 0, i = y * linesize;
            for (x = 0; x < image.width; ++x) {
                image.pixbuf[i++] = scanline[n++ + 2];
                image.pixbuf[i++] = scanline[n++];
                image.pixbuf[i++] = scanline[n++ - 2];
            }
        }

        free(scanline);
    } else if (bmp.dib.bpp == 32 && bmp.dib.compression == 3) {
        int x, y, i, n;
        struct bitmask {
            int r, g, b, a;
        } bitmask, offset = {0, 0, 0, 0};

        bitmask = *(struct bitmask*)bmp.padding;
        while (!((bitmask.r >> offset.r) & 1)) ++offset.r;
        while (!((bitmask.g >> offset.g) & 1)) ++offset.g;
        while (!((bitmask.b >> offset.b) & 1)) ++offset.b;
        while (!((bitmask.a >> offset.a) & 1)) ++offset.a;

        dif = bmp.offset - ftell(file);
        if (dif) {
            fseek(file, dif, SEEK_CUR);
        }

        image.channels = 4;
        image.pixbuf = (uint8_t*)malloc(stride * image.height);

        for (y = image.height - 1; y >= 0; --y) {
            i = y * stride;
            fread(image.pixbuf + i, stride, 1, file);
            for (x = 0; x < image.width; ++x) {
                n = *(int*)(image.pixbuf + i);
                image.pixbuf[i++] = (n & bitmask.r) >> offset.r;
                image.pixbuf[i++] = (n & bitmask.g) >> offset.g;
                image.pixbuf[i++] = (n & bitmask.b) >> offset.b;
                image.pixbuf[i++] = (n & bitmask.a) >> offset.a;
            }
        }
    } else if (bmp.dib.bpp == 32 && bmp.dib.compression == 0) {
        int x, y, i, n;
        struct bitmask {
            int r, g, b, a;
        } bitmask = {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000}, offset = {0};
        while (!((bitmask.r >> offset.r) & 1)) ++offset.r;
        while (!((bitmask.g >> offset.g) & 1)) ++offset.g;
        while (!((bitmask.b >> offset.b) & 1)) ++offset.b;
        while (!((bitmask.a >> offset.a) & 1)) ++offset.a;

        dif = bmp.offset - ftell(file);
        if (dif) {
            fseek(file, dif, SEEK_CUR);
        }

        image.channels = 4;
        image.pixbuf = (uint8_t*)malloc(stride * image.height);

        for (y = image.height - 1; y >= 0; --y) {
            i = y * stride;
            fread(image.pixbuf + i, stride, 1, file);
            for (x = 0; x < image.width; ++x) {
                n = *(int*)(image.pixbuf + i);
                image.pixbuf[i++] = (n & bitmask.r) >> offset.r;
                image.pixbuf[i++] = (n & bitmask.g) >> offset.g;
                image.pixbuf[i++] = (n & bitmask.b) >> offset.b;
                image.pixbuf[i++] = (n & bitmask.a) >> offset.a;
            }
        }
    } else if (bmp.dib.bpp == 16 && (!bmp.dib.compression || bmp.dib.compression == 3)) {
        uint8_t* scanline;
        int x, y, i, linesize = 4 * image.width;
        struct bitmask {
            uint32_t r, g, b;
        } bitmask, maskdif = {0}, offset = {0};

        dif = bmp.offset - ftell(file);
        if (dif) {
            fread(&bitmask, dif, 1, file);
        } else {
            bitmask = *(struct bitmask*)bmp.padding;
        }
        if (!bmp.dib.compression) {
            bitmask.r = 0x7c00;
            bitmask.g = 0x03e0;
            bitmask.b = 0x001f;
        }

        while (!((bitmask.r >> offset.r) & 0x01)) ++offset.r;
        while (!((bitmask.g >> offset.g) & 0x01)) ++offset.g;
        while (!((bitmask.b >> offset.b) & 0x01)) ++offset.b;
        while ((bitmask.r >> (offset.r + maskdif.r)) & 0x01) ++maskdif.r;
        while ((bitmask.g >> (offset.g + maskdif.g)) & 0x01) ++maskdif.g;
        while ((bitmask.b >> (offset.b + maskdif.b)) & 0x01) ++maskdif.b;

        maskdif.r = 8 - maskdif.r;
        maskdif.g = 8 - maskdif.g;
        maskdif.b = 8 - maskdif.b;

        image.channels = 4;
        image.pixbuf = (uint8_t*)malloc(linesize * image.height);
        scanline = (uint8_t*)malloc(stride);

        for (y = image.height - 1; y >= 0; --y) {
            i = y * linesize;
            fread(scanline, stride, 1, file);
            for (x = 0; x < image.width; ++x) {
                uint16_t n = *(uint16_t*)(scanline + (x << 1));
                image.pixbuf[i++] = ((n & bitmask.r) >> offset.r) << maskdif.r;
                image.pixbuf[i++] = ((n & bitmask.g) >> offset.g) << maskdif.g;
                image.pixbuf[i++] = ((n & bitmask.b) >> offset.b) << maskdif.b;
                image.pixbuf[i++] = 0xff;
            }
        }

        free(scanline);
    } else {
        fprintf(stderr, "spximg is not ready to parse this kind of BMP yet: %s\n",
            path
        );
        goto spxImageLoadBmpEnd;
    }

spxImageLoadBmpEnd:
    fclose(file);
    return image;
}

#endif /* SPXI_NO_BMP */

/* Generic Saving and Loading */

Img2D spxImageLoad(const char* path)
{
    int format;
    Img2D image = {NULL, 0, 0, 0};
    
    format = spxParseFormat(path);
    switch (format) {
        case SPXI_FORMAT_PNG: image = spxImageLoadPng(path); break;
        case SPXI_FORMAT_JPEG: image = spxImageLoadJpeg(path); break;
        case SPXI_FORMAT_PNM: image = spxImageLoadPnm(path); break;
        case SPXI_FORMAT_BMP: image = spxImageLoadBmp(path); break;
        case SPXI_FORMAT_UNKNOWN: 
            fprintf(stderr, "spximg could not recognize format: %s\n", path);
    }

    return image;
}

int spxImageSave(const Img2D image, const char* path)
{
    switch (spxParseExtension(path)) {
        case SPXI_FORMAT_PNG: return spxImageSavePng(image, path);
        case SPXI_FORMAT_JPEG: return spxImageSaveJpeg(image, path, SPXI_JPEG_QUALITY);
        case SPXI_FORMAT_PNM: return spxImageSavePnm(image, path);
    }

    fprintf(stderr, "spximg only supports saving images as PNG, JPEG and PPM\n");
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
    image.pixbuf = (uint8_t*)malloc(size);
    memset(image.pixbuf, SPXI_PADDING, size);
    return image;
}

Img2D spxImageCopy(const Img2D img)
{
    Img2D image;
    size_t size = img.width * img.height * img.channels;
    
    image.width = img.width;
    image.height = img.height;
    image.channels = img.channels;
    image.pixbuf = (uint8_t*)malloc(size);
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

