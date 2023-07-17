# spximg

Simple header only library for loading and saving image files. It supports
PNG, JPEG, GIF and PPM. This header is part of the 
[spxx](https://github.com/LogicEu/spxx.git) project. The main difference
with similar projects like [stb_image.h](https://github.com/nothings/stb/stb_image.h)
is that spximg.h assumes that most computers nowadays already have some version of
the [libpng](https://github.com/glennrp/libpng.git) and the 
[libjpeg](https://github.com/LuaDist/libjpeg.git) libraries, so it doesn't implement
those complex image formats from scratch, but rather uses their API,
resulting in much faster and lightweight compilations.

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
spximg.h does not implement the PNG and JPEG formats from scratch, instead, it uses
their official API and requires linking against those libraries.

```shell
gcc source.c -o program -lpng -ljpeg # -lz
```
You also need the standard headers of libpng and libjpeg, either installed in your
system, or provided with a path as include compilation flags (-Ipath/to/headers).

```C
#include <png.h>
#include <jpeglib.h>
```

