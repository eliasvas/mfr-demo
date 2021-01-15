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
#include "shadowmap.h"

static Quad q;
static Model m;
static MeshInfo mesh;
static Camera cam;
static BitmapFont bmf;
static Skybox skybox;
static ShadowMapFBO sfbo;

static mat4 view;
static mat4 proj;
static mat4 ortho;
static mat4 invproj;
static mat4 invview;
static vec4 background_color;
//Here goes any startup code you have.
#include "openexr_write.h"
static void 
init(void)
{
    init_quad(&q, "../assets/terrain.jpg");
    init_fullscreen_quad(&screen_quad, "../assets/red.png");
    init_shadowmap_fbo(&sfbo);
    init_camera(&cam);
    init_abuffer();
    {
        mesh = load_obj("../assets/bunny/stanford_bunny.obj");
        //mesh = load_obj("../assets/utah_teapot.obj");
        init_model_textured_basic(&m, &mesh);
        load_texture(&(m.diff),"../assets/bunny/stanford_bunny.jpg");
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
    proj = perspective_proj(45.f,global_platform.window_width / (f32)global_platform.window_height, 0.1f,50.f); 
    ortho = orthographic_proj(-20, 20, -20, 20, 0.1, 100);
    //background_color = v4(0.4f ,0.3f + fabs(cos(global_platform.current_time)), 0.9f, 1.f); 
    background_color = v4(0.5,0.6,0.7,1.f);

  invproj = inv_mat4(proj);
  invview = inv_mat4(view);


}


void
render_scene(Shader *quad_shader, Shader *mesh_shader)
{
   if (global_platform.key_pressed[KEY_L])
    { //render through stored data
      set_abuffer_data(test_data);
      display_abuffer();
    }else
    { //normal rendering
        mat4 quad_mvp = mul_mat4(proj, mul_mat4(view, mul_mat4(translate_mat4(v3(0,-1,0)),mul_mat4(quat_to_mat4(quat_from_angle(v3(1,0,0), -PI/2)), scale_mat4(v3(100,100,100))))));
        setup_shadowmap(&sfbo, view);
        use_shader(&sfbo.s);
        render_quad_mvp_shader(&q, quad_mvp, &sfbo.s);
        m.position = v3(0,0,5);
        render_model_textured_basic_shader(&m, &proj, &view, &sfbo.s);
        m.position = v3(0,0,2 * sin(global_platform.current_time));
        render_model_textured_basic_shader(&m, &proj, &view, &sfbo.s);
        m.position = v3(0,0,11);
        render_model_textured_basic_shader(&m, &proj, &view, &sfbo.s);
        m.position = v3(0,0,8);
        render_model_textured_basic_shader(&m, &proj, &view, &sfbo.s);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        use_shader(quad_shader);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, sfbo.depth_attachment);
        setInt(&m.s, "shadowMap", 10);
        setInt(quad_shader, "shadowMap", 10);
        setInt(quad_shader, "shadowmap_on", 1);
        setInt(mesh_shader, "shadowmap_on", 1);
        setMat4fv(quad_shader, "lightSpaceMatrix", (GLfloat*)sfbo.lightSpaceMatrix.elements);
        setMat4fv(quad_shader, "invproj", (GLfloat*)invproj.elements);
        setMat4fv(quad_shader, "invview", (GLfloat*)invview.elements);

        setFloat(&quad_shader, "near", 0.1f);
        setFloat(&quad_shader, "far", 50.f);


        setMat4fv(mesh_shader, "lightSpaceMatrix", (GLfloat*)sfbo.lightSpaceMatrix.elements);
        setMat4fv(mesh_shader, "invproj", (GLfloat*)invproj.elements);
        setMat4fv(mesh_shader, "invview", (GLfloat*)invview.elements);

        setFloat(&mesh_shader, "near", 0.1f);
        setFloat(&mesh_shader, "far", 50.f);


        setMat4fv(&m.s, "invproj", (GLfloat*)invproj.elements);
        setMat4fv(&m.s, "invview", (GLfloat*)invview.elements);



        m.position = v3(0,0,5);
        render_quad_mvp_shader(&q, quad_mvp, quad_shader);
        m.position = v3(0,0,5);
        use_shader(mesh_shader);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, sfbo.depth_attachment);
        setInt(mesh_shader, "shadowMap", 10);
        render_abuffer_shad(&m, mesh_shader);
        m.position = v3(0,0,2 * sin(global_platform.current_time));
        render_abuffer_shad(&m, mesh_shader);
        m.position = v3(0,0,11);
        render_abuffer_shad(&m, mesh_shader);
        m.position = v3(0,0,8);
        render_abuffer_shad(&m, mesh_shader);
        display_abuffer();
        clear_abuffer();
    }

}

static void 
render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(background_color.x, background_color.y, background_color.z,background_color.w);
    //render_skybox(&skybox);
     setMat4fv(&render_abuffer_shader, "invproj", (GLfloat*)invproj.elements);
      setMat4fv(&render_abuffer_shader, "invview", (GLfloat*)invview.elements);

    render_scene(&q.shader, &render_abuffer_shader);

    //NOTE: normal rendering
    //render_quad_mvp(&q, mul_mat4(proj,view));
    //render_model_textured_basic(&m,&proj, &view);


    //NOTE: depth-peeling rendering
    //clear_depth_peel_fbos();
    //render_depth_peel();


    if (global_platform.key_down[KEY_TAB])
        print_debug_info(&bmf);
}



