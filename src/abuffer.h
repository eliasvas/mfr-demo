#ifndef ABUFFER_H
#define ABUFFER_H

#include "tools.h"
#include "shader.h"
#include "texture.h"
#include "platform.h"
//why 16 and not 4 ? maybe 16 is 4 X components(4)????
#define ABUFFER_SIZE 16
//these have to go and be replaced by a nice little struct!
static Shader clear_abuffer_shader;
static GLuint quad_vao, quad_vbo;

static Shader render_abuffer_shader;
static Shader render_abuffer_quad_shader;
//the quad way
static Shader display_abuffer_shader;
static GLuint abuf_tex_id, abuf_counter_tex_id;


//Full screen quad vertices definition
static GLfloat quad_verts[] = {
   -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, -1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f,    

   1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, 1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f  
};

extern mat4 view, proj;
extern vec4 background_color;
static void 
init_abuffer_shaders(void)
{
    shader_load(&clear_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/clear_abuf.frag");
    shader_load(&render_abuffer_shader,"../assets/shaders/render_abuf.vert","../assets/shaders/render_abuf.frag");
    shader_load(&render_abuffer_quad_shader,"../assets/shaders/render_abuf_quad.vert","../assets/shaders/render_abuf_quad.frag");
    shader_load(&display_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/disp_abuf.frag");
}


static void 
init_abuffer(void)
{
    init_abuffer_shaders();
		
    ///ABuffer storage///
    if(!abuf_tex_id)
        glGenTextures(1, &abuf_tex_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, abuf_tex_id);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Texture creation
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, global_platform.window_width, global_platform.window_height, ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
    glBindImageTexture(0, abuf_tex_id, 0, TRUE, 0,  GL_READ_WRITE, GL_RGBA32F);

    ///ABuffer per-pixel counter///
    if(!abuf_counter_tex_id)
        glGenTextures(1, &abuf_counter_tex_id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, abuf_counter_tex_id);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Texture creation
    //Uses GL_R32F instead of GL_R32I that is not working in R257.15
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    glBindImageTexture(1, abuf_counter_tex_id, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??

}
static void 
draw_quad(Shader *shader) 
{

    use_shader(shader);

    if (!quad_vao)
        glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    if (!quad_vbo)
        glGenBuffers(1, &quad_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
	glVertexAttribPointer (0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, 0);
    setInt(shader, "screen_width", global_platform.window_width);
    setInt(shader, "screen_height", global_platform.window_height);
    glBindImageTexture(0, abuf_tex_id, 0, TRUE, 0,  GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, abuf_counter_tex_id, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
    setInt(shader, "abuf_img",0);
    setInt(shader, "counter_img",1);

	glDrawArrays(GL_TRIANGLES, 0, 24);
    glBindVertexArray(0);
}
static void 
clear_abuffer(void)
{
    //TODO FIIIIX this is very slow, every frame 
    //it generates new textures, it should only be done on resizes
    if (1){
        glBindTexture(GL_TEXTURE_2D_ARRAY, abuf_tex_id);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, global_platform.window_width, global_platform.window_height, ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
        glBindImageTexture(0, abuf_tex_id, 0, TRUE, 0,  GL_READ_WRITE, GL_RGBA32F);
        glBindTexture(GL_TEXTURE_2D, abuf_counter_tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    }

    //these have to be loaded somewhere via glBindImageTexture?????
    setInt(&clear_abuffer_shader, "counter_img", 1);
    setInt(&clear_abuffer_shader, "abuf_img", 0);

    draw_quad(&clear_abuffer_shader);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

static void render_abuffer(Model *m)
{
    //setup render ing a buffer shader
    use_shader(&render_abuffer_shader);
    //mat4 model = mul_mat4(translate_mat4(m->position),scale_mat4(v3(10,10,10)));
    mat4 model = mul_mat4(translate_mat4(m->position),scale_mat4(v3(0.1,0.1,0.1)));
    mat4 view_IT = transpose_mat4(inv_mat4(view));
    setMat4fv(&render_abuffer_shader, "model", (GLfloat*)model.elements);
    setMat4fv(&render_abuffer_shader, "view", (GLfloat*)view.elements);
    setMat4fv(&render_abuffer_shader, "proj", (GLfloat*)proj.elements);
    setMat4fv(&render_abuffer_shader, "view_IT", (GLfloat*)view_IT.elements);
    setInt(&render_abuffer_shader, "counter_img", 1);
    setInt(&render_abuffer_shader, "abuf_img", 0);
    setInt(&render_abuffer_shader, "screen_width", global_platform.window_width);
    setInt(&render_abuffer_shader, "screen_height", global_platform.window_height);
    //setVec4(&render_abuffer_shader, "background_color", background_color);

    //render the model (here we draw all the models one by one, 
    //we just happen to have only one model rn)
    glBindVertexArray(m->vao);
    glDrawArrays(GL_TRIANGLES,0, m->mesh->vertices_count);
    glBindVertexArray(0);


    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

static void render_abuffer_quad(Quad *q)
{
    //setup render ing a buffer shader
    use_shader(&render_abuffer_quad_shader);
    //mat4 model = mul_mat4(translate_mat4(m->position),scale_mat4(v3(10,10,10)));
    mat4 model = mul_mat4(translate_mat4(v3(0,1,3)),scale_mat4(v3(1,1,1)));
    mat4 view_IT = transpose_mat4(inv_mat4(view));
    setMat4fv(&render_abuffer_quad_shader, "model", (GLfloat*)model.elements);
    setMat4fv(&render_abuffer_quad_shader, "view", (GLfloat*)view.elements);
    setMat4fv(&render_abuffer_quad_shader, "proj", (GLfloat*)proj.elements);
    setMat4fv(&render_abuffer_quad_shader, "view_IT", (GLfloat*)view_IT.elements);
    setInt(&render_abuffer_quad_shader, "counter_img", 1);
    setInt(&render_abuffer_quad_shader, "abuf_img", 0);
    setInt(&render_abuffer_quad_shader, "screen_width", global_platform.window_width);
    setInt(&render_abuffer_quad_shader, "screen_height", global_platform.window_height);

    //render the quad (here we draw all the quad one by one, 
    //we just happen to have only one model rn)

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, q->texture.id);
    vec3 colors[4] = {v3(1,0,0), v3(0,1,0), v3(0,0,1), v3(1,1,1)};
    for (int i = 0; i < 4; ++i)
    {
        model = translate_mat4(v3(0,1,i));
        setMat4fv(&render_abuffer_quad_shader, "model", (GLfloat*)model.elements);
        setVec3(&render_abuffer_quad_shader, "color", colors[i]);
        glBindVertexArray(q->VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }


    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}

static void display_abuffer(void)
{
    //Ensure that all global memory write from render_abuffer() are done before resolving
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    //these are not really needed, draw_quad sets these alright
    setInt(&render_abuffer_shader, "counter_img", 1);
    setInt(&render_abuffer_shader, "abuf_img", 0);
    setInt(&render_abuffer_shader, "screen_width", global_platform.window_width);
    setInt(&render_abuffer_shader, "screen_height", global_platform.window_height);

    draw_quad(&display_abuffer_shader); 

#if 1
    //takes a screenshot of the first layer of the 3d texture
    if (global_platform.key_pressed[KEY_K])
    {
        u32 id;
        glGenTextures(1, &id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, abuf_tex_id);
        glCopyImageSubData(abuf_tex_id, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, id, GL_TEXTURE_2D,0, 0, 0, 0, global_platform.window_width, global_platform.window_height, 0);
        Texture tex;
        tex.id = id;
        tex.width = global_platform.window_width;
        tex.height = global_platform.window_height;

        write_texture2D_to_disk(&tex);
    }
#endif
}
#endif
