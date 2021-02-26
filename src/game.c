#include "platform.h"
#include "tools.h"
#include "objloader.h"
#include "model.h"
#include "quad.h" 
#include "fbo.h"
#include "renderer.h"
#include "collada_parser.h"
#include "animation.h"
#include "entity.h"
mat4 view,proj;

global Model debug_cube;
global Model light_cube;
global Model sphere;
global Renderer rend;

global b32 UI_OPEN;
global f32 trans = 0.f;


f32 write_success_timer = 0.f; //activates successful screenshot message rendering!
b32 DEEP_WRITE = 0; //if 1, take deep screenshot

internal void 
init(void)
{
    model_init_cube(&debug_cube);
    renderer_init(&rend);
    model_init_cube(&light_cube);
    light_cube.meshes[0].material.diff = debug_cube.meshes[0].material.spec;
    model_init_sphere(&sphere, 2.f, 20,20);

}



internal void 
update(void)
{
  DEEP_WRITE = global_platform.key_pressed[KEY_Q];
  if (DEEP_WRITE)write_success_timer = 3.f;
  else
      write_success_timer = max(0.f, write_success_timer -= global_platform.dt);
  renderer_set_deep_write(&rend, DEEP_WRITE); //if deep write 1, sets render mode to deep image screenshot
  renderer_begin_frame(&rend);
}

internal void 
render(void)
{
    renderer_push_point_light(&rend,(PointLight){v3(40*sin(global_platform.current_time),5,40*cos(global_platform.current_time)),
        1.f,0.09f,0.0032f,v3(6,5,7),v3(9,8,8),v3(9,8,8),256.f});


    light_cube.model = mat4_translate(v3(40*sin(global_platform.current_time),5,40*cos(global_platform.current_time)));
    renderer_push_model(&rend, &light_cube);
    
    //renderer_push_model(&rend, &debug_cube);
    //debug_cube.model = mat4_scale(v3(10,1,10));

    renderer_push_model(&rend, &debug_cube);
    debug_cube.model = mat4_mul(mat4_translate(v3(0,5,-1)),mat4_rotate(40, v3(0,1,1)));

    sphere.model = mat4_mul(mat4_translate(v3(0,5,0)),mat4_scale(v3(0.2f,0.2f,0.2f)));
    renderer_push_model(&rend, &sphere);

    if (write_success_timer > 0.f)
        renderer_push_text(&rend, v3(0.20,0.80,0.0), v2(0.02,0.025), "Data Written Succesfully!");
    //UI bullshit..
    {
        if (global_platform.key_pressed[KEY_TAB])
            UI_OPEN = !UI_OPEN;
        if (UI_OPEN)
        {
            f32 x_off = 0.05f;
            renderer_push_filled_rect(&rend, v3(0.f + x_off,0.5f, 0.f), v2(0.25f,0.25f),v4(0.2f,0.2f,0.2f,0.9f));
            renderer_push_line(&rend, v3(0.f + x_off,0.5f,0.f), v3(0.25f + x_off,0.5f,0.0), v4(1.f, 0.5f,0.5f,0.9f));
            renderer_push_line(&rend, v3(0.f + x_off,0.5f,0.f), v3(0.f + x_off,1.f,0.0), v4(1.f, 0.5f,0.5f,0.9f));
            renderer_push_line(&rend, v3(0.f + x_off,1.f,0.f), v3(0.25f + x_off,1.f,0.0), v4(1.f, 0.5f,0.5f,0.9f));
            renderer_push_line(&rend, v3(0.25f + x_off,0.5f,0.f), v3(0.25f + x_off,1.f,0.0), v4(1.f, 0.5f,0.5f,0.9f));
            renderer_push_text(&rend, v3(0.05,0.70,0.0), v2(0.02,0.025), "screenshot");
            renderer_push_text(&rend, v3(0.05,0.65,0.0), v2(0.015,0.020), "-format");
            renderer_push_text(&rend, v3(0.05,0.60,0.0), v2(0.015,0.020), "-padding");

            char ms[32];
            f32 msf = global_platform.dt * 100.f;
            sprintf(ms, "%.4f ms", msf);
            renderer_push_text(&rend, v3(0.82,0.90,0.0), v2(0.015,0.025), ms);
        }
    }

    renderer_end_frame(&rend);
}

