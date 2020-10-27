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
 *  -Work on the NEW renderer!
 *  -Scene Graph
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
    init_fullscreen_quad(&screen_quad, "../assets/red.png");
    init_camera(&cam);
    init_abuffer();
    {
        mesh = load_obj("../assets/bunny/stanford_bunny.obj");
        //mesh = load_obj("../assets/utah_teapot.obj");
        init_model_textured_basic(&m, &mesh);
        load_texture(&(m.diff),"../assets/red.png");
    }
    m.position = v3(0,0,-2);
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
    //background_color = v4(0.4f ,0.3f + fabs(cos(global_platform.current_time)), 0.9f, 1.f); 
    background_color = v4(0.2,0.2,0.2,1.f);
}


void
render_scene(void)
{
    render_quad_mvp_dp(&q, mul_mat4(proj,view));
    render_quad_mvp_dp(&q, mul_mat4(mul_mat4(proj,view), translate_mat4(v3(0,0,-2))));
    //render_model_textured_basic(&m,&proj, &view);
}

static void 
render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(background_color.x, background_color.y, background_color.z,background_color.w);
    render_skybox(&skybox);

    //NOTE: a-buffer rendering
#if 1
    clear_abuffer();
    m.position = v3(0,0,-5);
    render_abuffer(&m);
    m.position = v3(0,0,-2);
    render_abuffer(&m);
    m.position = v3(0,0,-11);
    render_abuffer(&m);
    m.position = v3(0,0,-8);
    render_abuffer(&m);
    display_abuffer();
#endif


    //NOTE: normal rendering
    //render_quad_mvp(&q, mul_mat4(proj,view));
    //render_model_textured_basic(&m,&proj, &view);


    //NOTE: depth-peeling rendering
    //clear_depth_peel_fbos();
    //render_depth_peel();



    if (global_platform.key_down[KEY_TAB])
        print_debug_info(&bmf);
    if (global_platform.key_pressed[KEY_P])
        write_texture2D_to_disk(&q.texture.id);
}

