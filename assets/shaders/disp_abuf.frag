#version 430
//#include "define.h"
//#include "data_structs.h"
//#include "sort.h"
struct NodeTypeLL
{
	float depth;
	uint color;
	uint next;
};
#define LOCAL_SIZE 32
vec2 fragments [LOCAL_SIZE];

void sort_insert(const int num)
{
	for (int j = 1; j < num; ++j)
	{
		vec2 key = fragments[j];
		int i = j - 1;

		while (i >= 0 && fragments[i].g > key.g)
		{
			fragments[i+1] = fragments[i];
			--i;
		}
		fragments[i+1] = key;
	}
}

void sort(const int num)
{
	sort_insert(num);
}
// Input Variables
uniform int	layer;

//layout(binding = 0, r32ui)	
//coherent uniform uimage2DRect in_image_head;

layout(binding = 0, r32ui)		
coherent uniform uimage2D  in_image_head;

layout(binding = 1, std430)		
coherent buffer  LinkedLists   
{ 
NodeTypeLL nodes[]; 
};

// Output Variables
layout(location = 0, index = 0) out vec4 out_frag_color;

layout(pixel_center_integer) in vec4 gl_FragCoord;
void main(void)
{
	// Get head pixel pointer
	uint index = imageLoad(in_image_head, ivec2(gl_FragCoord.xy)).x;
	if(index == 0u)
		discard;

	// Alloc optionally, used for fixing next fragment pointers
	uint fragments_id[LOCAL_SIZE];

	// Store fragment data values to a local array
	int count = 0;
	while (index != 0u)
	{
		fragments_id[count] = index; // Set optionally
		fragments[count++]  = vec2(uintBitsToFloat(nodes[index].color), nodes[index].depth);
		index			    = nodes[index].next;
	}
	
	// Sort fragments by their depth
    sort(count);

/*
	// Use optionally if you want to perform subsequent operations in a following pass
	{
		// Set head pixel pointer
		imageStore(in_image_head, ivec2(gl_FragCoord.xy), uvec4(fragments[0].g, 0.0u, 0.0u, 0.0u));
			
		// Correct fragment connection pointers
		for(int i=0; i<count-1; i++)
			nodes[fragments_id[i]].next = fragments_id[i+1];
		nodes[fragments_id[counter-1]].next = 0U;
	}
*/

	// Return the color value of the selected fragment
   	out_frag_color = unpackUnorm4x8(floatBitsToUint(fragments[layer].r));
}