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

#include <spxe.h>
#include <stdint.h>

typedef struct Img2D {
    uint8_t* pixbuf;
    int channels;
    int width;
    int height;
} Img2D;

typedef struct Tex2D {
    Px* fb;
    int width;
    int height;
} Tex2D;

Img2D spxImageCreate(int width, int height, int channels);
Img2D spxImageLoad(const char* path);
int spxImageSave(Img2D image, const char* path);
void spxImageFree(Img2D* image);

Tex2D spxTextureCreate(int width, int height);
Tex2D spxTextureLoad(const char* path);
int spxTextureSave(const Tex2D texture, const char* path);
void spxTextureFree(Tex2D* texture);

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
    int i, j;
    for (i = 0; s1[i]; ++i) {
        const int c = tolower(s1[i]);
        for (j = 0; s2[j]; ++j) {
            if (c != s2[j]) {
                return 0;
            }
        }
    }
    return 1;
}

static int spxParseFormat(const char* path)
{
    size_t i, dotpos = 0;
    for (i = 0; path[i]; ++i) {
        if (path[i] == '.') {
            dotpos = i;
        }
    }
    
    if (dotpos) {
        const char* ext = path + dotpos + 1;
        if (spxiStrcmpLower(ext, "png")) {
            return SPXI_FORMAT_PNG;
        } else if (spxiStrcmpLower(ext, "jpeg") || spxiStrcmpLower(ext, "jpg")) {
            return SPXI_FORMAT_JPEG;
        } else if (spxiStrcmpLower(ext, "gif")) {
            return SPXI_FORMAT_GIF;
        } else if (spxiStrcmpLower(ext, "ppm")) {
            return SPXI_FORMAT_PPM;
        }
    }

    return SPXI_FORMAT_UNKNOWN;
}

static int spxParseHeader(const char* filebuf)
{
    if (!memcmp(filebuf, "\211PNG\r\n\032\n", 8)) {
        return SPXI_FORMAT_PNG;
    } else if (filebuf[0] == 0xFF && filebuf[1] == 0xD8 && filebuf[2] == 0xFF) {
        return SPXI_FORMAT_JPEG;
    } else if (!memcmp(filebuf, "GIF87a", 6) || !memcmp(filebuf, "GIF89a", 6)) {
        return SPXI_FORMAT_GIF;
    } else if (!memcmp(filebuf, "P6 ", 3)) {
        return SPXI_FORMAT_PPM;
    }

    return SPXI_FORMAT_UNKNOWN;
}

static char* spxFileRead(const char* path)
{
    char* buf = NULL;
    FILE* file = fopen(path, "rb");
    
    if (file) {
        size_t len;
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        fseek(file, 0, SEEK_SET);
        buf = malloc(len);
        fread(buf, len, sizeof(char), file);
        fclose(file);
    }

    return buf;
}

static Tex2D spxImageLoadGuess(const char* path)
{
    int format;
    Tex2D texture = {NULL, 0, 0};
    char* filebuf = spxiFileRead(const char* path);
    if (!filebuf) {
        fprintf(stderr, "spximg could not open file '%s'\n", path);
        return texture;
    }

    format = spxiParseHeader(filebuf);
    if (!format) {
        fprintf(stderr, "spximg does not recognize image format of file '%s'\n", path);
        return texture;
    }

    switch (format) {
        case SPXI_FORMAT_PNG: return spxImageLoadPng(path);
        case SPXI_FORMAT_JPEG: return spxImageLoadJpeg(path);
        case SPXI_FORMAT_GIF: return spxImageLoadGif(path);
        case SPXI_FORMAT_PPM: return spxImageLoadPpm(path);
    }
}

Tex2D spxImageLoad(const char* path)
{
    switch (spxiParseFormat(path))
        case SPXI_FORMAT_PNG: return spxImageLoadPng(path);
        case SPXI_FORMAT_JPEG: return spxImageLoadJpeg(path);
        case SPXI_FORMAT_GIF: return spxImageLoadGif(path);
        case SPXI_FORMAT_PPM: return spxImageLoadPpm(path);
    }
    return spxImageLoadGuess(path);
}

Img2D spxImageCreate(int width, int height, int channels)
{
    size_t size;
    Img2D image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    size = width * height * channels;
    image.fb = malloc(size);
    memset(image.fb, 255, size);
    return image;
}

Img2D spxImageReshape(const Img2D image, int channels)
{
    Img2D reshaped;
    reshaped.channels = channels;
    reshaped.width = image.width;
    reshaped.height = image.height;
    reshaped.fb = (uint8_t*)malloc(reshaped.width * reshaped.height * channels);
    switch (channels) {
        case 
    }
    return reshaped;
}

int spxImageSave(const Img2D image, const char* path)
{
    switch (spxiParseFormat(path)) {
        case SPXI_FORMAT_PNG: return spxImageSavePng(image, path);
        case SPXI_FORMAT_JPEG: return spxImageSaveJpeg(image, path);
        case SPXI_FORMAT_GIF: return spxImageSaveGif(image, path);
        case SPXI_FORMAT_PPM: return spxImageSavePpm(image, path);
    }

    fprintf(stderr, "spximg only supports saving .png, .jpeg, .gif and .ppm files\n");
    return EXIT_FAILURE;
}

void spxImageFree(Img2D* image)
{
    if (image.fb) {
        free(image.fb);
        image.fb = NULL;
        image.width = 0;
        image.height = 0;
        image.channels = 0;
    }
}

/* texture implementation */

Tex2D spxTextureCreate(int width, int height)
{
    size_t size;
    Tex2D texture;
    texture.width = width;
    texture.height = height;
    size = width * height * sizeof(Px);
    texture.fb = malloc(size);
    memset(texture.fb, 255, size);
    return texture;
}

Tex2D spxTextureFromImage(const Img2D image)
{
    Tex2D texture = {NULL, image.width, image.height};
    if (image.channels != sizeof(Px)) {
        image = spxImageReshape(image, sizeof(Px));
        texture.fb = image.fb;
    } else {
        texture.fb = (uint8_t*)malloc(texture.width * texture.height * sizeof(Px));
        memcpy(texture.fb, image.fb, texture.width * texture.height * sizeof(Px));
    }

    return texture;
}

int spxTextureSave(const Tex2D texture, const char* path)
{
    Img2D image = {texture.fb, 4, textures.width, textures.height};
    return spxImageSave(image, path);  
}

void spxTextureeFree(Tex2D* texture)
{
    if (texture.fb) {
        free(texture.fb);
        texture.fb = NULL;
        texture.width = 0;
        texture.height = 0;
    }
}

#endif /* SPXI_APPLICATION */
#endif /* SIMPLE_PIXEL_IMAGE_H */

