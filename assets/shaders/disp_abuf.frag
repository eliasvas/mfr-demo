#version 420
#define ABUFFER_SIZE 16
uniform int screen_width;
uniform int screen_height;

layout(pixel_center_integer) in vec4 gl_FragCoord;

smooth in vec4 f_pos;

out vec4 out_frag_color;

uniform layout(size1x32) uimage2D counter_img;
uniform layout(size4x32) uimage2DArray abuf_img;

const float fragmentAlpha=0.5f;
const vec4 backgroundColor = vec4(1,1,1,0);
//local memory array
vec4 fragment_list[ABUFFER_SIZE];

void bubble_sort(int array_size) {
  for (int i = (array_size - 2); i >= 0; --i) {
    for (int j = 0; j <= i; ++j) {
      if (fragment_list[j].w > fragment_list[j+1].w) {
		vec4 temp = fragment_list[j+1];
		fragment_list[j+1] = fragment_list[j];
		fragment_list[j] = temp;
      }
    }
  }
}
//fill the array with fragments from the 3D image (2D array)
void fill_frag_array(ivec2 coords, int abNumFrag){
	//Load fragments into a local memory array for sorting
	for(int i=0; i<abNumFrag; i++){
		fragment_list[i]=imageLoad(abuf_img, ivec3(coords.x,coords.y, i));

	}
}
//keep only the closest fragments
vec4 resolve_closest(ivec2 coords, int abNumFrag){

	//Search smallest z
	vec4 minFrag=vec4(0.0f, 0.0f, 0.0f, 1000000.0f);
	for(int i=0; i<abNumFrag; i++){
		vec4 val=imageLoad(abuf_img, ivec3(coords, i));
		if(val.w<minFrag.w){
			minFrag=val;
		}
	}
	//Output final color for the frame buffer
	return minFrag;
}

void main(void)
{
    ivec2 coords = ivec2(gl_FragCoord.xy);
    if (coords.x >= 0 && coords.y >= 0 && coords.x < screen_width && coords.y < screen_height)
    {
        int ab_num_frag = int(imageLoad(counter_img, coords).r);
        /*
        if (ab_num_frag < 0)
            ab_num_frag = 0;
        if (ab_num_frag > ABUFFER_SIZE)
            ab_num_frag = ABUFFER_SIZE;
        */
        if (ab_num_frag > 0)
		    out_frag_color = resolve_closest(coords,ab_num_frag);
			//out_frag_color = resolve_alpha_blend(coords, ab_num_frag);
    }else{discard;}

}

