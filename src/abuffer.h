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
//static GLuint head_list, head_list;

GLuint head_list;
GLuint next_address;
GLuint global_node_buffer;
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
       sprintf(infoLog, "GL error: %i", err);
    }

}

extern mat4 view, proj;
extern vec4 background_color;
void 
init_abuffer_shaders(void)
{
    shader_load(&clear_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/clear_abuf.frag");
    shader_load(&render_abuffer_shader,"../assets/shaders/render_abuf.vert","../assets/shaders/render_abuf.frag");
    //shader_load(&render_abuffer_quad_shader,"../assets/shaders/render_abuf_quad.vert","../assets/shaders/render_abuf.frag");
    shader_load(&display_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/disp_abuf.frag");
}


void 
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

    //we initialize the atomic counter
    glGenBuffers(1, &next_address);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, next_address);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);




    if(!head_list)
        glGenTextures(1, &head_list);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, head_list);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Texture creation
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    //glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, global_platform.window_width, global_platform.window_height);
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??


}
void 
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
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, next_address);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, global_node_buffer);

	glDrawArrays(GL_TRIANGLES, 0, 24);
    glBindVertexArray(0);
}

#include "text.h"
extern BitmapFont bmf;

void 
clear_abuffer(void)
{

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
#if 1
    //clear Image 
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
    glBindTexture(GL_TEXTURE_2D, head_list);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
#endif
  
#if 1 
    //clear ssbo
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * global_platform.window_width * global_platform.window_height * 5, NULL, GL_STATIC_DRAW);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#endif



#if 0 


    GLuint zero = 0;
    //clear atomic counter NEVER BOUND TO AN INDEX
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    void *counter = glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_WRITE);
    memcpy(counter, &zero, sizeof(GLuint));
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    check_gl_errors();
    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);


#endif

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void 
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
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, next_address);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, global_node_buffer);
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

void display_abuffer(void)
{
    //Ensure that all global memory write from render_abuffer() are done before resolving
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

    //these are not really needed, draw_quad sets these alright
    setInt(&render_abuffer_shader, "screen_width", global_platform.window_width);
    setInt(&render_abuffer_shader, "screen_height", global_platform.window_height);


    draw_quad(&display_abuffer_shader); 
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
}
#endif
