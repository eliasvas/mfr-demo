#include "platform.h"
#include "tools.h"
#include "quad.h" 
#include "camera.h"
#include "objloader.h"
#include "model.h"
#include "entity.h"
#include "text.h"
#include "abuffer.h"
#include "skybox.h"
#include "depthpeel.h"
/*          TODO 
 *  -Model abstraction
 *  -Work on the NEW renderer!
 *  -Scene Graph
 *  -Make a depth-peeling demo?
 *  -----------------------------
 *  -Make good strings!!
 *  -IMGUI layer?
 *  -Framebuffers and stuff
 *  -3D animations (collada)
*/

static Quad q;
static Model m;
static MeshInfo mesh;
static Camera cam;
static BitmapFont bmf;
static Skybox skybox;

static mat4 view;
static mat4 proj;
static vec4 background_color;
//Here goes any startup code you have.
static void 
init(void)
{
    init_quad(&q, "../assets/dirt.png");
    init_camera(&cam);
    init_abuffer();
    {
        //mesh = load_obj("../assets/bunny/stanford_bunny.obj");
        mesh = load_obj("../assets/utah_teapot.obj");
        init_model_textured_basic(&m, &mesh);
        load_texture(&(m.diff),"../assets/bunny/stanford_bunny.jpg");
    }
    m.position = v3(0,0,-7);
    init_text(&bmf, "../assets/BMF.png");
    char *skybox_faces[6] = {"../assets/nebula/neb_rt.tga", "../assets/nebula/neb_lf.tga", "../assets/nebula/neb_up.tga",
        "../assets/nebula/neb_dn.tga", "../assets/nebula/neb_bk.tga", "../assets/nebula/neb_ft.tga" };
    //char *skybox_faces[6] = {"../assets/dirt.png", "../assets/dirt.png", "../assets/dirt.png", "../assets/dirt.png", "../assets/dirt.png", "../assets/dirt.png" };
    init_skybox(&skybox, skybox_faces);
    init_depth_peel();
}



static void 
update(void) {
    update_cam(&cam);
    view = get_view_mat(&cam);
    proj = perspective_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,100.f); 
    background_color = v4(0.4f ,0.3f + fabs(cos(global_platform.current_time)), 0.9f, 1.f); 
}


void
render_scene(void)
{
    render_quad_mvp(&q, mul_mat4(proj,view));
    render_model_textured_basic(&m,&proj, &view);
}

static void 
render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(background_color.x, background_color.y, background_color.z,background_color.w);
    clear_depth_peel_fbos();
    render_skybox(&skybox);

    //clear_abuffer();
    //render_abuffer_quad(&q);
    //render_abuffer(&m);
    //display_abuffer();

    //render_quad_mvp(&q, mul_mat4(proj,view));
    //render_model_textured_basic(&m,&proj, &view);
    render_depth_peel();



    if (global_platform.key_down[KEY_TAB])
        print_debug_info(&bmf);
    if (global_platform.key_pressed[KEY_P])
        write_texture2D_to_disk(&q.texture.id);
}

