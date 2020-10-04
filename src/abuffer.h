#ifndef ABUFFER_H
#define ABUFFER_H

#include "tools.h"
#include "shader.h"
#include "texture.h"
#include "platform.h"
//why 16 and not 4 ? maybe 16 is 4 X components(4)????
#define ABUFFER_SIZE 16
static Shader clear_abuffer_shader;
static Shader render_abuffer_shader;
//the quad way
static Shader display_abuffer_shader;

static void 
init_abuffer_shaders(void)
{
    //shader_load(&clear_abuffer_shader,"../assets/shaders/pass_through.vert","../assets/shaders/clear_abuf.frag");
    shader_load(&render_abuffer_shader,"../assets/shaders/mesh.vert","../assets/shaders/mesh.frag");
    shader_load(&display_abuffer_shader,"../assets/shaders/mesh.vert","../assets/shaders/mesh.frag");
}

static GLuint abufferTexID, abufferCounterTexID;

static void 
init_abuffer(void)
{
    init_abuffer_shaders();
		
    ///ABuffer storage///
    if(!abufferTexID)
        glGenTextures(1, &abufferTexID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, abufferTexID);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Texture creation
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, global_platform.window_width, global_platform.window_height, ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
    glBindImageTexture(0, abufferTexID, 0, TRUE, 0,  GL_READ_WRITE, GL_RGBA32F);

    ///ABuffer per-pixel counter///
    if(!abufferCounterTexID)
        glGenTextures(1, &abufferCounterTexID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, abufferCounterTexID);

    //Set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Texture creation
    //Uses GL_R32F instead of GL_R32I that is not working in R257.15
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, global_platform.window_width, global_platform.window_height, 0,  GL_RED, GL_FLOAT, 0);
    glBindImageTexture(1, abufferCounterTexID, 0, FALSE, 0,  GL_READ_WRITE, GL_R32UI);

}
/* example of changing some texture with the use of OpenGL 4.2 Images

    shader_load(&image_shader,"../assets/shaders/image_demo.vert", "../assets/shaders/image_demo.frag");
    {
        glBindImageTexture(3, q.texture.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
        use_shader(&image_shader);
        setFloat(&image_shader, "time", global_platform.current_time);
        setInt(&image_shader, "width", q.texture.width);
        setInt(&image_shader, "height", q.texture.height);
        glDrawArrays(GL_POINTS, 0, q.texture.width*q.texture.height);//we launch one GPU thread per pixel
        // make sure all computations are done, before we do the next pass, with a barrier.
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        write_texture2D_to_disk(&q.texture.id);
    }

*/

#endif
