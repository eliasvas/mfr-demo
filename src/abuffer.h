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
//static Shader render_abuffer_quad_shader;
//the quad way
static Shader display_abuffer_shader;
static GLuint head_list, head_list;

static GLuint head_list;
static GLuint next_address;
static GLuint global_node_buffer;
//Full screen quad vertices definition
static GLfloat quad_verts[] = {
   -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, -1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f,    

   1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, 1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f  
};
typedef struct NodeTypeLL
{
	f32 depth;
	u32 color;
	u32 next;
}NodeTypeLL;

static void check_gl_errors()
{
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
       sprintf(infoLog, "%i", err);
    }

}

extern mat4 view, proj;
extern vec4 background_color;
static void 
init_abuffer_shaders(void)
{
    shader_load(&clear_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/clear_abuf.frag");
    shader_load(&render_abuffer_shader,"../assets/shaders/render_abuf.vert","../assets/shaders/render_abuf.frag");
    //shader_load(&render_abuffer_quad_shader,"../assets/shaders/render_abuf_quad.vert","../assets/shaders/render_abuf.frag");
    shader_load(&display_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/disp_abuf.frag");
}


static void 
init_abuffer(void)
{
    init_abuffer_shaders();
		
    //we make the ssbo which by the end of the
    //drawcall will hold all fragments
    glGenBuffers(1, &global_node_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * global_platform.window_width * global_platform.window_height * 3, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, global_node_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &next_address);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, next_address);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);




    if(!head_list)
        glGenTextures(1, &head_list);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, head_list);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Texture creation
    //Uses GL_R32F instead of GL_R32I that is not working in R257.15
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??


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
    //glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??


	glDrawArrays(GL_TRIANGLES, 0, 24);
    glBindVertexArray(0);
}

#include "text.h"
extern BitmapFont bmf;

static void 
clear_abuffer(void)
{
     //clear the data on the global buffer
    //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * global_platform.window_width * global_platform.window_height * 3, NULL, GL_DYNAMIC_DRAW);

#if 0
    //set the counter to 0
    GLuint *counter_val;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    counter_val =(GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), 
            GL_MAP_READ_BIT | GL_MAP_WRITE_BIT); 
    //glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
    check_gl_errors();
    #if DEBUGINFO | 1
        char string[11];
        sprintf(string, "%i", counter_val[0]);
        print_text(&bmf,string, 0,250, 20);
    #endif
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
#endif

    glBindTexture(GL_TEXTURE_2D, head_list);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);

    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

static void 
render_abuffer(Model *m)
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
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
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

static void display_abuffer(void)
{
    //Ensure that all global memory write from render_abuffer() are done before resolving
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    //these are not really needed, draw_quad sets these alright
    setInt(&render_abuffer_shader, "screen_width", global_platform.window_width);
    setInt(&render_abuffer_shader, "screen_height", global_platform.window_height);


    draw_quad(&display_abuffer_shader); 
}
#endif
