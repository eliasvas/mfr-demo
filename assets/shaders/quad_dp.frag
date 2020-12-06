#version 330 core
out vec4 FragColor;
  
in vec4 vertexColor;
in vec2 f_tex_coord;
smooth in vec4 f_frag_pos_ls;
uniform sampler2D sampler;
uniform sampler2D depth_buffer;
uniform sampler2D shadowMap;

uniform int window_width;
uniform int window_height;
uniform int peel;
float e = 0.001;

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
			shadow += currentDepth - e > pcf_depth ? 1.0 : 0.0; //if 0, stay 0, if else add the adjacent shadow tiles
		}
		shadow /= 9.0;
		shadow *=2;
	}
	
	//make the shadow 0 if it is outside the far plane
	if (projCoords.z > 1.0)
		shadow = 1.0;
	
	return shadow;
}
//layout(pixel_center_integer) in vec4 gl_FragCoord;
void main()
{

	vec2 pos_in_tex = gl_FragCoord.xy / vec2(window_width,window_height);
	FragColor = texture(sampler,f_tex_coord);
	FragColor.a = 1.0;
	
	float shadow = ShadowCalculation(f_frag_pos_ls);       
    FragColor.xyz = (vec3(0.2,0.2,0.2) + (1.0 - shadow) * vec3(0.7,0.7,0.7)) * vec3(FragColor.xyz);
	
	
	//this means that there is something behind the quad (the depth is 0)
	if (texture(depth_buffer,pos_in_tex).x < 0.01)
		discard;

	if (peel)
		if (gl_FragCoord.z <= texture(depth_buffer,pos_in_tex).x)
			discard;
	
} 
