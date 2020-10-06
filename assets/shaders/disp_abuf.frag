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

//keeps only closest fragment
vec4 resolve_closest(ivec2 coords, int ab_num_frag);

//blend fragments front to back
vec4 resolve_alpha_blend(ivec2 coords, int ab_num_frag);

//compute gelly shader
vec4 resolve_gelly(ivec2 coords, int ab_num_frag);

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
            out_frag_color = resolve_alpha_blend(coords, ab_num_frag);
    }else{discard;}

}

void bubbleSort(int array_size) {
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

void swapFragArray(int n0, int n1){
	vec4 temp = fragment_list[n1];
	fragment_list[n1] = fragment_list[n0];
	fragment_list[n0] = temp;
}

void fillFragmentArray(ivec2 coords, int abNumFrag){
	//Load fragments into a local memory array for sorting
	for(int i=0; i<abNumFrag; i++){
		fragment_list[i]=imageLoad(abuf_img, ivec3(coords.x,coords.y, i));

	}
}

//Blend fragments front-to-back
vec4 resolve_aplha_blend(ivec2 coords, int abNumFrag){
	
	//Copy fragments in local array
	fillFragmentArray(coords, abNumFrag);

	//Sort fragments in local memory array
	bubbleSort(abNumFrag);
		
	vec4 finalColor=vec4(0.0f);


	const float sigma = 30.0f;
	float thickness=fragment_list[0].w/2.0f;

	finalColor=vec4(0.0f);
	for(int i=0; i<abNumFrag; i++){
		vec4 frag=fragment_list[i];
		
		vec4 col;
		col.rgb=frag.rgb;
		col.w=fragmentAlpha;	//uses constant alpha

#if ABUFFER_RESOLVE_ALPHA_CORRECTION
		if(i%2==abNumFrag%2)
			thickness=(fragment_list[i+1].w-frag.w)*0.5f;
		col.w=1.0f-pow(1.0f-col.w, thickness* sigma );
#endif

		col.rgb=col.rgb*col.w;

		finalColor=finalColor+col*(1.0f-finalColor.a);
	}

	finalColor=finalColor+backgroundColor*(1.0f-finalColor.a);

	return finalColor;

}
