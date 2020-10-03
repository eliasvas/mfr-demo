#ifndef TEXTURE_H
#define TEXTURE_H

#include "tools.h"
#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"



typedef struct Texture 
{
    GLuint id;
    u32 width;
    u32 height;
}Texture;

static b32 load_texture(Texture* tex,const char *filename)
{
    b32 result = 0;
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //TODO FIX
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //TODO FIX
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width = 0, height = 0, nrChannels;
    stbi_set_flip_vertically_on_load(TRUE);
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels,STBI_rgb_alpha);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        result = 1;
    }
    //stbi_image_free(data);
    tex->width = width;
    tex->height = height;
    return result;
}

typedef struct Image 
{
    unsigned char *data;
    i32 width;
    i32 height;
}Image;

static Image 
load_image_bytes(const char *filename)
{
    Image res;
    i32 width = 0, height = 0, nrChannels;
    stbi_set_flip_vertically_on_load(TRUE);
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels,STBI_rgb_alpha);
    res.data = data;
    res.width = width;
    res.height = height;
    return res;
}


#endif
