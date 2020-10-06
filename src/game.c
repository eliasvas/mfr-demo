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
 *  -Implement A-Buffer stuff (wow)
 *  -Scene Graph
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
//Here goes any startup code you have.
static void 
init(void)
{
    init_quad(&q, "../assets/dirt.png");
    init_camera(&cam);
    init_abuffer();
    {
        teapot_mesh = load_obj("../assets/bunny/stanford_bunny.obj");
        init_model_textured_basic(&m, &teapot_mesh);
        load_texture(&(m.diff),"../assets/bunny/stanford_bunny.jpg");
    }
    m.position = v3(0,0,-7);
    init_text(&bmf, "../assets/BMF.png");

}



static void 
update(void) {
    update_cam(&cam);
    view = get_view_mat(&cam);
    proj = perspective_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,100.f); 
}

static void 
render_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.4f,0.5f,0.9f,1.f);

    clear_abuffer();
    render_abuffer(&m);
    //display_abuffer();

    render_model_textured_basic(&m,&proj, &view);
    render_quad_mvp(&q, mul_mat4(proj,view));

    if (global_platform.key_down[KEY_TAB])
        print_debug_info(&bmf);
    if (global_platform.key_pressed[KEY_P])
        write_texture2D_to_disk(&q.texture.id);
}


static void 
render(void)
{
    render_scene();
}

