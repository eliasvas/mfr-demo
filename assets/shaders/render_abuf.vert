#version 430

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
uniform mat4 invproj;
uniform mat4 invview;
uniform mat4 view_IT;
uniform mat4 lightSpaceMatrix;
uniform int shadowmap_on;

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_tex_coord;

smooth out vec4 f_pos;
smooth out vec2 f_tex_coord;
smooth out vec3 f_normal;
smooth out vec4 f_frag_pos_ls;
smooth out vec3 f_frag_pos_ws;
flat out int f_shadowmap_on;

void main()
{
   vec4 pos = proj * view * model * vec4(vertex_pos.xyz,1.0); 


   vec3 normal_eye = normalize((view_IT*vec4(vertex_normal, 1.0f)).xyz);

   f_tex_coord = vertex_tex_coord;
   //f_tex_coord.z = abs(normal_eye.z);

   f_normal = normal_eye;
   f_pos = pos;
   f_frag_pos_ls = lightSpaceMatrix * vec4(vec3(model * vec4(vertex_pos, 1.0)),1.0);
   gl_Position = pos;
   f_frag_pos_ws = vec3(model * vec4(vertex_pos,1.0));
   f_shadowmap_on = shadowmap_on;
}
