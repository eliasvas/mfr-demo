#include "platform.h"
#include "tools.h"
#include "objloader.h"
#include "model.h"
#include "quad.h" 
#include "fbo.h"
#include "renderer.h"
#include "dui.h"
#include "collada_parser.h"
#include "animation.h"
#include "entity.h"
#include "openexr_write.h"
mat4 view,proj;

global Model debug_cube;
global Model light_cube;
global Model sphere;
Renderer rend;

global b32 UI_OPEN;
global f32 trans = 0.f;


f32 write_success_timer = 0.f; //activates successful screenshot message rendering!
b32 DEEP_WRITE = 0; //if 1, take deep screenshot
i32 RES = 1000;
b32 RGBA = 1;
b32 RGB = 0;
b32 PAD = 0;


global Model camera_model;
mat4 camera_translation_mat;
mat4 camera_rotation_mat;
internal void 
init(void)
{
    model_init_cube(&debug_cube);
    renderer_init(&rend);
    model_init_cube(&light_cube);
    model_init_cube(&camera_model);
    light_cube.meshes[0].material.diff = debug_cube.meshes[0].material.spec;
    model_init_sphere(&sphere, 2.f, 20,20);
    dui_default();
    texture_load(&camera_model.meshes[0].material.diff,"../assets/cam.tga");
    camera_translation_mat = mat4_translate(v3(0,5,10));
    camera_rotation_mat = m4d(1.f);
}



internal void 
update(void)
{
  if (DEEP_WRITE)write_success_timer = 3.f;
  else
      write_success_timer = max(0.f, write_success_timer -= global_platform.dt);
  u32 settings = (EXR_RGBA);
  rend.cam.can_rotate = !UI_OPEN;
  renderer_set_deep_write(&rend, DEEP_WRITE, PAD,RGB,RES); //if deep write 1, sets render mode to deep image screenshot
  renderer_begin_frame(&rend);
  vec3 camera_pos = v3(camera_translation_mat.elements[3][0], camera_translation_mat.elements[3][1], camera_translation_mat.elements[3][2]);
  rend.deep_alternate_view = look_at(camera_pos, vec3_add(camera_pos, v3(0,0,-1)), v3(0,1,0));
}

internal void 
render(void)
{
    renderer_push_point_light(&rend,(PointLight){v3(40*sin(global_platform.current_time),5,40*cos(global_platform.current_time)),
        1.f,0.09f,0.0032f,v3(6,5,7),v3(9,8,8),v3(9,8,8),256.f});


    light_cube.model = mat4_translate(v3(40*sin(global_platform.current_time),5,40*cos(global_platform.current_time)));
    //renderer_push_model(&rend, &light_cube);
    
    //renderer_push_model(&rend, &debug_cube);
    //debug_cube.model = mat4_scale(v3(10,1,10));

    renderer_push_model(&rend, &debug_cube);
    debug_cube.model = mat4_mul(mat4_translate(v3(0,5,-1)),mat4_rotate(40, v3(0,1,1)));

    sphere.model = mat4_mul(mat4_translate(v3(0,5,0)),mat4_scale(v3(0.2f,0.2f,0.2f)));
    renderer_push_model(&rend, &sphere);


    mat4 camera_model_mat = mat4_mul(camera_translation_mat, camera_rotation_mat);
    camera_model.model = camera_model_mat;
    if (!rend.deep_write && UI_OPEN)
        renderer_push_model(&rend, &camera_model);

    if (write_success_timer > 0.f)
        renderer_push_text(&rend, v3(0.20,0.80,0.0), v2(0.02,0.02 * (f32)global_platform.window_width/global_platform.window_height), "Data Written Succesfully!");
    dui_frame_begin();
    {
        if (global_platform.key_pressed[KEY_TAB])
            UI_OPEN = !UI_OPEN;
        if (UI_OPEN)
        {
            dui_draw_rect(200, 200, 270, 200, v4(0.1,0.1,0.1,0.9));
            do_slider(GEN_ID, 200 ,300, 4000.f, &RES);
            do_switch(GEN_ID, (dui_Rect){200,270,20,20}, &PAD);
            if (do_switch(GEN_ID, (dui_Rect){200,240,20,20}, &RGBA))RGB = 0;
            if (do_switch(GEN_ID, (dui_Rect){220,240,20,20}, &RGB))RGBA = 0;
            if (RGBA == 0 && RGB == 0)RGB = 1;
            DEEP_WRITE = do_button(GEN_ID, (dui_Rect){260,200,150,30});
            dui_draw_string(260, 370, "screenshot");
            dui_draw_string(190, 330, "resolution");
            dui_draw_string(215, 275, "padding");
            dui_draw_string(230, 240, "RGB/RGBA");
            dui_draw_string(280, 210, "CAPTURE");
            char ms[64];
            sprintf(ms, "%.4f ms", global_platform.dt);
            renderer_push_text(&rend, v3(0.82,0.90,0.0), v2(0.015,0.015 * global_platform.window_width/(f32)global_platform.window_height), ms);

            do_slider_float(GEN_ID, 800 ,300, 100.f, &camera_translation_mat.elements[3][0]);
            do_slider_float(GEN_ID, 800 ,280, 100.f, &camera_translation_mat.elements[3][1]);
            do_slider_float(GEN_ID, 800 ,260, 100.f, &camera_translation_mat.elements[3][2]);
        }
    }
    do_switch(GEN_ID, (dui_Rect){0,0,100,100}, &UI_OPEN);

    dui_frame_end();

    f32 far_plane = 20.f;
    f32 near_plane = 0.001f;
    f32 right = 5.f;
    f32 top = 5.f;
    vec3 camera_pos = v3(camera_translation_mat.elements[3][0],camera_translation_mat.elements[3][1],camera_translation_mat.elements[3][2]);
    if (UI_OPEN)
    {
        renderer_push_line(&rend, vec3_add(camera_pos, v3(-right, -top, near_plane)), vec3_add(camera_pos, v3(-right, -top, -far_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(-right, top, near_plane)), vec3_add(camera_pos, v3(-right, top, -far_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(right, -top, near_plane)), vec3_add(camera_pos, v3(right, -top, -far_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(right, top, near_plane)), vec3_add(camera_pos, v3(right, top, -far_plane)),v4(0.9,0.2,0.2,1.f));

        renderer_push_line(&rend, vec3_add(camera_pos, v3(right, top, near_plane)), vec3_add(camera_pos, v3(-right, top, near_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(-right, top, near_plane)), vec3_add(camera_pos, v3(-right, -top, near_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(-right, -top, near_plane)), vec3_add(camera_pos, v3(right, -top, near_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(right, -top, near_plane)), vec3_add(camera_pos, v3(right, top, near_plane)),v4(0.9,0.2,0.2,1.f));

        renderer_push_line(&rend, vec3_add(camera_pos, v3(right, top, -far_plane)), vec3_add(camera_pos, v3(-right, top, -far_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(-right, top, -far_plane)), vec3_add(camera_pos, v3(-right, -top, -far_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(-right, -top, -far_plane)), vec3_add(camera_pos, v3(right, -top, -far_plane)),v4(0.9,0.2,0.2,1.f));
        renderer_push_line(&rend, vec3_add(camera_pos, v3(right, -top, -far_plane)), vec3_add(camera_pos, v3(right, top, -far_plane)),v4(0.9,0.2,0.2,1.f));
    }

    renderer_end_frame(&rend);
}

