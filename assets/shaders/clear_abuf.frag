#version 400

#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 512

coherent uniform layout(size1x32) uimage2D counter_img;
coherent uniform layout(size4x32) image2DArray abuf_img;

void main(void)
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

    if (coords.x >= 0 && coords.y >=0 && coords.x<SCREEN_HEIGHT && coords.y<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT)
    {
        //set counter to 0
        imageStore(counter_img, coords, ivec4(0));
        
        //put black in first layer
        imageStore(abuf_img, ivec3(coords, 0), vec4(0.0f));
    }

    //discard fragment so that it isn't written in framebuffer
    discard;
}
