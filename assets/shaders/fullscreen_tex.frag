#version 330 core
out vec4 FragColor;

in vec2 f_tex_coord;
uniform sampler2D sampler;

void main()
{
    FragColor = vec4(vec3(texture(sampler, f_tex_coord)), 1.0);
    FragColor.a = 0.5;
}
