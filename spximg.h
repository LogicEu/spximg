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
image files. Supports loading PNG, JPEG, PPM and 
GIF file formats.

****************************************************/

#include <stdint.h>

typedef struct Img2D {
    uint8_t* pixbuf;
    int channels;
    int width;
    int height;
} Img2D;

Img2D spxImageCreate(int width, int height, int channels);
Img2D spxImageLoad(const char* path);
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

#include <png.h>
#include <jpeglib.h>

/* Core Simple Pixel Image Functions */

#define SPXI_FORMAT_UNKNOWN 0
#define SPXI_FORMAT_PNG     1
#define SPXI_FORMAT_JPEG    2
#define SPXI_FORMAT_GIF     3
#define SPXI_FORMAT_PPM     4

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
            return SPXI_FORMAT_PPM;
        }
    }

    return SPXI_FORMAT_UNKNOWN;
}

static int spxParseHeaderPng(const uint8_t* header)
{
    return !memcmp(header, "\211PNG\r\n\032\n", 8);
}

static int spxParseHeaderJpeg(const uint8_t* header)
{
    return header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF;
}

static int spxParseHeaderGif(const uint8_t* header)
{
    return !memcmp(header, "GIF87a", 6) || !memcmp(header, "GIF89a", 6);
}

static int spxParseHeaderPpm(const uint8_t* header)
{
    return (char)header[0] == 'P' && (char)header[1] >= '1' && (char)header[1] <= '6';
}

static int spxParseHeader(const uint8_t* header)
{
    if (spxParseHeaderPng(header)) {
        return SPXI_FORMAT_PNG;
    } else if (spxParseHeaderJpeg(header)) {
        return SPXI_FORMAT_JPEG;
    } else if (spxParseHeaderGif(header)) {
        return SPXI_FORMAT_GIF;
    } else if (spxParseHeaderPpm(header)) {
        return SPXI_FORMAT_PPM;
    }

    return SPXI_FORMAT_UNKNOWN;
}

static int spxParseFormat(const char* path, const uint8_t* header)
{
    int format = spxParseExtension(path);
    if ((format == SPXI_FORMAT_PNG && spxParseHeaderPng(header)) ||
        (format == SPXI_FORMAT_JPEG && spxParseHeaderJpeg(header)) ||
        (format == SPXI_FORMAT_GIF && spxParseHeaderGif(header)) ||
        (format == SPXI_FORMAT_PPM && spxParseHeaderPpm(header))) {
        return format;
    }
    return spxParseHeader(header);
}

static Img2D spxImageLoadPpm(const char* path)
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

static int spxImageSavePpm(const Img2D img, const char* path)
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

static Img2D spxImageLoadGuess(const char* path, const uint8_t* header)
{
    Img2D image = {NULL, 0, 0, 0};

    switch (spxParseHeader(header)) {
        /*
        case SPXI_FORMAT_PNG: image = spxImageLoadPng(path); break;
        case SPXI_FORMAT_JPEG: image = spxImageLoadJpeg(path); break;
        case SPXI_FORMAT_GIF: image = spxImageLoadGif(path); break;
        */
        case SPXI_FORMAT_PPM: image = spxImageLoadPpm(path); break;
        default: fprintf(
            stderr, 
            "spximg could not recognize image format of file: '%s'\n", 
            path
        );
    }

    return image;
}

Img2D spxImageLoad(const char* path)
{
    int format;
    uint8_t header[8];
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        Img2D image = {NULL, 0, 0, 0};
        fprintf(stderr, "spximg could not open file: %s\n", path);
        return image;
    }

    fread(header, 8, sizeof(uint8_t), file);
    fclose(file);

    format = spxParseFormat(path, header);
    switch (format) {
        /*
        case SPXI_FORMAT_PNG: return spxImageLoadPng(path);
        case SPXI_FORMAT_JPEG: return spxImageLoadJpeg(path);
        case SPXI_FORMAT_GIF: return spxImageLoadGif(path);
        */
        case SPXI_FORMAT_PPM: return spxImageLoadPpm(path);
    }

    return spxImageLoadGuess(path, header);
}

Img2D spxImageCreate(int width, int height, int channels)
{
    size_t size;
    Img2D image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    size = width * height * channels;
    image.pixbuf = malloc(size);
    memset(image.pixbuf, 255, size);
    return image;
}

int spxImageSave(const Img2D image, const char* path)
{
    switch (spxParseExtension(path)) {
        /*
        case SPXI_FORMAT_PNG: return spxImageSavePng(image, path);
        case SPXI_FORMAT_JPEG: return spxImageSaveJpeg(image, path);
        case SPXI_FORMAT_GIF: return spxImageSaveGif(image, path);
        case SPXI_FORMAT_PPM: return spxImageSavePpm(image, path);
        */
        case SPXI_FORMAT_PPM: return spxImageSavePpm(image, path); break;
    }

    fprintf(stderr, "spximg only supports saving .png, .jpeg, .gif and .ppm files\n");
    return EXIT_FAILURE;
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

