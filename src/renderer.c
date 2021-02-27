#include "renderer.h"
#include "fbo.h"
#include "skybox.h"
#include "tools.h"
#include "openexr_write.h"

local_persist char point_attr[7][64] = {
        "point_lights[x].position",
        "point_lights[x].ambient",
        "point_lights[x].diffuse",
        "point_lights[x].specular",

        "point_lights[x].constant",
        "point_lights[x].linear",
        "point_lights[x].quadratic",
};
local_persist char big_point_attr[7][64] = {
        "point_lights[xx].position",
        "point_lights[xx].ambient",
        "point_lights[xx].diffuse",
        "point_lights[xx].specular",

        "point_lights[xx].constant",
        "point_lights[xx].linear",
        "point_lights[xx].quadratic",
};
local_persist f32 screen_verts[] = { 
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
};
local_persist GLfloat quad_verts[] = {
   -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, -1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f,    

   1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, 1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f  
};

local_persist f32 filled_quad_verts[] = { 0.f,0.f, 0.f,1.f, 1.f,0.f, 1.f,1.f };

internal void renderer_check_gl_errors()
{
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
       sprintf(error_log, "GL error: %i", err);
    }

}


void
renderer_init(Renderer *rend)
{
    rend->multisampling = FALSE;
    rend->multisamping_count = 4;
    rend->depthpeeling = FALSE;
    rend->depthpeel_count = 1;
    rend->renderer_settings.render_dim = (ivec2){global_platform.window_width, global_platform.window_height};
    rend->renderer_settings.lighting_disabled = FALSE;



    rend->main_fbo = fbo_init(rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, FBO_COLOR_0 | FBO_DEPTH);
    rend->postproc_fbo = fbo_init(rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, FBO_COLOR_0 | FBO_DEPTH);
    rend->ui_fbo = fbo_init(rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, FBO_COLOR_0);
    rend->shadowmap_fbo = fbo_init(2048, 2048, FBO_DEPTH);
    //rend->depthpeel_fbo = fbo_init(rend->renderer_settings.render_dim.x * 2, rend->renderer_settings.render_dim.y * 2, FBO_COLOR_0 | FBO_DEPTH);
    rend->current_fbo = &rend->main_fbo;
    

    rend->model_alloc_pos = 0;

    rend->default_material = material_default();

    rend->directional_light = (DirLight){v3(-0.2,-1,-0.3),v3(0.8,0.7,0.7),v3(0.8,0.7,0.7),v3(0.9f,0.9f,0.9f)};
    rend->point_light_count = 0;

    char **faces= cubemap_default();
    skybox_init(&rend->skybox, faces);
    camera_init(&rend->cam);
    //rend->ortho = orthographic_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,80.f); 

    //initialize postproc VAO
    {
        GLuint quad_vbo;
        glGenVertexArrays(1, &rend->postproc_vao);
        glGenBuffers(1, &quad_vbo);
        glBindVertexArray(rend->postproc_vao);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(screen_verts), &screen_verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(GLfloat)));
        glBindVertexArray(0); 
    }

    //initialize filled rect vao
    {
        GLuint vbo;
        glGenVertexArrays(1, &rend->filled_rect_vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &rend->filled_rect_instance_vbo);
        glBindVertexArray(rend->filled_rect_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(filled_quad_verts), &filled_quad_verts[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        //this buffer should be update with the contents of filled rect instance data before rendering
        glBindBuffer(GL_ARRAY_BUFFER, rend->filled_rect_instance_vbo);
        glVertexAttribPointer(1,3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(0 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(5 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(1,1);
        glVertexAttribDivisor(2,1);
        glVertexAttribDivisor(3,1);
    }
    //initialize line vao 
    {
        GLuint vbo; //this shouldnt be here????
        glGenVertexArrays(1, &rend->line_vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &rend->line_instance_vbo);
        glBindVertexArray(rend->line_vao);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, rend->line_instance_vbo);
        glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *)(0 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(0,1);
        glVertexAttribDivisor(1,1);
        glVertexAttribDivisor(2,1);
    }
    //initialize text vao
    {
        GLuint vbo;
        glGenVertexArrays(1, &rend->text_vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &rend->text_instance_vbo);
        glBindVertexArray(rend->text_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(filled_quad_verts), &filled_quad_verts[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, rend->text_instance_vbo);
        glVertexAttribPointer(1,3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(0 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(5 * sizeof(float)));
        glVertexAttribDivisor(1,1);
        glVertexAttribDivisor(2,1);
        glVertexAttribDivisor(3,1);
    }

    //initialize a-buffer stuff
    {
        //node ssbo
        glGenBuffers(1, &rend->node_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, rend->node_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * global_platform.window_width * global_platform.window_height * 3, NULL, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rend->node_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        GLuint zero = 0;
        //we initialize the atomic counter
        glGenBuffers(1, &rend->next_address);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rend->next_address);
        glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, rend->next_address);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);




        glGenTextures(1, &rend->head_list);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rend->head_list);

        //Set filters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //TODO FIX
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        //Texture creation
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, global_platform.window_width, global_platform.window_height, 0,  GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
        glBindImageTexture(0, rend->head_list, 0, GL_FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??        

        glGenVertexArrays(1, &rend->quad_vao);
        glGenBuffers(1, &rend->quad_vbo);
        glBindVertexArray(rend->quad_vao);
        glBindBuffer (GL_ARRAY_BUFFER, rend->quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer (0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, 0);
        glBindVertexArray(0);
        //renderer_check_gl_errors();
    }


    shader_load(&rend->shaders[0],"../assets/shaders/phong.vert","../assets/shaders/phong.frag");
    shader_load(&rend->shaders[1],"../assets/shaders/skybox_reflect.vert","../assets/shaders/skybox_reflect.frag");
    shader_load(&rend->shaders[2],"../assets/shaders/postproc.vert","../assets/shaders/postproc.frag");
    shader_load(&rend->shaders[3],"../assets/shaders/shadowmap.vert","../assets/shaders/shadowmap.frag");
    shader_load(&rend->shaders[4],"../assets/shaders/animated3d.vert","../assets/shaders/phong.frag");
    shader_load(&rend->shaders[5],"../assets/shaders/filled_rect.vert","../assets/shaders/filled_rect.frag");
    shader_load(&rend->shaders[6],"../assets/shaders/line.vert","../assets/shaders/line.frag");
    shader_load(&rend->shaders[7],"../assets/shaders/text.vert","../assets/shaders/text.frag");
    shader_load(&rend->shaders[8],"../assets/shaders/render_abuf.vert","../assets/shaders/render_abuf.frag");
    shader_load(&rend->shaders[9],"../assets/shaders/pass_through.vert","../assets/shaders/clear_abuf.frag");
    shader_load(&rend->shaders[10],"../assets/shaders/pass_through.vert","../assets/shaders/disp_abuf.frag");


    //misc
    texture_load(&rend->white_texture,"../assets/white.tga");
    texture_load(&rend->bmf,"../assets/bmf.tga");
}

void renderer_draw_fs_quad(Renderer *rend, Shader *shader)
{
   use_shader(shader);

   glBindVertexArray(rend->quad_vao);
   shader_set_int(shader, "screen_width", global_platform.window_width);
   shader_set_int(shader, "screen_height", global_platform.window_height);
   glBindImageTexture(0, rend->head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
   glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rend->next_address);
   glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, rend->next_address);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, rend->node_buffer);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rend->node_buffer);

   glDrawArrays(GL_TRIANGLES, 0, 24);
   glBindVertexArray(0); 
} 
void renderer_clear_abuffer(Renderer *rend)
{
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    renderer_draw_fs_quad(rend, &rend->shaders[9]); 
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}
void renderer_display_abuffer(Renderer *rend)
{
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    renderer_draw_fs_quad(rend, &rend->shaders[10]); 
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    if (rend->deep_write){ 
        //get image-head data 
        //NOTE: we get the image data from the framebuffer because glGetTexImage returns an all-black image
        GLint *image_head= (GLint*)ALLOC(sizeof(u32) *rend->renderer_settings.render_dim.x *rend->renderer_settings.render_dim.y* 4);   
        for (int i = 0; i < rend->renderer_settings.render_dim.x *rend->renderer_settings.render_dim.y; ++i)
            image_head[i] = 0;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rend->head_list);
        //we must convert to only red component later @TODO
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, image_head);
        glBindTexture(GL_TEXTURE_2D, 0);


        //get the atomic counter data (size of ssbo)
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rend->next_address);
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
        for (u32 i = 0; i <rend->renderer_settings.render_dim.x*rend->renderer_settings.render_dim.y* 4; i+=4)
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
        DeepPixel *pixels = malloc(sizeof(DeepPixel) * max_samples *rend->renderer_settings.render_dim.x* rend->renderer_settings.render_dim.y);
        i32 k = 0;
        //first flip so origin is at top left corner!!
        i32 new_i = 0;
        i32 *image_head_new = ALLOC(sizeof(image_head[0]) *rend->renderer_settings.render_dim.x* rend->renderer_settings.render_dim.y * 4);
        u32 *samples_per_pixel = ALLOC(sizeof(u32) * rend->renderer_settings.render_dim.x* rend->renderer_settings.render_dim.y );
        for(u32 i = 0; i <  rend->renderer_settings.render_dim.x* rend->renderer_settings.render_dim.y ; ++i)
        {
            samples_per_pixel[i] = 0;
        }
        for (i32 j = rend->renderer_settings.render_dim.y -1; j >=0; --j)
        {
            for (i32 i = 0; i < rend->renderer_settings.render_dim.x ; ++i)
            {
                i32 index = rend->renderer_settings.render_dim.x  * j * 4 +  4 * i;
                u8 cmp[3];
                image_head_new[new_i++]= (i32)(image_head[index]);
                image_head_new[new_i++]= (i32)(image_head[index+1]);
                image_head_new[new_i++]= (i32)(image_head[index+2]);
                image_head_new[new_i++]= (i32)(image_head[index+3]);
            }
        }
        u32 curr_pixel = 0;
        for (u32 i = 0; i <rend->renderer_settings.render_dim.x* rend->renderer_settings.render_dim.y * 4; i+=4)
        {
            u32 total_samples = 0; 
            if (image_head_new[i] == 0)
            {
               samples_per_pixel[curr_pixel++] = 0;
               continue; 
            }

            //write samples found
            NodeTypeLL *curr = &nodes[image_head_new[i]];
            while(TRUE) 
            {
                DeepPixel to_add;
                to_add.a = curr->alpha;
                to_add.b = curr->blue;
                to_add.g = curr->green;
                to_add.r = curr->red;
                to_add.z = curr->depth;
                pixels[k++] = to_add;
                total_samples++;
                //curr->next = 0;
                if (curr->next == NULL)break;
                curr = &nodes[curr->next];
            };
            
            samples_per_pixel[curr_pixel++] = total_samples;
        }
        if (rend->deep_settings & EXR_PAD)
            deepexr_write_padding(rend->renderer_settings.render_dim.x,rend->renderer_settings.render_dim.y,pixels, samples_per_pixel, max_samples);
        else
            deepexr_write(rend->renderer_settings.render_dim.x,rend->renderer_settings.render_dim.y,pixels,samples_per_pixel, deep_pixels_count,max_samples);
        //sprintf(&error_log, "Data Written to Disk");
    }
}


void
renderer_begin_frame(Renderer *rend)
{
  if (rend->renderer_settings.render_dim.x != global_platform.window_width)
        global_platform.window_resized = TRUE;
  rend->renderer_settings.render_dim = (ivec2){global_platform.window_width, global_platform.window_height};
  rend->prev_render_dim = rend->renderer_settings.render_dim;
  if (rend->deep_write)
  {
      rend->renderer_settings.render_dim = rend->deep_render_dim;
      global_platform.window_resized = TRUE;
  }
    if (global_platform.window_resized)
  {
      fbo_resize(&rend->postproc_fbo, rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, FBO_COLOR_0|FBO_DEPTH);
      fbo_resize(&rend->main_fbo, rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, FBO_COLOR_0|FBO_DEPTH);

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, rend->node_buffer);
      glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeTypeLL) * rend->renderer_settings.render_dim.y * rend->renderer_settings.render_dim.x * 3, NULL, GL_STATIC_DRAW);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rend->node_buffer);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, rend->head_list);

        //Set filters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //TODO FIX
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        //Texture creation
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, 0,  GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
        glBindImageTexture(0, rend->head_list, 0, GL_FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??        



  }
  fbo_bind(&rend->postproc_fbo);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0,0,0,0);

  fbo_bind(&rend->shadowmap_fbo);
  glClear(GL_DEPTH_BUFFER_BIT);
  //glClearColor(1,1,1,1);



  fbo_bind(&rend->main_fbo);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.7,0.8,1,1);

  //fbo_resize(&rend->depthpeel_fbo, rend->renderer_settings.render_dim.x*2, rend->renderer_settings.render_dim.y*2, FBO_COLOR_0|FBO_DEPTH);
  rend->current_fbo = &rend->main_fbo;
  rend->model_alloc_pos = 0;
  rend->animated_model_alloc_pos = 0;
  rend->filled_rect_alloc_pos = 0;
  rend->line_alloc_pos = 0;
  rend->point_light_count = 0;
  rend->text_alloc_pos = 0;

  camera_update(&rend->cam);
  rend->view = get_view_mat(&rend->cam);
  if (rend->deep_write) 
      rend->proj = orthographic_proj(-5, 5, -5, 5, 0.001f, 20.f);
  else
      rend->proj = perspective_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,20.f); 
}

internal void
renderer_set_light_uniforms(Renderer *rend, Shader *s)
{
    shader_set_int(s, "point_light_count", rend->point_light_count);
    for (i32 i = 0; i < rend->point_light_count;++i)
    {
      if (i < 10)
      {
        point_attr[0][13] = '0'+i;
        point_attr[1][13] = '0'+i;
        point_attr[2][13] = '0'+i;
        point_attr[3][13] = '0'+i;
        point_attr[4][13] = '0'+i;
        point_attr[5][13] = '0'+i;
        point_attr[6][13] = '0'+i;
        shader_set_vec3(s,point_attr[0], rend->point_lights[i].position);
        shader_set_vec3(s,point_attr[1], rend->point_lights[i].ambient);
        shader_set_vec3(s,point_attr[2], rend->point_lights[i].diffuse);
        shader_set_vec3(s,point_attr[3], rend->point_lights[i].specular);
        shader_set_float(s,point_attr[4], rend->point_lights[i].constant);
        shader_set_float(s,point_attr[5], rend->point_lights[i].linear);
        shader_set_float(s,point_attr[6], rend->point_lights[i].quadratic);
      }
      else
      {

        point_attr[0][13] = '0'+ (i / 10);
       point_attr[0][14] = '0'+ (i % 10);
        shader_set_vec3(s,big_point_attr[0], rend->point_lights[i].position);
        shader_set_vec3(s,big_point_attr[1], rend->point_lights[i].ambient);
        shader_set_vec3(s,big_point_attr[2], rend->point_lights[i].diffuse);
        shader_set_vec3(s,big_point_attr[3], rend->point_lights[i].specular);
        shader_set_float(s,big_point_attr[4], rend->point_lights[i].constant);
        shader_set_float(s,big_point_attr[5], rend->point_lights[i].linear);
        shader_set_float(s,big_point_attr[6], rend->point_lights[i].quadratic);
      }
    }
    //directional light properties
    shader_set_vec3(s, "dirlight.direction", rend->directional_light.direction);
    shader_set_vec3(s, "dirlight.ambient", rend->directional_light.ambient);
    shader_set_vec3(s, "dirlight.diffuse", rend->directional_light.diffuse);
    shader_set_vec3(s, "dirlight.specular", rend->directional_light.specular);



}

//here we put the draw calls for everything that needs shadowmapping!
internal void
renderer_render_scene3D(Renderer *rend,Shader *shader)
{
  mat4 inv_view = mat4_inv(rend->view);
  vec3 view_pos = v3(inv_view.elements[3][0],inv_view.elements[3][1],inv_view.elements[3][2]);
  for(i32 i = 0; i < rend->model_alloc_pos;++i)
  { 
    RendererModelData data = rend->model_instance_data[i];
    mat4 ortho_proj = orthographic_proj(-100.f, 100.f, -100.f, 100.f, 0.01f, 100.f);
    //mat4 light_space_matrix = mat4_mul(ortho_proj, mat4_mul(
     //     mat4_translate(v3(rend->view.elements[3][0],rend->view.elements[3][1] + 10.f,rend->view.elements[3][2])), mat4_rotate(-90.f, v3(1.f,0.f,0.f))));

    mat4 light_space_matrix = mat4_mul(ortho_proj,look_at(v3(0,100,0), v3(-10,0,0), v3(0,1,0)));



    use_shader(&shader[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data.diff->id);
    shader_set_int(&shader[0], "material.diffuse", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data.spec->id);
    shader_set_int(&shader[0], "material.specular", 1);

    //in case we need the skybox's texture for the rendering
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, rend->skybox.tex_id);
    shader_set_int(&rend->skybox.shader, "skybox", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, rend->shadowmap_fbo.depth_attachment);
    shader_set_int(&shader[0], "shadow_map", 4);


    shader_set_mat4fv(&shader[0], "model", (GLfloat*)data.model.elements);
    shader_set_mat4fv(&shader[0], "view", (GLfloat*)rend->view.elements);
    shader_set_mat4fv(&shader[0], "proj", (GLfloat*)rend->proj.elements);
    shader_set_mat4fv(&shader[0], "ortho", (GLfloat*)rend->ortho.elements);
    shader_set_mat4fv(&shader[0], "light_space_matrix", (GLfloat*)light_space_matrix.elements);
    shader_set_vec3(&shader[0], "view_pos", view_pos);
    shader_set_vec3(&shader[0], "view_front", rend->cam.front);
    //set material properties
    shader_set_float(&shader[0], "material.shininess", data.material.shininess);
    shader_set_int(&shader[0], "deep_render", rend->deep_write);
    //light properties
    renderer_set_light_uniforms(rend, shader);
    //abuffer stuff
   glBindImageTexture(0, rend->head_list, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI); //maybe its GL_R32F??
   glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, rend->next_address);
   glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, rend->next_address);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, rend->node_buffer);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rend->node_buffer);



    glBindVertexArray(data.model_vao);
    glDrawArrays(GL_TRIANGLES,0, data.model_vertex_count);
    //glDrawArrays(GL_TRIANGLES,0, 3000*fabs(sin(global_platform.current_time)));
    glBindVertexArray(0);
  }

}
void
renderer_end_frame(Renderer *rend)
{
  mat4 inv_view = mat4_inv(rend->view);
  vec3 view_pos = v3(inv_view.elements[3][0],inv_view.elements[3][1],inv_view.elements[3][2]);
  //renderer_check_gl_errors();

  //update instance data for filled rect
  glBindBuffer(GL_ARRAY_BUFFER, rend->filled_rect_instance_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(RendererFilledRect) * rend->filled_rect_alloc_pos, &rend->filled_rect_instance_data[0], GL_DYNAMIC_DRAW);
  //update instance data for line 
  glBindBuffer(GL_ARRAY_BUFFER, rend->line_instance_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(RendererLine) * rend->line_alloc_pos, &rend->line_instance_data[0], GL_DYNAMIC_DRAW);
  //update instance data for text 
  glBindBuffer(GL_ARRAY_BUFFER, rend->text_instance_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(RendererChar) * rend->text_alloc_pos, &rend->text_instance_data[0], GL_DYNAMIC_DRAW);




  //first we render the scene to the depth map
  fbo_bind(&rend->shadowmap_fbo);
  renderer_render_scene3D(rend,&rend->shaders[3]);
  //then we render to the main fbo
  fbo_bind(&rend->main_fbo);
  for (u32 i = 0; i < rend->animated_model_alloc_pos; ++i)
  {
      use_shader(&rend->shaders[4]);
    
    shader_set_mat4fv(&rend->shaders[4], "proj", (GLfloat*)rend->proj.elements);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rend->animated_model_instance_data[i].diff.id);
    shader_set_int(&rend->shaders[4], "material.diffuse", 0); //we should really make the texture manager global or something(per Scene?)... sigh
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rend->animated_model_instance_data[i].spec.id);
    shader_set_int(&rend->shaders[4], "material.specular", 1);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, rend->shadowmap_fbo.depth_attachment);
    shader_set_int(&rend->shaders[4], "shadow_map", 4);

    if (0){
        mat4 identity = m4d(1.f);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[0]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[1]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[2]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[3]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[4]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[5]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[6]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[7]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[8]", (GLfloat*)identity.elements);
        shader_set_mat4fv(&rend->shaders[4], "joint_transforms[9]", (GLfloat*)identity.elements);
        //@memleak
        char str[32] = "joint_transforms[xx]";

        for (i32 i = 10; i < rend->animated_model_instance_data[i].joint_count; ++i)
        {
            str[17] = '0' + (i/10);
            str[18] = '0' + (i -(((int)(i/10)) * 10));
            shader_set_mat4fv(&rend->shaders[4], str, (GLfloat*)identity.elements);
        }
    }
    for (u32 j = 0; j < rend->animated_model_instance_data[i].joint_count; ++j)
        set_joint_transform_uniforms(&rend->shaders[4], &rend->animated_model_instance_data[i].joints[j]);
    shader_set_mat4fv(&rend->shaders[4], "view", (GLfloat*)rend->view.elements);
    shader_set_mat4fv(&rend->shaders[4], "model", (GLfloat*)rend->animated_model_instance_data[i].model.elements);
    shader_set_vec3(&rend->shaders[4], "lightdir", rend->directional_light.direction); 
    renderer_set_light_uniforms(rend, &rend->shaders[4]);
    mat4 inv_view = mat4_inv(rend->view);
    vec3 view_pos = v3(inv_view.elements[3][0],inv_view.elements[3][1],inv_view.elements[3][2]);
    shader_set_vec3(&rend->shaders[4], "view_pos", view_pos); 
    mat4 ortho_proj = orthographic_proj(-200.f, 200.f, -200.f, 200.f, 0.01f, 200.f);
    mat4 light_space_matrix = mat4_mul(ortho_proj,look_at(v3(0,100,0), v3(10,0,0), v3(0,1,0)));
    shader_set_mat4fv(&rend->shaders[4], "light_space_matrix", (GLfloat*)light_space_matrix.elements);




    glBindVertexArray(rend->animated_model_instance_data[i].vao);
    glDrawArrays(GL_TRIANGLES,0, rend->animated_model_instance_data[i].vertices_count);
    //glDrawArrays(GL_LINES,0, 40000);
    glBindVertexArray(0);
  }
  renderer_render_scene3D(rend,&rend->shaders[0]);
  skybox_render(&rend->skybox, rend->proj, rend->view);

  glBindFramebuffer(GL_FRAMEBUFFER, rend->postproc_fbo.fbo);
  glBindTexture(GL_TEXTURE_2D, rend->main_fbo.color_attachments[0]);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, rend->renderer_settings.render_dim.x, rend->renderer_settings.render_dim.y);

    glEnable(GL_DEPTH_TEST);
    use_shader(&rend->shaders[2]);
    glBindVertexArray(rend->postproc_vao);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rend->main_fbo.color_attachments[0]);
    shader_set_int(&rend->shaders[2],"screenTexture",1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rend->main_fbo.depth_attachment);
    shader_set_int(&rend->shaders[2],"depthTexture",0);
    shader_set_mat4fv(&rend->shaders[2], "proj", (GLfloat*)rend->proj.elements);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);


  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0,0,rend->renderer_settings.render_dim.x,rend->renderer_settings.render_dim.y);

  //@TODO ENABLE THISSSSSSSSSSSSSS
  fbo_copy_contents(rend->postproc_fbo.fbo,0);
  
  glDisable(GL_DEPTH_TEST);
  renderer_display_abuffer(rend);
  glEnable(GL_DEPTH_TEST);

    //render filled rects
    glDisable(GL_DEPTH_TEST);
    use_shader(&rend->shaders[5]);
    shader_set_mat4fv(&rend->shaders[5], "view", (GLfloat*)rend->view.elements);
    glBindVertexArray(rend->filled_rect_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rend->filled_rect_alloc_pos);
    glBindVertexArray(0);



  //at the end we render the skybox
    //render lines
    glLineWidth(3);
    use_shader(&rend->shaders[6]);
    shader_set_mat4fv(&rend->shaders[6], "view", (GLfloat*)rend->view.elements);
    glBindVertexArray(rend->line_vao);
    glDrawArraysInstanced(GL_LINES, 0, 2, rend->line_alloc_pos);
    glBindVertexArray(0);

    use_shader(&rend->shaders[7]);
    shader_set_mat4fv(&rend->shaders[7], "view", (GLfloat*)rend->view.elements);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rend->bmf.id);
    shader_set_int(&rend->shaders[7], "bmf_sampler",0);
    glBindVertexArray(rend->text_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4,rend->text_alloc_pos);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);




  renderer_clear_abuffer(rend, &rend->shaders[9]);
}

void renderer_push_model(Renderer *rend, Model *m)
{
  RendererModelData data = (RendererModelData){0};
  for (u32 i = 0; i < m->mesh_count; ++i)
  {
    data.model = m->model;
    data.model_vao = m->meshes[i].vao;
    data.model_vertex_count = m->meshes[i].vertices_count;
    data.diff = &m->meshes[i].material.diff;
    data.spec = &m->meshes[i].material.spec;
    data.material = material_default();
    rend->model_instance_data[rend->model_alloc_pos++] = data;
  }

}
/*
typedef struct RendererAnimatedModelData
{
    GLuint vao;
    Texture diff_tex;
    Texture spec_tex;
    
    u32 joint_count;
    Joint *joints;
    u32 vertices_count;
}RendererAnimatedModelData;
*/

void renderer_push_animated_model(Renderer *rend, AnimatedModel *m)
{
  RendererAnimatedModelData data = {0};
  data.vao = m->vao;
  data.diff = *m->diff_tex;
  data.spec = rend->white_texture; //declare a white texture for such cases (in renderer)
  data.joint_count = m->joint_count;
  data.joints = m->joints;
  data.vertices_count = m->vertices_count; 
  //data.model = mat4_translate(v3(0,2,0));
  data.model = mat4_translate(v3(0,0.5f,0));
  rend->animated_model_instance_data[rend->animated_model_alloc_pos++] = data;
}

void renderer_push_filled_rect(Renderer *rend, vec3 pos, vec2 dim, vec4 color)
{
    RendererFilledRect rect = (RendererFilledRect){pos, dim, color};
    rend->filled_rect_instance_data[rend->filled_rect_alloc_pos++] = rect;
}

void renderer_push_text(Renderer *rend, vec3 pos, vec2 dim, char *str)
{
    u32 str_len = str_size(str);
    for (u32 ch = 0; ch < str_len;++ch)
    {
        char letter = str[ch];
        f32 uv_x = (letter % 16) / 16.f;
        f32 uv_y = 1 - (letter / 16) /16.f - 1.f/16.f;
        vec2 uv_down_left = v2(uv_x, uv_y);
        //for each char in string .. put 'em in the instance data
        pos.x += dim.x;
        RendererChar c = (RendererChar){pos, dim, uv_down_left}; //@Fix
        rend->text_instance_data[rend->text_alloc_pos++] = c;
    }
}

void renderer_push_char(Renderer *rend, vec3 pos, vec2 dim, char ch)
{
    //for each char in string .. put 'em in the instance data
    RendererChar c = (RendererChar){pos, dim, v2(0,0)}; //@Fix
    rend->text_instance_data[rend->text_alloc_pos++] = c;
}


void renderer_push_line(Renderer *rend, vec3 start, vec3 end, vec4 color)
{
    RendererLine line = (RendererLine){start,end,color};
    rend->line_instance_data[rend->line_alloc_pos++] = line;
}


void renderer_push_point_light(Renderer *rend, PointLight l)
{
  rend->point_lights[rend->point_light_count++] = l;
}

void renderer_set_deep_write(Renderer *rend, b32 dw, u32 settings)
{
    rend->deep_write = dw;
    rend->deep_settings = settings;
    rend->deep_render_dim = (ivec2){global_platform.window_width, global_platform.window_height};
}

