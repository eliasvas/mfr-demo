#ifndef EXR_WRITE_H
#define EXR_WRITE_H
#include "tools.h"
#include "stdio.h"
//@TODO make it stb style (with EXR_WRITE_IMPL and stuff)

typedef struct NodeTypeLL
{
	f32 depth;
	u32 color;
	u32 next;
}NodeTypeLL;

//the xy coordinates are given by the file
typedef struct DeepPixel
{
    f32 a; //its for all the DeepPixel, no layers needed!
    f32 b;
    f32 g;
    f32 r;
    f32 z;
}DeepPixel;

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

#define DEEP_IMAGE_VERSION_BITMASK 0x800

static u32
count_bytes_in_file(FILE *file)
{
    u32 bytes = 0;
    for(bytes = 0; getc(file) != EOF; ++bytes);

}

u8* openexr_write(u32 width, u32 height, u32 channels,f32* data, i32* outSize)
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


u8* openexr_write_half(u32 width, u32 height, u32 channels,void* rgba16f, i32* outSize)
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

#if 1
u8 *deepexr_write(u32 width, u32 height,DeepPixel *pixels, u32 pixels_count, u32 num_of_deep_samples_per_pixel)
{
  //width = global_platform.window_width;
  //height = global_platform.window_height;
	u32 ww = width-1;
	u32 hh = height-1;
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		0x02, 0x08, 0, 0, // version, DEEP scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'A',0, 0x02,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, FLOAT 
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
    //@TODO DEEP IMAGE check these!!! it must be little endian!!
    // samples per pixel
    /*
    'm','a','x','S','a','m','p','l','e','s','P','e','r','P','i','x','e','l',0,
    'i','n','t',0,
    0xFF,0xFF,0xFF,0xFF, //this is -1 in 2s complement
    //deep image type attribute and version (must always be set to 1)
    'n','a','m','e',0,
		's','t','r','i','n','g',0,
		 0x0C,0,0,0,
    'd','e','e','p','_','i','m','a','g','e','0','1',//0,
    */

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
	for (i32 y = 0; y < height; ++y)
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
		*ptr++ = pixel_row_size_long& 0xFF;
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
      *ptr++ = (0) & 0xFF;
      *ptr++ = ((0) >> 8) & 0xFF;
      *ptr++ = ((0) >> 16) & 0xFF;
      *ptr++ = ((0) >> 24) & 0xFF;
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
    chsrc = src + 4;
		for (i32 x = 0; x < width* num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}
    chsrc = src + 8;
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
		chsrc = src + 24;
		for (i32 x = 0; x < width* num_of_deep_samples_per_pixel; ++x)
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			*ptr++ = chsrc[2];
			*ptr++ = chsrc[3];
			chsrc += stride;
		}

		src += width * stride;
	}
  FILE* f = fopen ("deep_image01.exr", "wb");
  fwrite (buf, 1, buf_size, f);
  //fwrite (buf, 1, buf_size, f);
  fclose (f);

	return buf;
}
#endif

u8* openexr_screenshot(void)
{
            i32 image_width = global_platform.window_width;
            i32 image_height = global_platform.window_height;
            //this is actually an f16 but I have implemented no such type :(
            u32 *rgba = (u32*)ALLOC(sizeof(u32) * 4 * global_platform.window_width* global_platform.window_height); 
            glReadPixels(0, 0, global_platform.window_width,global_platform.window_height,
                    GL_RGBA, GL_FLOAT, rgba);
            i32 exr_size; //in bytes
            u8* exr = openexr_write(image_width, image_height , 4, rgba, &exr_size);
            FILE* f = fopen ("test.exr", "wb");
            fwrite (exr, 1, exr_size, f);
            fclose (f);
            free (exr); 
}





#endif
