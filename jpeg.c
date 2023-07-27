#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <jpeglib.h>

int spxImageSaveJpeg(const Img2D img, const char* path, const int quality) 
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "imgtool could not write JPEG file '%s'\n", path);
        return;
    }

    jpeg_stdio_dest(&cinfo, file);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;	
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = (uint8_t*)(size_t)(data + cinfo.next_scanline * row_stride);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);

    fclose(file);
    jpeg_destroy_compress(&cinfo);
}

uint8_t* jpeg_file_load(const char* restrict path, unsigned int* w, unsigned int* h)
{
	struct stat file_info;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	unsigned long bmp_size;
	uint8_t *bmp_buffer;
	int row_stride, width, height, pixel_size;

    int rc = stat(path, &file_info);
    if (rc) {
        fprintf(stderr, "imgtool could not get info about JPEG file '%s'\n", path);
        return NULL;
    }

	unsigned long jpg_size = file_info.st_size;
	uint8_t* jpg_buffer = (uint8_t*)malloc(jpg_size + 100);

	FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "imgtool could not open JPEG file '%s'\n", path);
        return NULL;
    } 

	fread(jpg_buffer, jpg_size, 1, file);
	fclose(file);  

	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);
	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		fprintf(stderr, "file '%s' does not seem to be a normal JPEG.\n", path);
		return NULL;
	}

	jpeg_start_decompress(&cinfo);

    width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;

	bmp_size = width * height * pixel_size;
	bmp_buffer = (uint8_t*)malloc(bmp_size);
	row_stride = width * pixel_size;

	while (cinfo.output_scanline < cinfo.output_height) {
		uint8_t *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(jpg_buffer);

    *w = width;
    *h = height;
    return bmp_buffer;
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

    img = spxImageCreate(400, 300, 3);
    spxImageSaveJpeg(img, "images/bmp.jpeg");

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
