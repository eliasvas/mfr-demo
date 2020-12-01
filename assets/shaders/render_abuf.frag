#version 430

struct NodeTypeLL
{
	float depth;
	uint color;
	uint next;
};
//layout(binding = 0, r32ui)		
//coherent uniform uimage2DRect  in_image_head;

layout(binding = 0, r32ui)		
coherent uniform uimage2D  in_image_head;


layout(binding = 1, std430)		
coherent buffer  LinkedLists   
{ 
NodeTypeLL nodes[]; 
};

layout(binding = 2, offset = 0)
uniform atomic_uint   in_next_address;

uniform sampler2D diffuse_map;
uniform sampler2D shadowMap;

smooth in vec4 f_pos;
smooth in vec2 f_tex_coord;
smooth in vec3 f_normal;
smooth in vec4 f_frag_pos_ls;
flat in int f_shadowmap_on;
float ShadowCalculation(vec4 fragPosLightSpace)
{
	//perspective devide so we go to clip-space [-1,1]
	vec3 projCoords = f_frag_pos_ls.xyz / f_frag_pos_ls.w;
	//we transform to NDC so we go to [0,1]
	projCoords = projCoords * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, projCoords.xy).r;   
	float currentDepth = projCoords.z;  
	
	float shadow = 0.0;
	vec2 texel_size = 1.0 / vec2(1024,720); //instead of vec2(1024,720) must be the size of the tilemap..
	for (int x = -1; x <=1; ++x)
	{
		for (int y = -1; y <=1; ++y)
		{
			float pcf_depth = texture(shadowMap, projCoords.xy + vec2(x, y) * texel_size).r;
			shadow += currentDepth - 0.0001 > pcf_depth ? 1.0 : 0.0; //if 0, stay 0, if else add the adjacent shadow tiles
		}
		shadow /= 9.0;
	}
	
	//make the shadow 0 if it is outside the far plane
	if (projCoords.z > 1.0)
		shadow = 0.0;
	
	return shadow;
}
vec4 computePixelColor()
{
	vec4 color = texture(diffuse_map, f_tex_coord);
	float shadow = ShadowCalculation(f_frag_pos_ls);       
    vec3 lighting = (vec3(0.2,0.2,0.2) + (1.0 - shadow) * vec3(0.7,0.7,0.7)) * vec3(color);
	return vec4(lighting, 1.0);
}
//layout(pixel_center_integer) in vec4 gl_FragCoord;
void main(void)
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

	// get next available location in global buffer
	uint index = atomicCounterIncrement(in_next_address) + 1U;

	if(index < nodes.length())
	{
		//its not used rn
		vec4 color = computePixelColor();
		color.a = 0.3;
		
		nodes[index].color = packUnorm4x8(color);
		nodes[index].depth = gl_FragCoord.z;
		nodes[index].next  = imageAtomicExchange(in_image_head, ivec2(gl_FragCoord.xy), index);
		
	}
	//if (f_shadowmap == 0)
		discard;
}
