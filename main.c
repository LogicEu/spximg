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

#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>

#define SPXIMG_VERSION_MAJOR 1
#define SPXIMG_VERSION_MINOR 0
#define SPXIMG_VERSION_BUILD 0

static const char* spxImageFormatName(int format)
{
    static const char* table[] = {"Unknown", "PNG", "JPEG", "GIF", "PPM", "BMP"};
    return table[format > 5 ? 0 : format];
}

static const char* spxImageColorName(int channels)
{

    static const char* table[] = {"Unknown", "Gray", "GrayAlpha", "RGB", "RGBA"};
    return table[(channels < 0 || channels > 4) ? 0 : channels];
}

static int spximgVersion(const char* exestr)
{
    return fprintf(
        stdout, "%s: version %d.%d.%d\n",
        exestr, SPXIMG_VERSION_MAJOR, SPXIMG_VERSION_MINOR, SPXIMG_VERSION_BUILD
    );
}

static int spximgHelp(const char* exestr)
{
    fprintf(stdout, "%s usage:\n", exestr);
    fprintf(stdout, "<image.*>\t: Load <image.*> file (.png, .jpeg or .ppm)\n");
    fprintf(stdout, "-o <image.*>\t: Save <image.*> file (.png, .jpeg or .ppm)\n");
    fprintf(stdout, "-d\t\t: Display image information\n");
    fprintf(stdout, "-i\t\t: Save output image file to same path as input file\n");
    fprintf(stdout, "-n <int>\t: Reshape image to have <int> number of channels\n");
    fprintf(stdout, "-h, --help:\t: Display usage and available commands\n");
    fprintf(stdout, "-v, --version:\t: Display version information\n");
    return EXIT_SUCCESS;
}

static int spximgImageInfo(const Img2D image, const char* path, int format)
{
    static const char* sep = "-----------------------------------------------------\n";
    return fprintf(
        stdout, "%sfile: '%s'\nformat: %s\nwidth: %d\nheight: %d\n"
        "channels: %d - '%s'\n",
        sep, path, spxImageFormatName(format), image.width, image.height, 
        image.channels, spxImageColorName(image.channels)
    );
}

static int spximgCheckImage(
    const uint8_t* pixbuf, const char* path, const char* arg0, const char* argi)
{
    if (!pixbuf || !path) {
        fprintf(
            stderr, "%s: cannot use %s command when no image is loaded\n", arg0, argi
        );
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

static int spximgCheckArgs(
    const int argc, const int index, const char* arg0, const char* argi)
{
    if (index + 1 >= argc) {
        fprintf(stderr, "%s: missing argument for option %s\n", arg0, argi);
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int main(const int argc, const char** argv)
{
    int i, format = 0, status = EXIT_FAILURE;
    const char* path = NULL;
    Img2D image = {NULL, 0, 0, 0};

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            const char* cmd = argv[i] + 1;
            if ((cmd[0] == 'h' && !cmd[1]) || !strcmp(cmd, "-help")) {
                return spximgHelp(argv[0]);
            } else if ((cmd[0] == 'v' && !cmd[1]) || !strcmp(cmd, "-version")) {
                return spximgVersion(argv[0]);
            } else if (cmd[0] == 'd' && !cmd[1]) { 
                if (!spximgCheckImage(image.pixbuf, path, argv[0], argv[i])) {
                    spximgImageInfo(image, path, format);
                }
            } else if (cmd[0] == 'i' && !cmd[1]) {
                if (!spximgCheckImage(image.pixbuf, path, argv[0], argv[i])) {
                    spxImageSave(image, path);
                }
            } else if (cmd[0] == 'o' && !cmd[1]) {
                if (!spximgCheckImage(image.pixbuf, path, argv[0], argv[i++]) &&
                    !spximgCheckArgs(argc, i - 1, argv[0], argv[i - 1])) {
                    spxImageSave(image, argv[i]);
                }
            } else if (cmd[0] == 'n' && !cmd[1]) {
                if (!spximgCheckImage(image.pixbuf, path, argv[0], argv[i]) &&
                    !spximgCheckArgs(argc, i, argv[0], argv[i])) {
                    Img2D tmp = spxImageReshape(image, atoi(argv[++i]));
                    if (tmp.pixbuf) {
                        spxImageFree(&image);
                        image = tmp;
                    }
                }
            } else {
                fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[i]);
            }
        } else {
            path = argv[i];
            spxImageFree(&image);
            format = spxParseFormat(path);
            if (format == SPXI_FORMAT_NULL) {
                fprintf(stderr, "%s: could not open file %s\n", argv[0], argv[i]);
                continue;
            }

            image = spxImageLoad(path);
            if (status) {
                status = !image.pixbuf;
            }
        }
    }

    if (i == 1) {
        fprintf(stderr, "%s: missing arguments. See -h for more info\n", argv[0]);
        return EXIT_FAILURE;
    }
   
    spxImageFree(&image);
    return status;
}
