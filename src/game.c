#include "platform.h"
#include "tools.h"
#include "quad.h" 
#include "camera.h"
#include "objloader.h"
#include "model.h"
#include "entity.h"
#include "text.h"
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

//Here goes any startup code you have.
static void 
init(void)
{
    init_quad(&q, "../assets/dirt.png");
    init_camera(&cam);
    {
        teapot_mesh = load_obj("../assets/bunny/stanford_bunny.obj");
        init_model_textured_basic(&m, &teapot_mesh);
        load_texture(&(m.diff),"../assets/bunny/stanford_bunny.jpg");
    }
    m.position = v3(0,0,-7);
    init_text(&bmf, "../assets/BMF512.png");
}



//This gets called every frame and is meant to update your data.
static void 
update(void) {
    update_cam(&cam);
    view = get_view_mat(&cam);
    proj = perspective_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,100.f); 
}

//This gets called evert frame and is ment to render your data.
static void 
render(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.4f,0.5f,0.9f,1.f);

    render_model_textured_basic(&m,mul_mat4(proj, view));
    render_quad_mvp(&q, mul_mat4(proj, view));

#if 1 
    if (global_platform.key_down[KEY_TAB])
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        char string[9];
        sprintf(string, "%iX%i", global_platform.window_width, global_platform.window_height);
        print_text(&bmf,string, 0,50, 20);
        sprintf(string, "time: %.2f", global_platform.current_time);
        print_text(&bmf,string, 0,100, 20);
        sprintf(string, "fps: %.2f",freq.QuadPart/ (10000000.f*global_platform.dt));
        print_text(&bmf,string, 0,150, 20);
    }
#endif
}
