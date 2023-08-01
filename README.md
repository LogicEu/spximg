# spximg

Simple header only library for loading and saving image files. It supports
PNG, JPEG and PPM. This header is part of the 
[spxx](https://github.com/LogicEu/spxx.git) project. The main difference
with similar projects like 
[stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
is that spximg.h assumes that most computers nowadays already have some version of
the [libpng](https://github.com/glennrp/libpng.git) and the 
[libjpeg](https://github.com/LuaDist/libjpeg.git) libraries, so it doesn't implement
those image formats from scratch, but rather uses their API,
resulting in much faster and lightweight compilations. It recognizes the image 
format by checking file name extension and then comparing the file's header 
bytes against each format specification.

## Example

The following simple program accepts two command line arguments. It tries to read
the first argument as an image file and then saves it with the file name given
by the second argument and the format specified by its file extension, 
if supported.

```C

#define SPXI_APPLICATION
#include <spximg.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    Img2D img;

    if (argc < 3) {
        fprintf(stderr, "usage:\n%s <source_image> <target_image>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    img = spxImageLoad(argv[1]);
    if (!img.pixbuf) {
        fprintf(stderr, "%s: could not read image file: %s\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    if (spxImageSave(img, argv[2])) {
        fprintf(stderr, "%s: could not write image file: %s\n", argv[0], argv[2]);
        return EXIT_FAILURE;
    }

    spxImageFree(&img);
    return EXIT_SUCCESS;
}

```

## Header-Only

As a header only solution, you need to define 
SPXI_APPLICATION before including spximg.h to access the
implementation details. Otherwise you only declare the interface.

```C
#define SPXI_APPLICATION
#include <spximg.h>
```

## Dependencies

As stated before, the main difference between spximg.h and stb_image.h is that
spximg.h does not implement the PNG and JPEG formats from scratch, but uses
their official API so it requires linking against those libpng and libjpeg.

```shell
gcc source.c -o program -lpng -ljpeg -lz
```
You also need the standard headers of libpng and libjpeg, either installed in your
system, or provided with a path as include compilation flags (-Ipath/to/headers).

```C
#include <png.h>
#include <jpeglib.h>
```

