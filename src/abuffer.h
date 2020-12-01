#ifndef ABUFFER_H
#define ABUFFER_H

#include "tools.h"
#include "shader.h"
#include "texture.h"
#include "platform.h"
#include "openexr_write.h"

//why 16 and not 4 ? maybe 16 is 4 X components(4)????
#define ABUFFER_SIZE 16
//these have to go and be replaced by a nice little struct!
static Shader clear_abuffer_shader;
static GLuint quad_vao, quad_vbo;

static Shader render_abuffer_shader;
static Texture texture;
//static Shader render_abuffer_quad_shader;
//the quad way
static Shader display_abuffer_shader;
//static GLuint head_list, head_list;

static GLuint head_list;
static GLuint next_address;
static GLuint global_node_buffer;
static GLuint zero = 0;

#if !defined(ABUF_SOFTWARE_CLEAR)
#define ABUF_SOFTWARE_CLEAR 0
#endif

//Full screen quad vertices definition
static GLfloat quad_verts[] = {
   -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, -1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f,    

   1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, 1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f  
};
static void check_gl_errors()
{
    return;
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
       sprintf(infoLog, "GL error: %i", err);
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


GLuint *source_data;
static void 
init_abuffer(void)
{
    init_abuffer_shaders();
    source_data = arena_alloc(&global_platform.permanent_storage,global_platform.window_width * global_platform.window_height * sizeof(GLuint)); 
    load_texture(&(texture),"../assets/bunny/stanford_bunny.jpg");
		
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
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, next_address);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);




    if(!head_list)
        glGenTextures(1, &head_list);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, head_list);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //TODO FIX
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glGenerateMipmap(GL_TEXTURE_2D);

    //Texture creation
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, global_platform.window_width, global_platform.window_height, 0,  GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_UNSIGNED_INT, 0); //THISISCORRECT
    //vec4 data = v4(0.0,0.0,0.0,0.0);
    //glClearTexImage(GL_TEXTURE_2D, 0, GL_R32F, GL_FLOAT, &data);
    //glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, global_platform.window_width, global_platform.window_height);
    glBindImageTexture(0, head_list, 0, GL_FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??


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
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, next_address);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, global_node_buffer);

	glDrawArrays(GL_TRIANGLES, 0, 24);
    glBindVertexArray(0);


}
#include "stdio.h"
static void 
clear_abuffer(void)
{
 
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    if (global_platform.key_pressed[KEY_M])
    {
        void *data = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        FILE *file = fopen("data.txt", "w");
        fwrite(&data, sizeof(f32), 100,file);
        fclose(file);
        sprintf(&infoLog, "Data Written to Disk");
    }

    //resize image
    /*
    if (global_platform.window_resized){
        glBindTexture(GL_TEXTURE_2D, head_list);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * global_platform.window_width * global_platform.window_height *4 , NULL, GL_STATIC_DRAW);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

    }
    */

#if ABUF_SOFTWARE_CLEAR 
#if 1 
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0 , sizeof(GLuint), &zero);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
#endif

#if 0
    //clear Image 
    glBindImageTexture(0, head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
    glBindTexture(GL_TEXTURE_2D, head_list);

    //GLuint *pixels = arena_alloc(&global_platform.frame_storage, sizeof(GLuint)*global_platform.window_width * global_platform.window_height);
    //glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_INT, pixels);
    glClearTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, source_data);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
#endif
  
#if 1
    //clear ssbo
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, global_node_buffer);
    void * data = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * global_platform.window_width * global_platform.window_height *4 , NULL, GL_STATIC_DRAW);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
#endif
#endif

   //this clearing is done via a shader..
   draw_quad(&clear_abuffer_shader); 
   glMemoryBarrier(GL_ALL_BARRIER_BITS);

}
static void 
render_abuffer(Model *m)
{

    use_shader(&render_abuffer_shader);
    mat4 model = mul_mat4(translate_mat4(m->position),scale_mat4(v3(10,10,10)));
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
    setInt(&m->s, "duiffuse-map", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    //setVec4(&render_abuffer_shader, "background_color", background_color);

    //render the model (here we draw all the models one by one, 
    //we just happen to have only one model rn)
    glBindVertexArray(m->vao);
    glDrawArrays(GL_TRIANGLES,0, m->mesh->vertices_count);
    glBindVertexArray(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    check_gl_errors();

}
static void 
render_abuffer_shad(Model *m, Shader *s)
{

    mat4 model = mul_mat4(translate_mat4(m->position),scale_mat4(v3(10,10,10)));
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
    setInt(&m->s, "duiffuse-map", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    //setVec4(&render_abuffer_shader, "background_color", background_color);

    //render the model (here we draw all the models one by one, 
    //we just happen to have only one model rn)
    glBindVertexArray(m->vao);
    glDrawArrays(GL_TRIANGLES,0, m->mesh->vertices_count);
    glBindVertexArray(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    check_gl_errors();

}


static void display_abuffer(void)
{

    //Ensure that all global memory write from render_abuffer() are done before resolving
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    //these are not really needed, draw_quad sets these alright
    setInt(&render_abuffer_shader, "screen_width", global_platform.window_width);
    setInt(&render_abuffer_shader, "screen_height", global_platform.window_height);



    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    draw_quad(&display_abuffer_shader); 
    //glFinish();
    if (global_platform.key_pressed[KEY_Q]){ 
        //get image-head data 
        //NOTE: we get the image data from the framebuffer because glGetTexImage returns an all-black image
        GLint *image_head= (GLint*)ALLOC(sizeof(u32) *global_platform.window_width * global_platform.window_height * 4);   
        for (int i = 0; i < global_platform.window_width * global_platform.window_height; ++i)
            image_head[i] = 0;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head_list);
        //we must convert to only red component later @TODO
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, image_head);
        glBindTexture(GL_TEXTURE_2D, 0);


        //get the atomic counter data (size of ssbo)
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, next_address);
        int counter_val = 0; 
        GLuint *counter_data = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY); 
        memcpy(&counter_val, counter_data, sizeof(int));
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);


        //get the ssbo (NodeTypeLL data)
        NodeTypeLL *nodes = (NodeTypeLL*)ALLOC(sizeof(NodeTypeLL) * counter_val); 
        void *node_data= glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        memcpy(nodes, node_data, sizeof(NodeTypeLL) * counter_val);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        //deepexr_write(image_head,v2(global_platform.window_width, global_platform.window_height),(void *)nodes,counter_val,0);
        //write a sample openexr image
        //populat deep pixels array
        u32 deep_pixels_count = 0;
        u32 max_samples = 0;
        u32 max_samples_i = 0;
        //count max samples per-pixel
        for (u32 i = 0; i < global_platform.window_width * global_platform.window_height * 4; i+=4)
        {
            u32 local_max_samples = 0;
            if (image_head[i] == 0)continue;
            NodeTypeLL *curr = &nodes[image_head[i]];
            ++local_max_samples;
            ++deep_pixels_count;
            while (curr->next != 0)
            {
                ++local_max_samples;
                ++deep_pixels_count;
                curr = &nodes[curr->next];
            }
            if (local_max_samples > max_samples){
                max_samples = local_max_samples;
                max_samples_i = i / 4;
            }
        }
        DeepPixel *pixels = malloc(sizeof(DeepPixel) * max_samples * global_platform.window_width * global_platform.window_height);
        i32 k = 0;
#if 1
        for (u32 i = 0; i < global_platform.window_width * global_platform.window_height * 4; i+=4)
        {
            i32 remaining_samples = max_samples; 
            //if no samples found, write {0}s
            if (image_head[i] == 0)
            {
                for (u32 j = 0; j < max_samples;++j)
                    pixels[k++] = (DeepPixel){0};
               continue; 
            }

            //write samples found
            NodeTypeLL *curr = &nodes[image_head[i]];
            do
            {
                u32 color = curr->color;
                DeepPixel to_add;
                to_add.a = (f32)(color & 0xFF) /255.f;
                to_add.b = (f32)((color >> 8) & 0xFF) / 255.f;
                to_add.g = (f32)((color >> 16) & 0xFF)/255.f;
                to_add.r = (f32)((color >> 24) & 0xFF)/255.f;
                pixels[k++] = to_add;
                remaining_samples--;
                //curr->next = 0;
                curr = &nodes[curr->next];
            }while (curr->next != 0);

            //write the remaining samples
            for (u32 j = 0; j < remaining_samples;++j)
                    pixels[k++] = (DeepPixel){0};

        }
#endif
        deepexr_write(global_platform.window_width, global_platform.window_height,pixels, deep_pixels_count,max_samples);
        openexr_screenshot();
        sprintf(&infoLog, "Data Written to Disk");
    }

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}


#endif
