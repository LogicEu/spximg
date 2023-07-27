#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>

#include <jpeglib.h>

#ifndef SPXI_JPEG_QUALITY 
#define SPXI_JPEG_QUALITY 100
#endif /* SPXI_JPEG_QUALITY */

Img2D spxImageReshape2to1(const Img2D img)
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

static const char* spxFormatName(int format)
{
    static const char* table[] = {"Unknown", "PNG", "JPEG", "GIF", "PPM"};
    return table[format > 4 ? 0 : format];
}

static const char* spxImageChannelsString(int colorType)
{
    switch (colorType) {
        case SPXI_COLOR_RGBA: return "RGBA";
        case SPXI_COLOR_RGB: return "RGB";
        case SPXI_COLOR_GRAY_ALPHA: return "GrayAlpha";
        case SPXI_COLOR_GRAY: return "Gray";                        
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

    image = spxImageLoadJpeg(argv[1]);
    if (!image.pixbuf) {
        fprintf(stderr, "%s: could not load file: %s\n", argv[0], argv[1]);
        return 2;
    }

    fprintf(
        stdout, 
        "file: '%s'\nformat: %s\nwidth: %d\nheight: %d\nchannels: %d\n",
        argv[1],
        "JPEG",
        image.width,
        image.height,
        image.channels
    );

    img = spxImageCreate(400, 300, 1);
    spxImageSaveJpeg(img, "images/bmp.jpeg", SPXI_JPEG_QUALITY);

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
