#ifndef EXR_WRITE_H
#define EXR_WRITE_H
#include "tools.h"
#include "stdio.h"
//@TODO make it stb style (with EXR_WRITE_IMPL and stuff)

typedef struct NodeTypeLL
{
  f32 alpha;
  f32 blue;
  f32 green;
  f32 red;
	f32 depth;
	u32 next;
}NodeTypeLL;

typedef struct DeepPixel
{
    f32 a; //its for all the DeepPixel, no layers needed!
    f32 b;
    f32 g;
    f32 r;
    f32 z;
}DeepPixel;
typedef struct DeepPixelRGB
{
    f32 b;
    f32 g;
    f32 r;
    f32 z;
}DeepPixelRGB;


enum {
    EXR_FAIL = 0x0,
    EXR_OK = 0x1,
    EXR_SCANLINE = 0x2,
};

enum {
    EXR_UINT = 0x0,
    EXR_HALF = 0x1,
    EXR_FLOAT = 0x2,
};
enum {
    EXR_INCREASING_Y = 0x0,
    EXR_DECREASING_Y = 0x1,
    EXR_RANDOM_Y = 0x2,
};

enum {
    EXR_PAD = 0x1,
    EXR_RGB = 0x2,
    EXR_RGBA = 0x4,
};
#define DEEP_IMAGE_VERSION_BITMASK 0x800

internal u32
count_bytes_in_file(FILE *file)
{
    u32 bytes = 0;
    for(bytes = 0; getc(file) != EOF; ++bytes);

}

internal u8* openexr_write(u32 width, u32 height, u32 channels,f32* data, i32* outSize)
{
	u32 ww = width-1;
	u32 hh = height-1;
    //NOTE(ilias): Do we want array initialization? maybe be more explicit???
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		2, 0, 0, 0, // version, scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'B',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // B, float 
		'G',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // G, float 
		'R',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, float
		0,
		// compression
		'c','o','m','p','r','e','s','s','i','o','n',0,
		'c','o','m','p','r','e','s','s','i','o','n',0,
		1,0,0,0,
		0, // no compression
		// dataWindow
		'd','a','t','a','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// displayWindow
		'd','i','s','p','l','a','y','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// lineOrder
		'l','i','n','e','O','r','d','e','r',0,
		'l','i','n','e','O','r','d','e','r',0,
		1,0,0,0,
		0, // increasing Y
		// pixelAspectRatio
		'p','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
		// screenWindowCenter
		's','c','r','e','e','n','W','i','n','d','o','w','C','e','n','t','e','r',0,
		'v','2','f',0,
		8,0,0,0,
		0,0,0,0, 0,0,0,0,
		// screenWindowWidth
		's','c','r','e','e','n','W','i','n','d','o','w','W','i','d','t','h',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
		// end of header
		0,
	};
	i32 hdr_size = array_count(hdr_data);

    //how much space the scanlines take after the header is written!
	i32 scanline_table_size = 8 * height;

    //size of a row of data (width * sizeof(vec3) * sizeof(FLOAT))
	u32 pixel_row_size = width * 3 * 4;
    
    //size of a row of data + ycoord
	u32 full_row_size = pixel_row_size + 8;

	u32 buf_size = hdr_size + scanline_table_size + height * full_row_size;
	u8* buf = (u8*)malloc (buf_size);
	if (!buf)
		return NULL;

	// copy in header
	memcpy (buf, hdr_data, hdr_size);

	// line offset table
	u32 index = hdr_size + scanline_table_size;
	u8* ptr = buf + hdr_size;
	for (int y = 0; y < height; ++y)
	{
		*ptr++ = index & 0xFF;
		*ptr++ = (index >> 8) & 0xFF;
		*ptr++ = (index >> 16) & 0xFF;
		*ptr++ = (index >> 24) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		index += full_row_size;
	}

	// scanline data
  /*each scanline must have:
     - y coordinate
     - pixel data size
     - pixel data
  */
	u8* src = (u8*)data;
	i32 stride = channels * 4;
	for (i32 y = 0; y < height; ++y)
	{
		//y coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
		//pixel data size
		*ptr++ = pixel_row_size & 0xFF;
		*ptr++ = (pixel_row_size >> 8) & 0xFF;
		*ptr++ = (pixel_row_size >> 16) & 0xFF;
		*ptr++ = (pixel_row_size >> 24) & 0xFF;
		//pixel data (B, G, R)
		u8* chsrc;
		chsrc = src + 8;
		for (i32 x = 0; x < width; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
		chsrc = src + 4;
		for (i32 x = 0; x < width; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
		chsrc = src + 0;
		for (i32 x = 0; x < width; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}

		src += width * stride;
	}

	assert (ptr - buf == buf_size);

	*outSize = buf_size;
	return buf;
}


internal u8* openexr_write_half(u32 width, u32 height, u32 channels,void* rgba16f, i32* outSize)
{
	u32 ww = width-1;
	u32 hh = height-1;
    //NOTE(ilias): Do we want array initialization? maybe be more explicit???
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		2, 0, 0, 0, // version, scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'B',0, 1,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // B, half
		'G',0, 1,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // G, half
		'R',0, 1,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, half
		0,
		// compression
		'c','o','m','p','r','e','s','s','i','o','n',0,
		'c','o','m','p','r','e','s','s','i','o','n',0,
		1,0,0,0,
		0, // no compression
		// dataWindow
		'd','a','t','a','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// displayWindow
		'd','i','s','p','l','a','y','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// lineOrder
		'l','i','n','e','O','r','d','e','r',0,
		'l','i','n','e','O','r','d','e','r',0,
		1,0,0,0,
		0, // increasing Y
		// pixelAspectRatio
		'p','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
		// screenWindowCenter
		's','c','r','e','e','n','W','i','n','d','o','w','C','e','n','t','e','r',0,
		'v','2','f',0,
		8,0,0,0,
		0,0,0,0, 0,0,0,0,
		// screenWindowWidth
		's','c','r','e','e','n','W','i','n','d','o','w','W','i','d','t','h',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f


		// end of header
		0,
	};
	i32 hdr_size = array_count(hdr_data);

    //how much space the scanlines take after the header is written!
	i32 scanline_table_size = 8 * height;

    //size of a row of data (width * sizeof(vec3) * sizeof(HALF_FLOAT))
	u32 pixel_row_size = width * 3 * 2;
    
    //size of a row of data + 
	u32 full_row_size = pixel_row_size + 8;

	u32 buf_size = hdr_size + scanline_table_size + height * full_row_size;
	u8* buf = (u8*)malloc (buf_size);
	if (!buf)
		return NULL;

	// copy in header
	memcpy (buf, hdr_data, hdr_size);

	// line offset table
	u32 index = hdr_size + scanline_table_size;
	u8* ptr = buf + hdr_size;
	for (int y = 0; y < height; ++y)
	{
		*ptr++ = index & 0xFF;
		*ptr++ = (index >> 8) & 0xFF;
		*ptr++ = (index >> 16) & 0xFF;
		*ptr++ = (index >> 24) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		index += full_row_size;
	}

	// scanline data
  /*each scanline must have:
     - y coordinate
     - pixel data size
     - pixel data
  */
	u8* src = (u8*)rgba16f;
	i32 stride = channels * 2;
	for (i32 y = 0; y < height; ++y)
	{
		//y coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
		//pixel data size
		*ptr++ = pixel_row_size & 0xFF;
		*ptr++ = (pixel_row_size >> 8) & 0xFF;
		*ptr++ = (pixel_row_size >> 16) & 0xFF;
		*ptr++ = (pixel_row_size >> 24) & 0xFF;
		//pixel data (B, G, R)
		u8* chsrc;
		chsrc = src + 4;
		for (i32 x = 0; x < width; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 2;
		for (i32 x = 0; x < width; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 0;
		for (i32 x = 0; x < width; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}

		src += width * stride;
	}

	assert (ptr - buf == buf_size);

	*outSize = buf_size;
	return buf;
}

extern mat4 view, proj;
extern Renderer rend;
internal u8 *deepexr_write_padding(u32 width, u32 height,DeepPixel *pixels, u32 pixels_count, u32 num_of_deep_samples_per_pixel)
{
  //width = global_platform.window_width;
  //height = global_platform.window_height;
	u32 ww = width-1;
	u32 hh = height-1;
    mat4 view_mat = get_view_mat(&rend.deep_cam);
    u32 *v = &view_mat;
    u32 *p = &rend.proj;
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		0x02, 0x08, 0, 0, // version, DEEP scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'A',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // A, FLOAT 
		'B',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // B, FLOAT 
		'G',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // G, FLOAT 
		'R',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, FLOAT 
		'Z',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // Z, FLOAT 
		0,
		// compression
		'c','o','m','p','r','e','s','s','i','o','n',0,
		'c','o','m','p','r','e','s','s','i','o','n',0,
		1,0,0,0,
		0, // no compression
		// dataWindow
		'd','a','t','a','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// displayWindow
		'd','i','s','p','l','a','y','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// lineOrder
		'l','i','n','e','O','r','d','e','r',0,
		'l','i','n','e','O','r','d','e','r',0,
		1,0,0,0,
		0, // increasing Y
		// pixelAspectRatio
		'p','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
		// screenWindowCenter
		's','c','r','e','e','n','W','i','n','d','o','w','C','e','n','t','e','r',0,
		'v','2','f',0,
		8,0,0,0,
		0,0,0,0, 0,0,0,0,
		// screenWindowWidth
		's','c','r','e','e','n','W','i','n','d','o','w','W','i','d','t','h',0,
		'f','l','o','a','t',0, 
    4,0,0,0,
		0,0,0x80,0x3f, // 1.0f

    'm','a','x','S','a','m','p','l','e','s','P','e','r','P','i','x','e','l',0,
    'i','n','t',0,
    4,0,0,0,
    (((num_of_deep_samples_per_pixel) >> 0) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 4) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 8) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 12) & 0xFF),

    'v','i','e','w','M','a','t','r','i','x',0,
    'm','4','4','f',0,
     64, 0, 0,0,
     (v[0] >>0)& 0xFF,(v[0] >>4)& 0xFF,(v[0] >>8)& 0xFF,(v[0] >>12)& 0xFF,
     (v[1] >>0)& 0xFF,(v[1] >>4)& 0xFF,(v[1] >>8)& 0xFF,(v[1] >>12)& 0xFF,
     (v[2] >>0)& 0xFF,(v[2] >>4)& 0xFF,(v[2] >>8)& 0xFF,(v[2] >>12)& 0xFF,
     (v[3] >>0)& 0xFF,(v[3] >>4)& 0xFF,(v[3] >>8)& 0xFF,(v[3] >>12)& 0xFF,
     (v[4] >>0)& 0xFF,(v[4] >>4)& 0xFF,(v[4] >>8)& 0xFF,(v[4] >>12)& 0xFF,
     (v[5] >>0)& 0xFF,(v[5] >>4)& 0xFF,(v[5] >>8)& 0xFF,(v[5] >>12)& 0xFF,
     (v[6] >>0)& 0xFF,(v[6] >>4)& 0xFF,(v[6] >>8)& 0xFF,(v[6] >>12)& 0xFF,
     (v[7] >>0)& 0xFF,(v[7] >>4)& 0xFF,(v[7] >>8)& 0xFF,(v[7] >>12)& 0xFF,
     (v[8] >>0)& 0xFF,(v[8] >>4)& 0xFF,(v[8] >>8)& 0xFF,(v[8] >>12)& 0xFF,
     (v[9] >>0)& 0xFF,(v[9] >>4)& 0xFF,(v[9] >>8)& 0xFF,(v[9] >>12)& 0xFF,
     (v[10] >>0)& 0xFF,(v[10] >>4)& 0xFF,(v[10] >>8)& 0xFF,(v[10] >>12)& 0xFF,
     (v[11] >>0)& 0xFF,(v[11] >>4)& 0xFF,(v[11] >>8)& 0xFF,(v[11] >>12)& 0xFF,
     (v[12] >>0)& 0xFF,(v[12] >>4)& 0xFF,(v[12] >>8)& 0xFF,(v[12] >>12)& 0xFF,
     (v[13] >>0)& 0xFF,(v[13] >>4)& 0xFF,(v[13] >>8)& 0xFF,(v[13] >>12)& 0xFF,
     (v[14] >>0)& 0xFF,(v[14] >>4)& 0xFF,(v[14] >>8)& 0xFF,(v[14] >>12)& 0xFF,
     (v[15] >>0)& 0xFF,(v[15] >>4)& 0xFF,(v[15] >>8)& 0xFF,(v[15] >>12)& 0xFF,

    'p','r','o','j','e','c','t','i','o','n','M','a','t','r','i','x',0,
    'm','4','4','f',0,
     64, 0, 0,0,
     (p[0] >>0)& 0xFF,(p[0] >>4)& 0xFF,(p[0] >>8)& 0xFF,(p[0] >>12)& 0xFF,
     (p[1] >>0)& 0xFF,(p[1] >>4)& 0xFF,(p[1] >>8)& 0xFF,(p[1] >>12)& 0xFF,
     (p[2] >>0)& 0xFF,(p[2] >>4)& 0xFF,(p[2] >>8)& 0xFF,(p[2] >>12)& 0xFF,
     (p[3] >>0)& 0xFF,(p[3] >>4)& 0xFF,(p[3] >>8)& 0xFF,(p[3] >>12)& 0xFF,
     (p[4] >>0)& 0xFF,(p[4] >>4)& 0xFF,(p[4] >>8)& 0xFF,(p[4] >>12)& 0xFF,
     (p[5] >>0)& 0xFF,(p[5] >>4)& 0xFF,(p[5] >>8)& 0xFF,(p[5] >>12)& 0xFF,
     (p[6] >>0)& 0xFF,(p[6] >>4)& 0xFF,(p[6] >>8)& 0xFF,(p[6] >>12)& 0xFF,
     (p[7] >>0)& 0xFF,(p[7] >>4)& 0xFF,(p[7] >>8)& 0xFF,(p[7] >>12)& 0xFF,
     (p[8] >>0)& 0xFF,(p[8] >>4)& 0xFF,(p[8] >>8)& 0xFF,(p[8] >>12)& 0xFF,
     (p[9] >>0)& 0xFF,(p[9] >>4)& 0xFF,(p[9] >>8)& 0xFF,(p[9] >>12)& 0xFF,
     (p[10] >>0)& 0xFF,(p[10] >>4)& 0xFF,(p[10] >>8)& 0xFF,(p[10] >>12)& 0xFF,
     (p[11] >>0)& 0xFF,(p[11] >>4)& 0xFF,(p[11] >>8)& 0xFF,(p[11] >>12)& 0xFF,
     (p[12] >>0)& 0xFF,(p[12] >>4)& 0xFF,(p[12] >>8)& 0xFF,(p[12] >>12)& 0xFF,
     (p[13] >>0)& 0xFF,(p[13] >>4)& 0xFF,(p[13] >>8)& 0xFF,(p[13] >>12)& 0xFF,
     (p[14] >>0)& 0xFF,(p[14] >>4)& 0xFF,(p[14] >>8)& 0xFF,(p[14] >>12)& 0xFF,
     (p[15] >>0)& 0xFF,(p[15] >>4)& 0xFF,(p[15] >>8)& 0xFF,(p[15] >>12)& 0xFF,


   	't','y','p','e',0,
		's','t','r','i','n','g',0,
		 0x0C,0,0,0,
    'd','e','e','p','s','c','a','n','l','i','n','e',//0,

    'v','e','r','s','i','o','n',0,
    'i','n','t',0,
    4,0,0,0,
    1,0,0,0,
		// end of header
		0,
	};
	i32 hdr_size = array_count(hdr_data);


	i32 scanline_table_size = sizeof(u64) * height;
	u32 pixel_row_size = width * sizeof(DeepPixel) * num_of_deep_samples_per_pixel;
	u32 full_row_size = pixel_row_size + 4 * width + 28; //TODO(iv): investigate

	u32 buf_size = hdr_size + scanline_table_size + height * full_row_size;
    //TODO: write in custom allocator?!!?!
	u8* buf = (u8*)malloc (buf_size);
	if (!buf)
		return NULL;

	// copy in header
	memcpy (buf, hdr_data, hdr_size);

	// line offset table
	u32 index = hdr_size + scanline_table_size;
	u8* ptr = buf + hdr_size;
	for (i32 y = 0; y < height; ++y)
	{
		*ptr++ = index & 0xFF;
		*ptr++ = (index >> 8) & 0xFF;
		*ptr++ = (index >> 16) & 0xFF;
		*ptr++ = (index >> 24) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		index += full_row_size; //TODO(iv): investigate
	}

	// scanline data (channels MUST be in alphabetical order!)
  /*each deep scanline must have:
     - y coordinate (int)
     - packed size of pixel offset table (u64)
     - packed size of sample data (u64)
     - unpacked size of sample data (u64)
     - compressed pixel offset table (ints??)
     - compressed sample data (sizeof(DeepData))
  */
	u8* src = (u8*)pixels;
	i32 stride = sizeof(DeepPixel);
  //we do this because the packed size is a list of ints
  i32 num_of_deep_samples_per_pixel_int = (i32)num_of_deep_samples_per_pixel;
  u64 num_of_deep_samples_per_pixel_long = (u64)num_of_deep_samples_per_pixel;
  u64 pixel_row_size_long = (u64)pixel_row_size;
	i32 pixel_row_size_int = (i32)pixel_row_size;
	for (i32 y = 0; y < (int)height; ++y)
	{
		// y coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
    //packed size of pixel offset table
    *ptr++ = (width*sizeof(int)) & 0xFF;
    *ptr++ = ((width*sizeof(int)) >> 8) & 0xFF;
    *ptr++ = ((width*sizeof(int)) >> 16) & 0xFF;
    *ptr++ = ((width*sizeof(int)) >> 24) & 0xFF;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
		// packed size of sample data 
		*ptr++ = pixel_row_size_long & 0xFF;
		*ptr++ = (pixel_row_size_long>> 8) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 16) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 24) & 0xFF;
		*ptr++ = (pixel_row_size_long >> 32) & 0xFF;
		*ptr++ = (pixel_row_size_long >> 40) & 0xFF;
		*ptr++ = (pixel_row_size_long >> 48) & 0xFF;
		*ptr++ = (pixel_row_size_long >> 56) & 0xFF;
    // unpacked size of sample data
		*ptr++ = pixel_row_size_long & 0xFF;
		*ptr++ = (pixel_row_size_long>> 8) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 16) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 24) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 32) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 40) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 48) & 0xFF;
		*ptr++ = (pixel_row_size_long>> 56) & 0xFF;
    //compressed pixel offset table (each deep pixel has num_of_.. samples per pixel!)
    for (i32 i = 0; i < width; ++i)
    {
      *ptr++ = (num_of_deep_samples_per_pixel_int * (int)(i+1)) & 0xFF;
      *ptr++ = ((num_of_deep_samples_per_pixel_int* (int)(i+1)) >> 8) & 0xFF;
      *ptr++ = ((num_of_deep_samples_per_pixel_int* (int)(i+1)) >> 16) & 0xFF;
      *ptr++ = ((num_of_deep_samples_per_pixel_int* (int)(i+1)) >> 24) & 0xFF;
    }
    //compressed sample data
		// R G B A Z(ABGRZ ya mean) data for each deep pixel
		u8* chsrc;
		chsrc = src + 0;
		for (i32 x = 0; x < width * num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];

			chsrc += stride;
		}
    chsrc = src + 4; //ok
		for (i32 x = 0; x < width* num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
    chsrc = src + 8; //ok
		for (i32 x = 0; x < width* num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];

			chsrc += stride;
		}
		chsrc = src + 12;
		for (i32 x = 0; x < width* num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
		chsrc = src + 16;
		for (i32 x = 0; x < width* num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}




		src += width * stride * num_of_deep_samples_per_pixel;
	}
  FILE* f = fopen ("deep.exr", "wb");
  fwrite (buf, 1, buf_size, f);
  //fwrite (buf, 1, buf_size, f);
  fclose (f);

	return buf;
}

internal u8 *deepexr_write(u32 width, u32 height,DeepPixel *pixels,u32 *samples_per_pixel, u32 pixels_count, u32 num_of_deep_samples_per_pixel)
{
    //width = global_platform.window_width;
    //height = global_platform.window_height;
    u32 ww = width-1;
    u32 hh = height-1;
    mat4 view_mat = get_view_mat(&rend.deep_cam);
    u32 *v = &view_mat;
    u32 *p = &rend.proj;
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		0x02, 0x08, 0, 0, // version, DEEP scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'B',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // B, FLOAT 
		'G',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // G, FLOAT 
		'R',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, FLOAT 
		'Z',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // Z, FLOAT 
		0,
		// compression
		'c','o','m','p','r','e','s','s','i','o','n',0,
		'c','o','m','p','r','e','s','s','i','o','n',0,
		1,0,0,0,
		0, // no compression
		// dataWindow
		'd','a','t','a','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// displayWindow
		'd','i','s','p','l','a','y','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// lineOrder
		'l','i','n','e','O','r','d','e','r',0,
		'l','i','n','e','O','r','d','e','r',0,
		1,0,0,0,
		0, // increasing Y
		// pixelAspectRatio
		'p','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
		// screenWindowCenter
		's','c','r','e','e','n','W','i','n','d','o','w','C','e','n','t','e','r',0,
		'v','2','f',0,
		8,0,0,0,
		0,0,0,0, 0,0,0,0,
		// screenWindowWidth
		's','c','r','e','e','n','W','i','n','d','o','w','W','i','d','t','h',0,
		'f','l','o','a','t',0, 
    4,0,0,0,
		0,0,0x80,0x3f, // 1.0f

    'm','a','x','S','a','m','p','l','e','s','P','e','r','P','i','x','e','l',0,
    'i','n','t',0,
    4,0,0,0,
    (((num_of_deep_samples_per_pixel) >> 0) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 4) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 8) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 12) & 0xFF),

    'v','i','e','w','M','a','t','r','i','x',0,
    'm','4','4','f',0,
     64, 0, 0,0,
     (v[0] >>0)& 0xFF,(v[0] >>4)& 0xFF,(v[0] >>8)& 0xFF,(v[0] >>12)& 0xFF,
     (v[1] >>0)& 0xFF,(v[1] >>4)& 0xFF,(v[1] >>8)& 0xFF,(v[1] >>12)& 0xFF,
     (v[2] >>0)& 0xFF,(v[2] >>4)& 0xFF,(v[2] >>8)& 0xFF,(v[2] >>12)& 0xFF,
     (v[3] >>0)& 0xFF,(v[3] >>4)& 0xFF,(v[3] >>8)& 0xFF,(v[3] >>12)& 0xFF,
     (v[4] >>0)& 0xFF,(v[4] >>4)& 0xFF,(v[4] >>8)& 0xFF,(v[4] >>12)& 0xFF,
     (v[5] >>0)& 0xFF,(v[5] >>4)& 0xFF,(v[5] >>8)& 0xFF,(v[5] >>12)& 0xFF,
     (v[6] >>0)& 0xFF,(v[6] >>4)& 0xFF,(v[6] >>8)& 0xFF,(v[6] >>12)& 0xFF,
     (v[7] >>0)& 0xFF,(v[7] >>4)& 0xFF,(v[7] >>8)& 0xFF,(v[7] >>12)& 0xFF,
     (v[8] >>0)& 0xFF,(v[8] >>4)& 0xFF,(v[8] >>8)& 0xFF,(v[8] >>12)& 0xFF,
     (v[9] >>0)& 0xFF,(v[9] >>4)& 0xFF,(v[9] >>8)& 0xFF,(v[9] >>12)& 0xFF,
     (v[10] >>0)& 0xFF,(v[10] >>4)& 0xFF,(v[10] >>8)& 0xFF,(v[10] >>12)& 0xFF,
     (v[11] >>0)& 0xFF,(v[11] >>4)& 0xFF,(v[11] >>8)& 0xFF,(v[11] >>12)& 0xFF,
     (v[12] >>0)& 0xFF,(v[12] >>4)& 0xFF,(v[12] >>8)& 0xFF,(v[12] >>12)& 0xFF,
     (v[13] >>0)& 0xFF,(v[13] >>4)& 0xFF,(v[13] >>8)& 0xFF,(v[13] >>12)& 0xFF,
     (v[14] >>0)& 0xFF,(v[14] >>4)& 0xFF,(v[14] >>8)& 0xFF,(v[14] >>12)& 0xFF,
     (v[15] >>0)& 0xFF,(v[15] >>4)& 0xFF,(v[15] >>8)& 0xFF,(v[15] >>12)& 0xFF,

    'p','r','o','j','e','c','t','i','o','n','M','a','t','r','i','x',0,
    'm','4','4','f',0,
     64, 0, 0,0,
     (p[0] >>0)& 0xFF,(p[0] >>4)& 0xFF,(p[0] >>8)& 0xFF,(p[0] >>12)& 0xFF,
     (p[1] >>0)& 0xFF,(p[1] >>4)& 0xFF,(p[1] >>8)& 0xFF,(p[1] >>12)& 0xFF,
     (p[2] >>0)& 0xFF,(p[2] >>4)& 0xFF,(p[2] >>8)& 0xFF,(p[2] >>12)& 0xFF,
     (p[3] >>0)& 0xFF,(p[3] >>4)& 0xFF,(p[3] >>8)& 0xFF,(p[3] >>12)& 0xFF,
     (p[4] >>0)& 0xFF,(p[4] >>4)& 0xFF,(p[4] >>8)& 0xFF,(p[4] >>12)& 0xFF,
     (p[5] >>0)& 0xFF,(p[5] >>4)& 0xFF,(p[5] >>8)& 0xFF,(p[5] >>12)& 0xFF,
     (p[6] >>0)& 0xFF,(p[6] >>4)& 0xFF,(p[6] >>8)& 0xFF,(p[6] >>12)& 0xFF,
     (p[7] >>0)& 0xFF,(p[7] >>4)& 0xFF,(p[7] >>8)& 0xFF,(p[7] >>12)& 0xFF,
     (p[8] >>0)& 0xFF,(p[8] >>4)& 0xFF,(p[8] >>8)& 0xFF,(p[8] >>12)& 0xFF,
     (p[9] >>0)& 0xFF,(p[9] >>4)& 0xFF,(p[9] >>8)& 0xFF,(p[9] >>12)& 0xFF,
     (p[10] >>0)& 0xFF,(p[10] >>4)& 0xFF,(p[10] >>8)& 0xFF,(p[10] >>12)& 0xFF,
     (p[11] >>0)& 0xFF,(p[11] >>4)& 0xFF,(p[11] >>8)& 0xFF,(p[11] >>12)& 0xFF,
     (p[12] >>0)& 0xFF,(p[12] >>4)& 0xFF,(p[12] >>8)& 0xFF,(p[12] >>12)& 0xFF,
     (p[13] >>0)& 0xFF,(p[13] >>4)& 0xFF,(p[13] >>8)& 0xFF,(p[13] >>12)& 0xFF,
     (p[14] >>0)& 0xFF,(p[14] >>4)& 0xFF,(p[14] >>8)& 0xFF,(p[14] >>12)& 0xFF,
     (p[15] >>0)& 0xFF,(p[15] >>4)& 0xFF,(p[15] >>8)& 0xFF,(p[15] >>12)& 0xFF,


   	't','y','p','e',0,
		's','t','r','i','n','g',0,
		 0x0C,0,0,0,
    'd','e','e','p','s','c','a','n','l','i','n','e',//0,

    'v','e','r','s','i','o','n',0,
    'i','n','t',0,
    4,0,0,0,
    1,0,0,0,
		// end of header
		0,
	};
	i32 hdr_size = array_count(hdr_data);
    u32 *row_sizes = ALLOC(sizeof(u32) * width * height);
    for (u32 i = 0; i < height; ++i)
    {
        u32 samples = 0;
        for (u32 j = 0; j <width; ++j)
        {
            samples += samples_per_pixel[i * width + j]; 
        }
        row_sizes[i] = samples;//*sizeof(DeepPixel);
    }


	i32 scanline_table_size = sizeof(u64) * height;

    u32 row_size_sum = 0;
    for (u32 i = 0; i < height; ++i)
    {
        row_size_sum += row_sizes[i] * sizeof(DeepPixelRGB); 
        row_size_sum += 4 * width + 28;
    }

	u32 buf_size = hdr_size + row_size_sum + scanline_table_size;
    //TODO: write in custom allocator?!!?!
	u8* buf = (u8*)malloc (buf_size);
	if (!buf)
		return NULL;

	// copy in header
	memcpy (buf, hdr_data, hdr_size);

	// line offset table
	u32 index = hdr_size + scanline_table_size;
	u8* ptr = buf + hdr_size;
	for (i32 y = 0; y < height; ++y)
	{
        //u32 row_size_i = calc_row_size_i(pp, i);
		*ptr++ = index & 0xFF;
		*ptr++ = (index >> 8) & 0xFF;
		*ptr++ = (index >> 16) & 0xFF;
		*ptr++ = (index >> 24) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		index += row_sizes[y] * sizeof(DeepPixelRGB) + 4*width + 28; //TODO(iv): SUSPECT GUY 
	}

	// scanline data (channels MUST be in alphabetical order!)
  /*each deep scanline must have:
     - y coordinate (int)
     - packed size of pixel offset table (u64)
     - packed size of sample data (u64)
     - unpacked size of sample data (u64)
     - compressed pixel offset table (ints??)
     - compressed sample data (sizeof(DeepData))
  */
	u8* src = (u8*)pixels;
	i32 stride = sizeof(DeepPixel);
  //we do this because the packed size is a list of ints
	for (i32 y = 0; y < (int)height; ++y)
	{

		// y coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
        //packed size of pixel offset table
        u32 offset_table_size = width * sizeof(int);
        *ptr++ = (offset_table_size >> 0) & 0xFF;
        *ptr++ = (offset_table_size >> 8) & 0xFF;
        *ptr++ = (offset_table_size >> 16) & 0xFF;
        *ptr++ = (offset_table_size >> 24) & 0xFF;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
		// packed size of sample data 
        u32 packed_size = row_sizes[y] * sizeof(DeepPixel);
		*ptr++ = (packed_size >> 0)& 0xFF;
		*ptr++ = (packed_size >> 8)& 0xFF;
		*ptr++ = (packed_size >> 16)& 0xFF;
		*ptr++ = (packed_size >> 24)& 0xFF;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        // unpacked size of sample data
        *ptr++ = (packed_size >> 0)& 0xFF;
        *ptr++ = (packed_size >> 8)& 0xFF;
        *ptr++ = (packed_size >> 16)& 0xFF;
        *ptr++ = (packed_size >> 24)& 0xFF;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
    //compressed pixel offset table (each deep pixel has num_of_.. samples per pixel!)
    i32 row_sum = 0;
    for (i32 i = 0; i < width; ++i)
    {
      row_sum += samples_per_pixel[y * width + i];
      *ptr++ = (row_sum >> 0) & 0xFF;
      *ptr++ = (row_sum >> 8) & 0xFF;
      *ptr++ = (row_sum>> 16) & 0xFF;
      *ptr++ = (row_sum >> 24) & 0xFF;
    }
    //compressed sample data
		// R G B A Z(ABGRZ ya mean) data for each deep pixel
		u8* chsrc;
    chsrc = src + 4; //ok
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
    chsrc = src + 8; //ok
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];

			chsrc += stride;
		}
		chsrc = src + 12;
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
		chsrc = src + 16;
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}


		src += sizeof(DeepPixel) * row_sizes[y];
	}
  FILE* f = fopen ("deep.exr", "wb");
  fwrite (buf, 1, buf_size, f);
  //fwrite (buf, 1, buf_size, f);
  fclose (f);

	return buf;
}

internal u8 *deepexr_write_rgba(u32 width, u32 height,DeepPixel *pixels,u32 *samples_per_pixel, u32 pixels_count, u32 num_of_deep_samples_per_pixel)
{
    //width = global_platform.window_width;
    //height = global_platform.window_height;
    u32 ww = width-1;
    u32 hh = height-1;
    mat4 view_mat = get_view_mat(&rend.deep_cam);
    u32 *v = &view_mat;
    u32 *p = &rend.proj;
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		0x02, 0x08, 0, 0, // version, DEEP scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'A',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // A, FLOAT 
		'B',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // B, FLOAT 
		'G',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // G, FLOAT 
		'R',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, FLOAT 
		'Z',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // Z, FLOAT 
		0,
		// compression
		'c','o','m','p','r','e','s','s','i','o','n',0,
		'c','o','m','p','r','e','s','s','i','o','n',0,
		1,0,0,0,
		0, // no compression
		// dataWindow
		'd','a','t','a','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// displayWindow
		'd','i','s','p','l','a','y','W','i','n','d','o','w',0,
		'b','o','x','2','i',0,
		16,0,0,0,
		0,0,0,0,0,0,0,0,
		ww&0xFF, (ww>>8)&0xFF, (ww>>16)&0xFF, (ww>>24)&0xFF,
		hh&0xFF, (hh>>8)&0xFF, (hh>>16)&0xFF, (hh>>24)&0xFF,
		// lineOrder
		'l','i','n','e','O','r','d','e','r',0,
		'l','i','n','e','O','r','d','e','r',0,
		1,0,0,0,
		0, // increasing Y
		// pixelAspectRatio
		'p','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0,
		'f','l','o','a','t',0,
		4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
		// screenWindowCenter
		's','c','r','e','e','n','W','i','n','d','o','w','C','e','n','t','e','r',0,
		'v','2','f',0,
		8,0,0,0,
		0,0,0,0, 0,0,0,0,
		// screenWindowWidth
		's','c','r','e','e','n','W','i','n','d','o','w','W','i','d','t','h',0,
		'f','l','o','a','t',0, 
    4,0,0,0,
		0,0,0x80,0x3f, // 1.0f

    'm','a','x','S','a','m','p','l','e','s','P','e','r','P','i','x','e','l',0,
    'i','n','t',0,
    4,0,0,0,
    (((num_of_deep_samples_per_pixel) >> 0) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 4) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 8) & 0xFF),
    (((num_of_deep_samples_per_pixel) >> 12) & 0xFF),

    'v','i','e','w','M','a','t','r','i','x',0,
    'm','4','4','f',0,
     64, 0, 0,0,
     (v[0] >>0)& 0xFF,(v[0] >>4)& 0xFF,(v[0] >>8)& 0xFF,(v[0] >>12)& 0xFF,
     (v[1] >>0)& 0xFF,(v[1] >>4)& 0xFF,(v[1] >>8)& 0xFF,(v[1] >>12)& 0xFF,
     (v[2] >>0)& 0xFF,(v[2] >>4)& 0xFF,(v[2] >>8)& 0xFF,(v[2] >>12)& 0xFF,
     (v[3] >>0)& 0xFF,(v[3] >>4)& 0xFF,(v[3] >>8)& 0xFF,(v[3] >>12)& 0xFF,
     (v[4] >>0)& 0xFF,(v[4] >>4)& 0xFF,(v[4] >>8)& 0xFF,(v[4] >>12)& 0xFF,
     (v[5] >>0)& 0xFF,(v[5] >>4)& 0xFF,(v[5] >>8)& 0xFF,(v[5] >>12)& 0xFF,
     (v[6] >>0)& 0xFF,(v[6] >>4)& 0xFF,(v[6] >>8)& 0xFF,(v[6] >>12)& 0xFF,
     (v[7] >>0)& 0xFF,(v[7] >>4)& 0xFF,(v[7] >>8)& 0xFF,(v[7] >>12)& 0xFF,
     (v[8] >>0)& 0xFF,(v[8] >>4)& 0xFF,(v[8] >>8)& 0xFF,(v[8] >>12)& 0xFF,
     (v[9] >>0)& 0xFF,(v[9] >>4)& 0xFF,(v[9] >>8)& 0xFF,(v[9] >>12)& 0xFF,
     (v[10] >>0)& 0xFF,(v[10] >>4)& 0xFF,(v[10] >>8)& 0xFF,(v[10] >>12)& 0xFF,
     (v[11] >>0)& 0xFF,(v[11] >>4)& 0xFF,(v[11] >>8)& 0xFF,(v[11] >>12)& 0xFF,
     (v[12] >>0)& 0xFF,(v[12] >>4)& 0xFF,(v[12] >>8)& 0xFF,(v[12] >>12)& 0xFF,
     (v[13] >>0)& 0xFF,(v[13] >>4)& 0xFF,(v[13] >>8)& 0xFF,(v[13] >>12)& 0xFF,
     (v[14] >>0)& 0xFF,(v[14] >>4)& 0xFF,(v[14] >>8)& 0xFF,(v[14] >>12)& 0xFF,
     (v[15] >>0)& 0xFF,(v[15] >>4)& 0xFF,(v[15] >>8)& 0xFF,(v[15] >>12)& 0xFF,

    'p','r','o','j','e','c','t','i','o','n','M','a','t','r','i','x',0,
    'm','4','4','f',0,
     64, 0, 0,0,
     (p[0] >>0)& 0xFF,(p[0] >>4)& 0xFF,(p[0] >>8)& 0xFF,(p[0] >>12)& 0xFF,
     (p[1] >>0)& 0xFF,(p[1] >>4)& 0xFF,(p[1] >>8)& 0xFF,(p[1] >>12)& 0xFF,
     (p[2] >>0)& 0xFF,(p[2] >>4)& 0xFF,(p[2] >>8)& 0xFF,(p[2] >>12)& 0xFF,
     (p[3] >>0)& 0xFF,(p[3] >>4)& 0xFF,(p[3] >>8)& 0xFF,(p[3] >>12)& 0xFF,
     (p[4] >>0)& 0xFF,(p[4] >>4)& 0xFF,(p[4] >>8)& 0xFF,(p[4] >>12)& 0xFF,
     (p[5] >>0)& 0xFF,(p[5] >>4)& 0xFF,(p[5] >>8)& 0xFF,(p[5] >>12)& 0xFF,
     (p[6] >>0)& 0xFF,(p[6] >>4)& 0xFF,(p[6] >>8)& 0xFF,(p[6] >>12)& 0xFF,
     (p[7] >>0)& 0xFF,(p[7] >>4)& 0xFF,(p[7] >>8)& 0xFF,(p[7] >>12)& 0xFF,
     (p[8] >>0)& 0xFF,(p[8] >>4)& 0xFF,(p[8] >>8)& 0xFF,(p[8] >>12)& 0xFF,
     (p[9] >>0)& 0xFF,(p[9] >>4)& 0xFF,(p[9] >>8)& 0xFF,(p[9] >>12)& 0xFF,
     (p[10] >>0)& 0xFF,(p[10] >>4)& 0xFF,(p[10] >>8)& 0xFF,(p[10] >>12)& 0xFF,
     (p[11] >>0)& 0xFF,(p[11] >>4)& 0xFF,(p[11] >>8)& 0xFF,(p[11] >>12)& 0xFF,
     (p[12] >>0)& 0xFF,(p[12] >>4)& 0xFF,(p[12] >>8)& 0xFF,(p[12] >>12)& 0xFF,
     (p[13] >>0)& 0xFF,(p[13] >>4)& 0xFF,(p[13] >>8)& 0xFF,(p[13] >>12)& 0xFF,
     (p[14] >>0)& 0xFF,(p[14] >>4)& 0xFF,(p[14] >>8)& 0xFF,(p[14] >>12)& 0xFF,
     (p[15] >>0)& 0xFF,(p[15] >>4)& 0xFF,(p[15] >>8)& 0xFF,(p[15] >>12)& 0xFF,


   	't','y','p','e',0,
		's','t','r','i','n','g',0,
		 0x0C,0,0,0,
    'd','e','e','p','s','c','a','n','l','i','n','e',//0,

    'v','e','r','s','i','o','n',0,
    'i','n','t',0,
    4,0,0,0,
    1,0,0,0,
		// end of header
		0,
	};
	i32 hdr_size = array_count(hdr_data);
    u32 *row_sizes = ALLOC(sizeof(u32) * width * height);
    for (u32 i = 0; i < height; ++i)
    {
        u32 samples = 0;
        for (u32 j = 0; j <width; ++j)
        {
            samples += samples_per_pixel[i * width + j]; 
        }
        row_sizes[i] = samples;//*sizeof(DeepPixel);
    }


	i32 scanline_table_size = sizeof(u64) * height;

    u32 row_size_sum = 0;
    for (u32 i = 0; i < height; ++i)
    {
        row_size_sum += row_sizes[i] * sizeof(DeepPixel); 
        row_size_sum += 4 * width + 28;
    }

	u32 buf_size = hdr_size + row_size_sum + scanline_table_size;
    //TODO: write in custom allocator?!!?!
	u8* buf = (u8*)malloc (buf_size);
	if (!buf)
		return NULL;

	// copy in header
	memcpy (buf, hdr_data, hdr_size);

	// line offset table
	u32 index = hdr_size + scanline_table_size;
	u8* ptr = buf + hdr_size;
	for (i32 y = 0; y < height; ++y)
	{
        //u32 row_size_i = calc_row_size_i(pp, i);
		*ptr++ = index & 0xFF;
		*ptr++ = (index >> 8) & 0xFF;
		*ptr++ = (index >> 16) & 0xFF;
		*ptr++ = (index >> 24) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		index += row_sizes[y] * sizeof(DeepPixel) + 4*width + 28; //TODO(iv): SUSPECT GUY 
	}

	// scanline data (channels MUST be in alphabetical order!)
  /*each deep scanline must have:
     - y coordinate (int)
     - packed size of pixel offset table (u64)
     - packed size of sample data (u64)
     - unpacked size of sample data (u64)
     - compressed pixel offset table (ints??)
     - compressed sample data (sizeof(DeepData))
  */
	u8* src = (u8*)pixels;
	i32 stride = sizeof(DeepPixel);
  //we do this because the packed size is a list of ints
	for (i32 y = 0; y < (int)height; ++y)
	{

		// y coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
        //packed size of pixel offset table
        u32 offset_table_size = width * sizeof(int);
        *ptr++ = (offset_table_size >> 0) & 0xFF;
        *ptr++ = (offset_table_size >> 8) & 0xFF;
        *ptr++ = (offset_table_size >> 16) & 0xFF;
        *ptr++ = (offset_table_size >> 24) & 0xFF;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
		// packed size of sample data 
        u32 packed_size = row_sizes[y] * sizeof(DeepPixel);
		*ptr++ = (packed_size >> 0)& 0xFF;
		*ptr++ = (packed_size >> 8)& 0xFF;
		*ptr++ = (packed_size >> 16)& 0xFF;
		*ptr++ = (packed_size >> 24)& 0xFF;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        // unpacked size of sample data
        *ptr++ = (packed_size >> 0)& 0xFF;
        *ptr++ = (packed_size >> 8)& 0xFF;
        *ptr++ = (packed_size >> 16)& 0xFF;
        *ptr++ = (packed_size >> 24)& 0xFF;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
        *ptr++ = 0;
    //compressed pixel offset table (each deep pixel has num_of_.. samples per pixel!)
    i32 row_sum = 0;
    for (i32 i = 0; i < width; ++i)
    {
      row_sum += samples_per_pixel[y * width + i];
      *ptr++ = (row_sum >> 0) & 0xFF;
      *ptr++ = (row_sum >> 8) & 0xFF;
      *ptr++ = (row_sum>> 16) & 0xFF;
      *ptr++ = (row_sum >> 24) & 0xFF;
    }
    //compressed sample data
		// R G B A Z(ABGRZ ya mean) data for each deep pixel
		u8* chsrc;
		chsrc = src + 0;
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];

			chsrc += stride;
		}
    chsrc = src + 4;
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
    chsrc = src + 8; //ok
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];

			chsrc += stride;
		}
		chsrc = src + 12;
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
		chsrc = src + 16;
		for (i32 x = 0; x < row_sizes[y]; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}


		src += sizeof(DeepPixel) * row_sizes[y];
	}
  FILE* f = fopen ("deep.exr", "wb");
  fwrite (buf, 1, buf_size, f);
  //fwrite (buf, 1, buf_size, f);
  fclose (f);

	return buf;
}


internal u8* openexr_screenshot(void)
{
            i32 image_width = global_platform.window_width;
            i32 image_height = global_platform.window_height;
            //this is actually an f16 but I have implemented no such type :(
            u32 *rgba = (u32*)ALLOC(sizeof(u32) * 4 * global_platform.window_width* global_platform.window_height); 
            glReadPixels(0, 0, global_platform.window_width,global_platform.window_height,
                    GL_RGBA, GL_FLOAT, rgba);
            i32 exr_size; //in bytes
            u8 *exr = openexr_write(image_width, image_height , 4, rgba, &exr_size);
            FILE *f = fopen ("test.exr", "wb");
            fwrite (exr, 1, exr_size, f);
            fclose (f);
            free (exr); 
}

internal FILE *file_find(FILE *file, const char *str)
{
    char current[256];
    u32 current_index = 0;
    char ch;
    while(TRUE) {
       ch = fgetc(file);
       if( feof(file) ) {
          break;
       }
       if (str[current_index] == ch)current[current_index++] = ch;
       if (strcmp(str, current) == 0)
       {
           fseek(file, ftell(file) - current_index, SEEK_SET);
           //sprintf(error_log, "string %s found at position %i",current, ftell(file));
           return file;
       }
    } 
    return NULL;
}
internal FILE *file_forward(FILE *file, u32 offset)
{
   fseek(file, ftell(file) + offset , SEEK_SET);
}
internal i32 file_read_int(FILE *file)
{
    i32 n[4];
    n[0] = (fgetc(file) & 0xff);
    n[1] = (fgetc(file) & 0xff);
    n[2] = (fgetc(file) & 0xff);
    n[3] = (fgetc(file) & 0xff);
    n[0] = (n[0]<<0) | (n[1]<<8) | (n[2]<<16) | (n[3]<<24);
    return n[0];
}
internal u32 file_read_uint(FILE *file)
{
    u32 n[4];
    n[0] = (fgetc(file) & 0xff);
    n[1] = (fgetc(file) & 0xff);
    n[2] = (fgetc(file) & 0xff);
    n[3] = (fgetc(file) & 0xff);
    n[0] = (n[0]<<0) | (n[1]<<8) | (n[2]<<16) | (n[3]<<24);
    return n[0];
}
internal f32 file_read_float(FILE *file)
{
    f32 f;
    u32 *f_ptr = (u32*)&f;
    u8 n[4];
    n[0] = (fgetc(file) & 0xff);
    n[1] = (fgetc(file) & 0xff);
    n[2] = (fgetc(file) & 0xff);
    n[3] = (fgetc(file) & 0xff);
    *f_ptr = (n[0]<<0) | (n[1]<<8) | (n[2]<<16) | (n[3]<<24);
    return f;
}



internal RendererPointData *deepexr_read(const char * filename, u32 *point_count)
{
    RendererPointData *points = ALLOC(sizeof(RendererPointData) * 100000000); 
    u32 point_index = 0;
    mat4 view;
    mat4 proj;
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    file_find(file, "box2i");
    file_forward(file, str_size("box2i"));
    file_forward(file, 13);
    u32 total_row_size = 0;
    //HEADER -- here we read header attrbutes --
    i32 ww = file_read_int(file);
    i32 wh = file_read_int(file);
    //find the view matrix
    file_find(file, "viewMatrix");
    file_forward(file, str_size("viewMatrix"));
    file_forward(file, 10); //@TODO put _actual_ value here
    for (u32 i = 0; i < 16; ++i)
        view.raw[i] = file_read_float(file);
    //sprintf(error_log, "view = %f %f %f %f", view.raw[0], view.raw[1],view.raw[2],view.raw[3]);
    //find the projection matrix
    file_find(file, "projectionMatrix");
    file_forward(file, str_size("projectionMatrix"));
    file_forward(file, 10); //@TODO put _actual_ value here
    for (u32 i = 0; i < 16; ++i)
        proj.raw[i] = file_read_float(file);
    file_find(file, "version");
    file_forward(file, str_size("version"));
    file_forward(file, 14);
    //OFFSET TABLE -- here we read the offset table --
    //offset table = distance of each scanline from start of file!
    u32 *offsets = ALLOC(sizeof(u32) * (wh+1));
    for (u32 i = 0; i < wh+1; ++i)
    {
        offsets[i] = file_read_uint(file);
        file_read_int(file);
    }
    //DEEP DATA -- here we read the deep fragments
    //each scanline is of the type  AA..ABB..BGG..GRR..RZZ..Z
    DeepPixel *pixels = ALLOC(ww * wh *10*sizeof(DeepPixel));
    f32 *input = ALLOC(ww * 10 * sizeof(float));
    i32 *spp = ALLOC(sizeof(i32) * (ww + 1));
    for (u32 i = 0; i < wh+1; ++i)
    {
        //we go where scanline i begins
        rewind(file);
        fseek(file, offsets[i], SEEK_SET);
        //we read y coord (i32)
        i32 y = file_read_int(file);
        //we read packed size of pixel offset table (u64)
        u32 size_of_pixel_table = file_read_uint(file);
        file_read_uint(file);
        //we read packed size of sample data (u64)
        u32 unpacked = file_read_uint(file);
        file_read_uint(file);
        //we read unpacked size of sample data (u64)
        u32 size_of_sample_data = file_read_uint(file);
        file_read_uint(file);
        //we read "compressed" pixel offset table (i32 arr??) 
        u32 row_size = 0;
        for (u32 j = 0; j < ww + 1; ++j)
        {
            spp[j] = file_read_int(file);
            //last spp is the row_size
            row_size = spp[j];
        }
        total_row_size += row_size;
        for (u32 j = ww; j > 0; --j)
        {
            spp[j] = spp[j] - spp[j-1];
        }
        //we read "compressed" sample data (DeepPixel- per component)
        for (u32 j = 0; j < row_size; ++j)
        {
            pixels[j].a = file_read_float(file);
        }
        for (u32 j =0; j< row_size; ++j)
        {
            pixels[j].b = file_read_float(file);
        }
        for (u32 j = 0; j < row_size; ++j)
        {
            pixels[j].g = file_read_float(file);
        }
        for (u32 j = 0; j < row_size; ++j)
        {
            pixels[j].r = file_read_float(file);
        }
        for (u32 j = 0; j < row_size; ++j)
        {
            pixels[j].z = file_read_float(file);
        }
        u32 pixel_counter = 0;
        for (u32 j = 0; j < wh+1; ++j)
        {
            for (u32 k = 0; k < spp[j]; ++k)
            {
                    f32 yf = (1.f - ((f32)i/(f32)wh+1)) * 5;
                    f32 xf = ((f32)j/(f32)ww+1) * 5;
                    f32 depth = -pixels[pixel_counter].z;
                    points[point_index] = (RendererPointData){v3(xf,yf,depth), v4(pixels[pixel_counter].r,pixels[pixel_counter].g,pixels[pixel_counter].b,1.f)};//pixels[pixel_counter].a)};
                    point_index++;
                    pixel_counter++;
            }
        }
    }
    fclose(file);
    FREE(pixels);
    FREE(input);
    FREE(offsets);
    FREE(spp);
    *point_count = point_index;
    return points;
}





#endif
