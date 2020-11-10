
πρακτικα αυτο είναι το βασικό OS Interface του engine οτι θέλεις
να Pollαρεις το βρείσκεις απο δώ πχ αν έχει πατηθεί ένα κουμπι κάνεις:

if (platform.key_pressed[KEY_K])
    printf("Key K pressed.\n");

typedef struct Platform
{
    i32 window_width; //width of window (width border)
    i32 window_height; //height of window (with border)
    b32 exit; //whether we should exit
    b32 vsync; //whether the app has vsync (capped FPS.. kinda)
    b32 fullscreen; //whether the app is in fullscreen
    b32 initialized; //whether the app is initialized
    b32 window_resized; //whether the window was resize last frame
    f32 target_fps; //in how many frames should the app run
    f32 current_time; //current time in milliseconds

    f32 mouse_x; //mouse x pos wrt top left corner
    f32 mouse_y; //mouse y coordinate wrt top left corner
    b8 left_mouse_down; //whether LMB is down 
    b8 right_mouse_down; //whether RMB is down
    b8 key_down[KEY_MAX]; //whether a key is down
    b8 key_pressed[KEY_MAX]; //whether a key was JUST pressed
    i32 last_key; //the last key pressed
    b32 keyboard_used; //whether keyboard is used..
    f32 dt; //time between frames in milliseconds


    Arena permanent_storage; //an arena to store permanent data, never flushed
    Arena frame_storage; //an arena to store frame data, flushed EVERY frame
}Platform;


