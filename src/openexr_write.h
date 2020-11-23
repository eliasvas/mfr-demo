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
    u32 color; // R G B A
    f32 depth;
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

u8* openexr_write (u32 width, u32 height, u32 channels,void* rgba16f, i32* outSize)
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
	u8* src = (u8*)rgba16f;
	i32 stride = channels * 2;
	for (i32 y = 0; y < height; ++y)
	{
		// coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
		// data size
		*ptr++ = pixel_row_size & 0xFF;
		*ptr++ = (pixel_row_size >> 8) & 0xFF;
		*ptr++ = (pixel_row_size >> 16) & 0xFF;
		*ptr++ = (pixel_row_size >> 24) & 0xFF;
		// B, G, R
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
u8 *deepexr_write(u32 width, u32 height,f32 *image_head, NodeTypeLL *nodes,u32 nodes_count)
{
	u32 ww = width-1;
	u32 hh = height-1;
	u8 hdr_data[] = {
		0x76, 0x2f, 0x31, 0x01, // magic
		2, 0, 0, 1, // version, DEEP scanline
		// channels
		'c','h','a','n','n','e','l','s',0,
		'c','h','l','i','s','t',0,
		55,0,0,0,
		'B',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // B, FLOAT 
		'G',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // G, FLOAT 
		'R',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // R, FLOAT 
		'Z',0, 2,0,0,0, 0, 0,0,0,1,0,0,0,1,0,0,0, // Z, FLOAT 
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
		'f','l','o','a','t',0, 4,0,0,0,
		0,0,0x80,0x3f, // 1.0f
        //@TODO check these!!! it must be little endian!!
        // samples per pixel
        'm','a','x','S','a','m','p','l','e','s','P','e','r','P','i','x','e','l',0,
        255,255,255,255, //this is -1 in 2s complement
        //deep image version (must always be set to 1)
        'v','e','r','s','i','o','n',0,
        0,0,0,255, //this is 1 in 2s complement
		// end of header
		0,
	};
	i32 hdr_size = array_count(hdr_data);

	i32 scanline_table_size = 8 * height;
	u32 pixel_row_size = width * 3 * 2;
	u32 full_row_size = pixel_row_size + 8;

	u32 buf_size = hdr_size + scanline_table_size + height * full_row_size;
    //TODO: write in custom allocator?!!?!
	u8* buf = (u8*)malloc (buf_size);
	if (!buf)
		return NULL;

	// copy in header
	memcpy (buf, hdr_data, hdr_size);

    /*
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
	u8* src = (u8*)nodes;
	i32 stride = sizeof(NodeTypeLL);
	for (i32 y = 0; y < height; ++y)
	{
		// coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = (y >> 8) & 0xFF;
		*ptr++ = (y >> 16) & 0xFF;
		*ptr++ = (y >> 24) & 0xFF;
		// data size
		*ptr++ = pixel_row_size & 0xFF;
		*ptr++ = (pixel_row_size >> 8) & 0xFF;
		*ptr++ = (pixel_row_size >> 16) & 0xFF;
		*ptr++ = (pixel_row_size >> 24) & 0xFF;
		// B, G, R
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
    */

	return buf;
}
#endif

u8* openexr_screenshot(void)
{
            i32 image_width = global_platform.window_width;
            i32 image_height = global_platform.window_height;
            //this is actually an f16 but I have implemented no such type :(
            u16 *rgba = (u16*)ALLOC(sizeof(u16) * 4 * global_platform.window_width* global_platform.window_height); 
            glReadPixels(0, 0, global_platform.window_width,global_platform.window_height,
                    GL_RGBA, GL_HALF_FLOAT, rgba);
            i32 exr_size; //in bytes
            u8* exr = openexr_write (image_width, image_height , 4, rgba, &exr_size);
            FILE* f = fopen ("test.exr", "wb");
            fwrite (exr, 1, exr_size, f);
            fclose (f);
            free (exr); 
}





#endif
