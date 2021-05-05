#include "platform.h"
#include "tools.h"
#include "objloader.h"
#include "model.h"
#include "quad.h" 
#include "fbo.h"
#include "renderer.h"
#include "dui.h"
#include "entity.h"
#include "openexr_write.h"
mat4 view,proj;

Renderer rend;

global b32 UI_OPEN;
global f32 trans = 0.f;


f32 write_success_timer = 0.f; //activates successful screenshot message rendering!
b32 DEEP_WRITE = 0; //if 1, take deep screenshot
b32 DEEP_READ = 0;
i32 RES = 1000;
b32 RGBA = 1;
b32 RGB = 0;
b32 PAD = 0;


global Model camera_model;
mat4 camera_translation_mat;
mat4 camera_rotation_mat;


global RendererPointData *points;
vec3 middle;
vec3 points_position;
u32 points_count;
global u32 point_size;

global EntityManager entity_manager;

global char path[256];


/*TODO:
 * lines are occluded, lines should be rendered last????
 * make renderer_points moving better
 * */
internal void 
init(void)
{
    entity_manager_init(&entity_manager);
    renderer_init(&rend);
    /*
    Model *m = entity_add_model(&entity_manager.model_manager,entity_create(&entity_manager));
    model_init_sphere(m, 2.f, 20,20);
    m->model = mat4_mul(mat4_translate(v3(0,5,0)),mat4_scale(v3(0.2f,0.2f,0.2f)));

    m = entity_add_model(&entity_manager.model_manager,entity_create(&entity_manager));
    model_init_cube(m);
    m->model = mat4_mul(mat4_translate(v3(0,5,-1)),mat4_rotate(40, v3(0,1,1)));
    */

    scene_init("../assets/scene.txt", &entity_manager);

    dui_default();
    model_init_cube(&camera_model);
    texture_load(&camera_model.meshes[0].material.diff,"../assets/cam.tga");
    points = deepexr_read("../build/image.exr", &points_count);
    point_size = 0;
    points_position = v3(0,0,0);
}



internal void 
update(void)
{
    entity_manager_update(&entity_manager, &rend);
    camera_update_3p(&rend.deep_cam);
  if (DEEP_WRITE)write_success_timer = 3.f;
  else
      write_success_timer = max(0.f, write_success_timer -= global_platform.dt);
  if (global_platform.key_pressed[KEY_Q])
      rend.deep_cam = rend.cam;
  u32 settings = (EXR_RGBA);
  rend.cam.can_rotate = !UI_OPEN;
  renderer_set_deep_write(&rend, DEEP_WRITE, PAD,RGB,RES); //if deep write 1, sets render mode to deep image screenshot
  renderer_begin_frame(&rend);

  if(DEEP_READ) 
    {
      points = deepexr_read(path, &points_count);
      //update_renderer_points(points, points_count);
    }

    if (!points) points_count = 0;
}

internal void 
render(void)
{

    entity_manager_render(&entity_manager, &rend);
    renderer_push_point_light(&rend,(PointLight){v3(40*sin(global_platform.current_time),5,40*cos(global_platform.current_time)),
        1.f,0.09f,0.0032f,v3(6,5,7),v3(9,8,8),v3(9,8,8),256.f});


    
    //renderer_push_model(&rend, &debug_cube);
    //debug_cube.model = mat4_scale(v3(10,1,10));

    //renderer_push_model(&rend, &debug_cube);

    //renderer_push_model(&rend, &sphere);


    camera_model.model = mat4_inv(get_view_mat(&rend.deep_cam));
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
            do_slider(GEN_ID, 200 ,300, 2000.f, &RES);
            do_switch(GEN_ID, (dui_Rect){200,270,20,20}, &rend.renderer_settings.render_mode);
            if (do_switch(GEN_ID, (dui_Rect){200,240,20,20}, &RGBA))RGB = 0;
            if (do_switch(GEN_ID, (dui_Rect){220,240,20,20}, &RGB))RGBA = 0;
            if (RGBA == 0 && RGB == 0)RGB = 1;
            DEEP_WRITE = do_button(GEN_ID, (dui_Rect){260,200,150,30});
            dui_draw_string(260, 370, "screenshot");
            dui_draw_string(190, 330, "resolution");
            dui_draw_string(215, 275, "padding");
            dui_draw_string(230, 240, "RGB/RGBA");
            dui_draw_string(280, 210, "CAPTURE");

            dui_draw_rect(800, 200, 270, 200, v4(0.1,0.1,0.1,0.9));
            do_slider_float(GEN_ID, 800 ,240,50.f, &rend.deep_far);
            do_slider_float(GEN_ID, 800 ,260,50.f, &rend.deep_right);
            do_slider(GEN_ID, 800 ,220, 10, &point_size);
            DEEP_READ = do_button(GEN_ID, (dui_Rect){808,350,50,20})
                        || do_textfield(GEN_ID, 800, 360, path);

            char ms[64];
            sprintf(ms, "%.4f ms", global_platform.dt);
            renderer_push_text(&rend, v3(0.82,0.90,0.0), v2(0.015,0.015 * global_platform.window_width/(f32)global_platform.window_height), ms);
        }
    }
    do_switch(GEN_ID, (dui_Rect){0,0,100,100}, &UI_OPEN);

    dui_frame_end();

    f32 right = 5.f;
    f32 top = 5.f;
    if (UI_OPEN)
    {
        
        vec3 right = vec3_mulf(vec3_normalize(vec3_cross(rend.deep_cam.front, rend.deep_cam.up)), rend.deep_right);
        vec3 up = vec3_mulf(vec3_normalize(rend.deep_cam.up), rend.deep_right);
        vec3 farr = vec3_mulf(vec3_normalize(rend.deep_cam.front), rend.deep_far);
        up = vec3_cross(right, rend.deep_cam.front);

        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(right, up)), 
                vec3_add(vec3_add(rend.deep_cam.pos, vec3_add(right, up)), farr), v4(0.9,0.2,0.2,1.0));
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), up)), 
                vec3_add(vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), up)), farr)), v4(0.9,0.2,0.2,1.0);
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(right, vec3_mulf(up, -1.f))), 
                vec3_add(vec3_add(rend.deep_cam.pos, vec3_add(right, vec3_mulf(up,-1.f))), farr), v4(0.9,0.2,0.2,1.0));
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), vec3_mulf(up, -1.f))), 
                vec3_add(vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), vec3_mulf(up,-1.f))), farr), v4(0.9,0.2,0.2,1.0));

        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), vec3_mulf(up, -1.f))), 
                vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,1.f), vec3_mulf(up, -1.f))), v4(0.9,0.2,0.2,1));
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(right, vec3_mulf(up, -1.f))), 
                vec3_add(rend.deep_cam.pos, vec3_add(right, up)), v4(0.9,0.2,0.2,1));
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(right, up)), 
                vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), up))), v4(0.9,0.2,0.2,1.0);
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), up)), 
                vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), vec3_mulf(up, -1.f))),v4(0.9,0.2,0.2,1)); 

        renderer_push_line(&rend, vec3_add(farr,vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), vec3_mulf(up, -1.f)))), 
                vec3_add(farr,vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,1.f), vec3_mulf(up, -1.f)))), v4(0.9,0.2,0.2,1));
        renderer_push_line(&rend, vec3_add(farr,vec3_add(rend.deep_cam.pos, vec3_add(right, vec3_mulf(up, -1.f)))), 
                vec3_add(rend.deep_cam.pos, vec3_add(right, vec3_add(up,farr))), v4(0.9,0.2,0.2,1));
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(vec3_add(right,farr), up)), 
                vec3_add(rend.deep_cam.pos, vec3_add(vec3_mulf(right,-1.f), vec3_add(up,farr)))), v4(0.9,0.2,0.2,1.0);
        renderer_push_line(&rend, vec3_add(rend.deep_cam.pos, vec3_add(vec3_add(vec3_mulf(right,-1.f), up),farr)), 
                vec3_add(rend.deep_cam.pos, vec3_add(vec3_add(vec3_mulf(right,-1.f), vec3_mulf(up, -1.f)),farr)),v4(0.9,0.2,0.2,1)); 
    }
    glPointSize(point_size);

    //points_position.y += sin(global_platform.current_time) * global_platform.dt;
    //rend.point_alloc_pos = 0;
    if (points_count != rend.point_alloc_pos)
    {
        rend.points_updated = TRUE;
        rend.point_alloc_pos = 0;
        RendererPointData p;
        f64 point_accum[3] = {0,0,0};
        for (u32 i = 0; i < points_count; ++i)
        {
            p = points[i];
            p.pos = vec3_add(p.pos, points_position);
            point_accum[0] += p.pos.x;
            point_accum[1] += p.pos.y;
            point_accum[2] += p.pos.z;
            renderer_push_point(&rend, p);
        }
        if (global_platform.current_time > 1.f)sprintf(info_log, "point_accum: %f", point_accum);
        point_accum[0] /= points_count;
        point_accum[1] /= points_count;
        point_accum[2] /= points_count;

        middle = v3(point_accum[0], point_accum[1], point_accum[2]);
    }
    else
        rend.points_updated = FALSE;

    //renderer_push_compass(&rend, middle);

    renderer_end_frame(&rend);
}

