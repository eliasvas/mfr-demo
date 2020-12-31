#version 460
//#include "define.h"
//#include "data_structs.h"
//#include "sort.h"
struct NodeTypeLL
{
	float depth;
	float red;
	float green;
	float blue;
	float alpha;
	uint next;
};
#define LOCAL_SIZE 32

vec4 fragments [LOCAL_SIZE];
float fragments_z [LOCAL_SIZE];
const vec3 cool = vec3(0.2,0.1,0.5);
const vec3 warm = vec3(0.9,0.2,0.1);



float fragment_alpha = 0.5;

//layout(binding = 0, r32ui)	
//coherent uniform uimage2DRect in_image_head;

layout(binding = 0, r32ui)		
coherent uniform uimage2D  in_image_head;

layout(binding = 1, std430)		
coherent buffer  LinkedLists   
{ 
NodeTypeLL nodes[]; 
};


layout(location = 0, index = 0) out vec4 out_frag_color;


void sort_insert(const int num)
{
	for (int j = 1; j < num; ++j)
	{
		float key = fragments_z[j];
		vec4 key_color = fragments[j];
		int i = j - 1;

		while (i >= 0 && fragments_z[i] > key)
		{
			fragments[i+1].r = fragments[i].r;
			fragments[i+1].g = fragments[i].g;
			fragments[i+1].b = fragments[i].b;
			fragments[i+1].a = fragments[i].a;
			fragments_z[i+1] = fragments_z[i];
			--i;
		}
		fragments_z[i+1] = key;
		fragments[i+1].r = key_color.r;
		fragments[i+1].g = key_color.g;
		fragments[i+1].b = key_color.b;
		fragments[i+1].a = key_color.a;
	}
}

void sort(const int num)
{
	sort_insert(num);
}
vec4 resolve_alpha_blend(ivec2 coords, int ab_num_frag){
		
	vec4 final_color=vec4(0.0f);

	final_color=vec4(0.0f);
	for(int i=0; i<ab_num_frag; i++){
		vec4 frag= vec4(fragments[i].r, fragments[i].g,fragments[i].b,fragments[i].a);
		
		vec4 col;
		col.rgb=frag.rgb;
		//col.rgb = (float(ab_num_frag)/8) * warm + (1.f - float(ab_num_frag)/8) * cool;
		col.w=fragment_alpha;	//uses constant alpha

		col.rgb=col.rgb*col.w;

		final_color=final_color+col*(1.0f-final_color.a);
	}

	//final_color=final_color+background_color*(1.0f-final_color.a);

	return final_color;

}




layout(pixel_center_integer) in vec4 gl_FragCoord;
void main(void)
{
	// Get head pixel pointer
	uint index = imageLoad(in_image_head, ivec2(gl_FragCoord.xy)).x;
	if(index == 0u)
		discard;


	// Store fragment data values to a local array
	int count = 0;
	while (index != 0u)
	{
		fragments[count++]  = vec4(nodes[index].red, nodes[index].green, nodes[index].blue, nodes[index].alpha);
		fragments_z[count] = nodes[index].depth;
		index			    = nodes[index].next;
	}
	
    sort(count);

	if (count == 0)discard;
	// Return the color value of the selected fragment
   	//out_frag_color = unpackUnorm4x8(floatBitsToUint(fragments[layer].r));
	out_frag_color = resolve_alpha_blend(ivec2(gl_FragCoord.xy), count);
	//if (out_frag_color.x < 0.1)discard;
	//out_frag_color.a = 1.0;
}