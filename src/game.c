#include "platform.h"
#include "tools.h"
#include "quad.h" 
#include "camera.h"
#include "objloader.h"
#include "model.h"
#include "entity.h"
#include "text.h"
#include "abuffer.h"
/*          TODO 
 *  -Model abstraction
 *  -Work on the NEW renderer!
 *  -Look at A-Buffer stuff
 *  -Implement A-Buffer stuff (wow)
 *  -----------------------------
 *  -Make good strings!!
 *  -IMGUI layer?
 *  -Framebuffers and stuff
 *  -3D animations (collada)
*/

static Quad q;
static Model m;
static MeshInfo teapot_mesh;
static Camera cam;
static BitmapFont bmf;

static mat4 view;
static mat4 proj;
static Shader image_shader;
//Here goes any startup code you have.
static void 
init(void)
{
    init_quad(&q, "../assets/white.png");
    init_camera(&cam);
    init_abuffer();
    {
        teapot_mesh = load_obj("../assets/bunny/stanford_bunny.obj");
        init_model_textured_basic(&m, &teapot_mesh);
        load_texture(&(m.diff),"../assets/bunny/stanford_bunny.jpg");
    }
    m.position = v3(0,0,-7);
    init_text(&bmf, "../assets/BMF.png");

    shader_load(&image_shader,"../assets/shaders/image_demo.vert", "../assets/shaders/image_demo.frag");
    
}



static void 
update(void) {
    update_cam(&cam);
    view = get_view_mat(&cam);
    proj = perspective_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,100.f); 
#if 1
    {
        glBindImageTexture(3, q.texture.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
        use_shader(&image_shader);
        setFloat(&image_shader, "time", global_platform.current_time);
        setInt(&image_shader, "width", q.texture.width);
        setInt(&image_shader, "height", q.texture.height);
        glDrawArrays(GL_POINTS, 0, q.texture.width*q.texture.height);//we launch one GPU thread per pixel
        // make sure all computations are done, before we do the next pass, with a barrier.
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        //write_texture2D_to_disk(&q.texture.id);
    }
#endif

}

static void 
render(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.4f,0.5f,0.9f,1.f);

    render_model_textured_basic(&m,&proj, &view);
    render_quad_mvp(&q, mul_mat4(proj, view));

    if (global_platform.key_down[KEY_TAB])
        print_debug_info(&bmf);
    if (global_platform.key_pressed[KEY_P])
        write_texture2D_to_disk(&q.texture.id);
    

}
