#version 430 core
out vec4 FragColor;

in vec2 f_tex_coord;
in vec3 f_frag_pos;
in vec3 f_frag_pos_ws;
in vec3 f_normal;
in vec4 f_frag_pos_ls;
uniform mat4 proj;
uniform mat4 view;
uniform int deep_render;
uniform vec3 view_front;
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    //vec3 ambient;
    //vec3 diffuse;
    //vec3 specular;
    float shininess;
}; 
struct NodeTypeLL
{

	float alpha;
	float blue;
	float green;
	float red;
	float depth;
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

struct DirLight
{
   vec3 direction;

   vec3 ambient;
   vec3 diffuse;
   vec3 specular;
};
struct PointLight
{
   vec3 position;

   vec3 ambient;
   vec3 diffuse;
   vec3 specular;
};
#define MAX_POINT_LIGHTS 32
uniform vec3 view_pos;
uniform Material material;
uniform PointLight point_lights[MAX_POINT_LIGHTS];
uniform DirLight dirlight;
uniform int point_light_count;
uniform sampler2D shadow_map;

float linearize_depth(float d)
{
	float A = proj[2].z;
    float B = proj[3].z;
    float zNear = - B / (1.0 - A);
    float zFar  =   B / (1.0 + A);
	float ndc = d * 2.0 - 1.0;
	return (2.0 * zNear * zFar) / (zFar + zNear - ndc * (zFar - zNear));	 
}
float shadow_calc()
{
	float bias = 0.001;
	// perform perspective divide
    vec3 proj_coords = f_frag_pos_ls.xyz / f_frag_pos_ls.w;
    // transform to [0,1] range
    proj_coords = proj_coords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closest_depth = texture(shadow_map, proj_coords.xy).r; 
    // get depth of current fragment from light's perspective
    float current_depth = proj_coords.z;
    // check whether current frag pos is in shadow
    //float shadow = current_depth - bias > closest_depth  ? 1.0 : 0.0;
	
	float shadow = 0.0;
	vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
	for(int x = -1; x <= 1; ++x)
	{
    for(int y = -1; y <= 1; ++y)
		{
			float pcf_depth = texture(shadow_map, proj_coords.xy + vec2(x, y) * texel_size).r; 
			shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;

    return 1.0 - shadow;
}


void main()
{
	float kd = 1.0;
	float a = 0.2;
	float b = 0.6;
	vec3 n = normalize(f_normal);
	vec3 light_dir = normalize(-dirlight.direction);
	vec3 view_dir = normalize(view_pos - f_frag_pos_ws);
	vec3 reflect_dir = reflect(-light_dir, n);
	vec3 color;
	vec3 surface = vec3(0.8,0.5,0.4);
	vec3 warm = vec3(0.6,0.4,0.0) + 0.25 * surface;
	vec3 cool = vec3(0,0,0.35) + 0.25 * surface;
	float NL = dot(n, light_dir);
	float k = (1 + NL) / 2.0;
	vec3 gc = k * warm + (1-k) * cool;
	float spec = pow(max(dot(view_dir, reflect_dir),0.0),32);
	color = spec + (1-spec)*gc;
		float constant = 1.f;
	float linear = 0.09;
	float quadratic = 0.032;
	for(int i = 0; i < point_light_count;++i)
	{
		n = normalize(f_normal);
		light_dir = normalize(point_lights[i].position - f_frag_pos_ws);
		view_dir = normalize(view_pos - f_frag_pos_ws);
		reflect_dir = reflect(-light_dir, n);
		
		float NL = dot(n, light_dir);
		float k = (1 + NL) / 2.0;
		vec3 gc = k * warm + (1-k) * cool;
		float spec = pow(max(dot(view_dir, reflect_dir),0.0),32);
		vec3 kolor = spec + (1-spec)*gc;
		float distance = abs(length(point_lights[i].position - f_frag_pos_ws));
		float attenuation = 1.0/(constant + linear * distance + quadratic*(distance*distance));
		attenuation = 1.0/(distance);
		kolor *= attenuation;
		/*
		float attenuation = 1.0/(distance);
		ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;
		*/
		color += kolor;
	}
	FragColor = vec4(color,texture(material.diffuse,f_tex_coord).a);
	{
		ivec2 coords = ivec2(gl_FragCoord.xy);

	// get next available location in global buffer
	uint index = atomicCounterIncrement(in_next_address) + 1U;

	if(index < nodes.length())
	{
		//its not used rn
		vec4 color = FragColor;
		//color.a = 0.9;
		
		nodes[index].red = color.r;
		nodes[index].green = color.g;
		nodes[index].blue = color.b;
		nodes[index].alpha = color.a;
		if (deep_render > 0)
		{
			float A = proj[2].z;
			float B = proj[3].z;
			float zNear = (B + 1.0) / A;
			float zFar  =  (B - 1.0) / A;
			float t = (gl_FragCoord.z + 1.0) / 2.0;
			nodes[index].depth = zNear + t * (zFar - zNear);
		}
		else{
			nodes[index].depth = f_frag_pos.z;
		}
		nodes[index].next  = imageAtomicExchange(in_image_head, ivec2(gl_FragCoord.xy), index);
	}
	discard;
	}
}
