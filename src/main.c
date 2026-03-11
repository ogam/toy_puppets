#include <cute.h>

#include "game/game.c"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/html5.h>

bool em_ui_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
    UNUSED(eventType);
    UNUSED(userData);
    handle_window_events_ex(uiEvent->windowInnerWidth, uiEvent->windowInnerHeight);
    return false;
}
#endif

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    int display_index = 0;
    int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
    int width = GAME_WIDTH;
    int height = GAME_HEIGHT;
    
	CF_Result result = cf_make_app("toy puppets", display_index, 0, 0, width, height, options, argv[0]);
    
    if (cf_is_error(result)) return -1;
    
    cf_set_target_framerate(TARGET_FRAMERATE);
    cf_app_set_vsync(true);
    
    init();
    
#ifdef __EMSCRIPTEN__
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, em_ui_resize);
#endif
    while (cf_app_is_running())
    {
        cf_app_update(update);
        cf_app_draw_onto_screen(false);
    }
    
    profile_file_stream_end();
    
    cf_destroy_app();
    
    return 0;
}