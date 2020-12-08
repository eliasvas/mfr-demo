#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "tools.h"
#include "texture.h"
#include "shader.h"

#define SHADOW_WIDTH 1024 
#define SHADOW_HEIGHT 1024 


typedef struct ShadowMapFBO
{
    GLuint fbo;
    GLuint color_attachments[4];
    GLuint depth_attachment;
    u32 w,h;
    Shader s;
    mat4 lightSpaceMatrix;
}ShadowMapFBO;

extern mat4 view, proj;

static void
init_shadowmap_fbo(ShadowMapFBO *shadowmap)
{
    //we generate a new framebuffer and make a texture that will represent depth 
    glGenFramebuffers(1, &shadowmap->fbo);
    glGenTextures(1, &shadowmap->depth_attachment);
    glBindTexture(GL_TEXTURE_2D, shadowmap->depth_attachment);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);  

    
    //we attach the depth texture as a depth attachment for our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowmap->depth_attachment, 0);
    glDrawBuffer(GL_NONE);//we explicitly state that we are not going to read or draw, so we
    glReadBuffer(GL_NONE);//don't need to bind any color attachment to our FBO
    shader_load (&shadowmap->s, "../assets/shaders/shadowmap.vert","../assets/shaders/shadowmap.frag");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


static void
setup_shadowmap(ShadowMapFBO* shadowmap, mat4 view_matrix)
{
    //this is so that a new image is generated and shadows are resolution independant NOTE: ITS REALLY FUCKING SLOW
    //maybe do it only if resolution has changed
    glBindTexture(GL_TEXTURE_2D, shadowmap->depth_attachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, global_platform.window_width, global_platform.window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);  

    //render to depth map
    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap->fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    f32 near_plane = 0.1f;
    f32 far_plane = 100.f;
    mat4 light_projection = orthographic_proj(-50.f,50.f,-50.f,50.f, near_plane, far_plane); //we use orthographic projection because we do direction lights..

    //view_matrix = mul_mat4(translate_mat4({view_matrix.elements[3][0],view_matrix.elements[3][1], view_matrix.elements[3][2]}),rotate_mat4(90.f, v3(1.f,0.f,0.f)));
    view_matrix = look_at(v3(0,20,5), v3(0,0,-5), v3(0,1,0));

    mat4 lightSpaceMatrix = mul_mat4(light_projection,view_matrix); 

    shadowmap->lightSpaceMatrix = lightSpaceMatrix;
    //glBindFramebuffer(GL_FRAMEBUFFER,0);

    //render the scene as normal
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //ConfigureShaderAndMatrices
    use_shader(&shadowmap->s);
    setMat4fv(&shadowmap->s, "lightSpaceMatrix", (f32*)shadowmap->lightSpaceMatrix.elements);
    setInt(&shadowmap->s, "shadowmap_on", 1);
    setInt(&shadowmap->s, "shadowMap", 1);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, shadowmap->depth_attachment);
    //RenderScene()
}

typedef struct ShadowmapDebugQuad
{
	Shader shader; //wow
    GLuint vao;
    GLuint vbo;
}ShadowmapDebugQuad;


static void 
setup_debug_quad(ShadowmapDebugQuad* debug_quad, ShadowMapFBO* shadowmap)
{
	shader_load (&debug_quad->shader, "../assets/shaders/shadowmap_to_quad.vert","../assets/shaders/shadowmap_to_quad.frag");
	setFloat(&debug_quad->shader,"near_plane", 1.f);
    setFloat(&debug_quad->shader,"far_plane", 20.f);
    glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, shadowmap->depth_attachment);
}

static void
render_to_debug_quad(ShadowmapDebugQuad* debug_quad)
{
	
    use_shader(&debug_quad->shader);
    if (debug_quad->vao == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &debug_quad->vao);
        glGenBuffers(1, &debug_quad->vbo);
        glBindVertexArray(debug_quad->vao);
        glBindBuffer(GL_ARRAY_BUFFER, debug_quad->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(debug_quad->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
} 
#endif





















