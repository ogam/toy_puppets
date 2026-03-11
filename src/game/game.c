#include "common/types.h"
#include "common/macros.h"
#include "common/memory.h"
#include "common/profiler.h"
#include "common/strings.h"
#include "game/audio.h"
#include "game/assets.h"
#include "game/body.h"
#include "game/condition.h"
#include "game/game.h"
#include "game/items.h"
#include "game/names.h"
#include "game/spells.h"
#include "game/overworld.h"
#include "game/inventory.h"
#include "game/spatial_map.h"
#include "game/world.h"
#include "game/ui.h"
#include "game/game_ui.h"
#include "tools/animation.h"

#include "dcimgui.h"

typedef struct Input
{
    // y is flipped so bottom is 0, top is window height
    CF_V2 screen_position;
    CF_V2 world_position;
    CF_ButtonBinding cast_spell;
    CF_ButtonBinding cancel_spell;
    CF_ButtonBinding place_creature;
    CF_ButtonBinding remove_creature;
} Input;

typedef b32 Debug_State;
enum
{
    Debug_State_Stats,
    Debug_State_Body_Bounds,
    Debug_State_Animation_Tool,
    Debug_State_UI,
};

typedef u8 Cursor_State;
enum
{
    Cursor_State_None,
    Cursor_State_Cast,
};

typedef struct Cursor
{
    CF_Color color;
    Body body;
    CF_Coroutine slap_co;
    Cursor_State state;
} Cursor;

typedef struct Body_Frame
{
    dyna CF_V2* particles;
} Body_Frame;

typedef struct Body_Text
{
    dyna Body_Frame* frames;
    dyna Body* bodies;
    CF_Aabb bounds;
    CF_V2* handle;
    CF_V2* next_handle;
} Body_Text;

typedef struct App
{
    Memory memory;
    Input input;
    Audio audio;
    Assets assets;
    World world;
    UI ui;
    Game_UI game_ui;
    
    Overworld overworld;
    
    Cursor cursor;
    Body_Text body_text;
    
    CF_V2 screen_size;
    Debug_State debug_state;
} App;

App* s_app;

CF_Aabb move_aabb(CF_Aabb aabb, CF_V2 offset)
{
    aabb.min = cf_add(aabb.min, offset);
    aabb.max = cf_add(aabb.max, offset);
    return aabb;
}

void handle_window_events_ex(s32 w, s32 h)
{
    CF_V2 screen_size = cf_v2((f32)w, (f32)h);
    
    if (cf_distance(screen_size, s_app->screen_size) > 0)
    {
        cf_app_set_canvas_size(w, h);
    }
    
    if (!BIT_IS_SET(s_app->debug_state, Debug_State_Animation_Tool))
    {
        if (cf_app_has_focus())
        {
            ImGui_SetMouseCursor(ImGuiMouseCursor_None);
            cf_mouse_hide(true);
        }
        else
        {
            ImGui_SetMouseCursor(ImGuiMouseCursor_Arrow);
            cf_mouse_hide(false);
        }
    }
    
    s_app->screen_size = screen_size;
}

void handle_window_events()
{
#ifndef __EMSCRIPTEN__
    s32 w, h;
    cf_app_get_size(&w, &h);
    
    handle_window_events_ex(w, h);
#endif
}

void init_input()
{
    Input* input = &s_app->input;
    
    input->cast_spell = cf_make_button_binding(0, 0.1f);
    cf_button_binding_add_mouse_button(input->cast_spell, CF_MOUSE_BUTTON_LEFT);
    
    input->cancel_spell = cf_make_button_binding(0, 0.1f);
    cf_button_binding_add_mouse_button(input->cancel_spell, CF_MOUSE_BUTTON_RIGHT);
    
    input->place_creature = cf_make_button_binding(0, 0.1f);
    cf_button_binding_add_mouse_button(input->place_creature, CF_MOUSE_BUTTON_LEFT);
    
    input->remove_creature = cf_make_button_binding(0, 0.1f);
    cf_button_binding_add_mouse_button(input->remove_creature, CF_MOUSE_BUTTON_RIGHT);
}

void update_input()
{
    Input* input = &s_app->input;
    
    world_push_camera();
    
    CF_V2 screen_size = s_app->screen_size;
    CF_V2 screen_position = cf_v2(cf_mouse_x(), cf_mouse_y());
    CF_V2 world_position = cf_screen_to_world(screen_position);
    screen_position.y = screen_size.y - screen_position.y;
    
    input->screen_position = screen_position;
    input->world_position = world_position;
    
    world_pop_camera();
}

void init_body_text()
{
    Body_Text* body_text = &s_app->body_text;
    f32 font_size = 128.0f;
    f32 padding = 2.0f;
    CF_V2 position = cf_v2(-(font_size + padding) * 5, font_size * 2.0f);
    CF_Aabb letter_bounds = cf_make_aabb_from_top_left(position, font_size, font_size);
    CF_V2 offset = cf_v2(font_size + padding, 0.0f);
    
    {
        body_text->bounds = cf_make_aabb_center_half_extents(cf_v2(0, 0), cf_v2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f));
    }
    
    const char* text = "TOY PUPPETS";
    const char* c = text;
    cf_push_font_size(font_size);
    while (*c)
    {
        switch (*c)
        {
            case 'T':
            {
                Body body = make_default_body(Body_Type_Count, 5, font_size);
                body.particles[0] = cf_top_left(letter_bounds);
                body.particles[2] = cf_top(letter_bounds);
                body.particles[1] = cf_lerp(body.particles[0], body.particles[2], 0.5f);
                body.particles[3] = cf_center(letter_bounds);
                body.particles[4] = cf_bottom(letter_bounds);
                
                body.particles[3].x = body.particles[1].x;
                body.particles[4].x = body.particles[1].x;
                
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 0;
                    constraint.b = 1;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 1;
                    constraint.b = 2;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 1;
                    constraint.b = 3;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 3;
                    constraint.b = 4;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                break;
            }
            case 'O':
            {
                Body body = make_default_body(Body_Type_Count, 6, font_size);
                
                body.particles[0] = cf_top_left(letter_bounds);
                body.particles[1] = cf_left(letter_bounds);
                body.particles[2] = cf_bottom_left(letter_bounds);
                body.particles[3] = cf_bottom(letter_bounds);
                body.particles[4] = cf_center(letter_bounds);
                body.particles[5] = cf_top(letter_bounds);
                
                for (s32 index = 0; index < cf_array_count(body.particles); ++index)
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = index % cf_array_count(body.particles);
                    constraint.b = (index + 1) % cf_array_count(body.particles);
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                break;
            }
            case 'Y':
            {
                Body body = make_default_body(Body_Type_Count, 4, font_size);
                body.particles[0] = cf_top_left(letter_bounds);
                body.particles[1] = cf_top(letter_bounds);
                body.particles[2] = cf_center(letter_bounds);
                body.particles[3] = cf_bottom(letter_bounds);
                
                body.particles[2].x = cf_lerp(body.particles[0].x, body.particles[1].x, 0.5f);
                body.particles[3].x = body.particles[2].x;
                
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 0;
                    constraint.b = 2;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 1;
                    constraint.b = 2;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 2;
                    constraint.b = 3;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                break;
            }
            case 'P':
            {
                Body body = make_default_body(Body_Type_Count, 5, font_size);
                body.particles[0] = cf_bottom_left(letter_bounds);
                body.particles[1] = cf_left(letter_bounds);
                body.particles[2] = cf_top_left(letter_bounds);
                body.particles[3] = cf_top(letter_bounds);
                body.particles[4] = cf_center(letter_bounds);
                
                for (s32 index = 0; index < cf_array_count(body.particles) - 1; ++index)
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = index;
                    constraint.b = index + 1;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 4;
                    constraint.b = 1;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                break;
            }
            case 'U':
            {
                Body body = make_default_body(Body_Type_Count, 6, font_size);
                
                body.particles[0] = cf_top_left(letter_bounds);
                body.particles[1] = cf_left(letter_bounds);
                body.particles[2] = cf_bottom_left(letter_bounds);
                body.particles[3] = cf_bottom(letter_bounds);
                body.particles[4] = cf_center(letter_bounds);
                body.particles[5] = cf_top(letter_bounds);
                
                for (s32 index = 0; index < cf_array_count(body.particles) - 1; ++index)
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = index;
                    constraint.b = index + 1;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                break;
            }
            case 'E':
            {
                Body body = make_default_body(Body_Type_Count, 6, font_size);
                body.particles[0] = cf_top_left(letter_bounds);
                body.particles[1] = cf_top(letter_bounds);
                body.particles[2] = cf_left(letter_bounds);
                body.particles[3] = cf_center(letter_bounds);
                body.particles[4] = cf_bottom_left(letter_bounds);
                body.particles[5] = cf_bottom(letter_bounds);
                
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 0;
                    constraint.b = 1;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 0;
                    constraint.b = 2;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 2;
                    constraint.b = 3;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 2;
                    constraint.b = 4;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = 4;
                    constraint.b = 5;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                
                break;
            }
            case 'S':
            {
                Body body = make_default_body(Body_Type_Count, 6, font_size);
                body.particles[0] = cf_top(letter_bounds);
                body.particles[1] = cf_top_left(letter_bounds);
                body.particles[2] = cf_left(letter_bounds);
                body.particles[3] = cf_center(letter_bounds);
                body.particles[4] = cf_bottom(letter_bounds);
                body.particles[5] = cf_bottom_left(letter_bounds);
                
                for (s32 index = 0; index < cf_array_count(body.particles) - 1; ++index)
                {
                    Particle_Constraint constraint = { 0 };
                    constraint.stiffness = 0.8f;
                    constraint.a = index;
                    constraint.b = index + 1;
                    constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
                    constraint.length = constraint.base_length;
                    cf_array_push(body.constraints, constraint);
                }
                
                CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                Body_Frame frame = { 0 };
                cf_array_fit(frame.particles, cf_array_count(body.particles));
                cf_array_setlen(frame.particles, cf_array_count(body.particles));
                
                CF_MEMCPY(frame.particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
                
                cf_array_push(body_text->bodies, body);
                cf_array_push(body_text->frames, frame);
                
                break;
            }
        }
        
        letter_bounds = move_aabb(letter_bounds, offset);
        ++c;
    }
    cf_pop_font_size();
}

void init()
{
    s_app = cf_alloc(sizeof(App));
    CF_MEMSET(s_app, 0, sizeof(App));
    
    init_profiler(64);
    profile_file_stream_begin("trace.json");
    
    init_memory();
    init_audio();
    init_input();
    init_world();
    init_ui();
    init_game_ui();
    
    cf_app_init_imgui();
    
    assets_load_all();
    
    s_app->cursor.color = cf_color_white();
    s_app->cursor.body = make_hand_body(HAND_HEIGHT);
    
    animation_tool_init();
    animation_tool_reset();
    
    init_body_text();
    
    s_app->screen_size = cf_v2(GAME_WIDTH, GAME_HEIGHT);
}

void update_cursor()
{
    World* world = &s_app->world;
    Cursor* cursor = &s_app->cursor;
    
    CF_V2 min = cf_v2(-10000.0f, -10000.0f);
    CF_V2 max = cf_neg(min);
    CF_Aabb bounds = cf_make_aabb(min, max);
    
    Body* body = &cursor->body;
    
    if (world->state == World_State_Arena_In_Progress)
    {
        if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT))
        {
            body_hand_pinch(body);
        }
        else if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT))
        {
            if (cursor->state == Cursor_State_None)
            {
                if (COROUTINE_IS_DONE(cursor->slap_co))
                {
                    cf_destroy_coroutine(cursor->slap_co);
                    cursor->slap_co = cf_make_coroutine(body_hand_slap_co, 0, body);
                }
            }
        }
    }
    else
    {
        // stop any slap animations while not in an active arena
        if (COROUTINE_IS_RUNNING(cursor->slap_co))
        {
            cf_destroy_coroutine(cursor->slap_co);
            cursor->slap_co = (CF_Coroutine){ 0 };
            
            body_hand_slap_end(body);
        }
        
        if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT))
        {
            body_hand_pinch(body);
        }
    }
    
    if (COROUTINE_IS_RUNNING(cursor->slap_co))
    {
        cf_coroutine_resume(cursor->slap_co);
    }
    else
    {
        body_hand_move(body, s_app->input.world_position);
        
        if (cursor->state == Cursor_State_Cast)
        {
            body_hand_cast(body);
        }
    }
    
    body_acceleration_reset(body);
    body_stabilize(body);
    body_verlet(body);
    body_satisfy_constraints(body, bounds);
}

void update_body_text()
{
    Body_Text* body_text = &s_app->body_text;
    if (game_ui_peek_state() == Game_UI_State_Main_Menu)
    {
        CF_V2 world_position = s_app->input.world_position;
        
        // update handles
        {
            f32 closest_distance = F32_MAX;
            CF_V2* next_handle = NULL;
            f32 picking_distance_sq = 10.0f * 10.0f;
            
            for (s32 index = 0; index < cf_array_count(body_text->bodies); ++index)
            {
                Body* body = body_text->bodies + index;
                for (s32 particle_index = 0; particle_index < cf_array_count(body->particles); ++particle_index)
                {
                    CF_V2 dp = cf_sub(world_position, body->particles[particle_index]);
                    f32 distance_sq = cf_len_sq(dp);
                    if (closest_distance > distance_sq && distance_sq <= picking_distance_sq)
                    {
                        closest_distance = distance_sq;
                        next_handle = body->particles + particle_index;
                    }
                }
            }
            
            if (next_handle)
            {
                if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT))
                {
                    body_text->handle = next_handle;
                }
            }
            
            if (cf_mouse_just_released(CF_MOUSE_BUTTON_LEFT))
            {
                body_text->handle = NULL;
            }
            
            if (body_text->handle)
            {
                *body_text->handle = world_position;
            }
            
            body_text->next_handle = next_handle;
        }
        
        // update bodies
        for (s32 index = 0; index < cf_array_count(body_text->bodies); ++index)
        {
            Body* body = body_text->bodies + index;
            Body_Frame* frame = body_text->frames + index;
            
            for (s32 particle_index = 0; particle_index < cf_array_count(body->particles); ++particle_index)
            {
                // current particles doesn't haver mass to slow down the acceleration
                // so these bodies will fling back and forth, so lerp for now..
                body->particles[particle_index] = cf_lerp(body->particles[particle_index], frame->particles[particle_index], body->time_step);
            }
            
            body_verlet(body);
            body_satisfy_constraints(body, body_text->bounds);
            body_acceleration_reset(body);
        }
    }
    else
    {
        // reset
        for (s32 index = 0; index < cf_array_count(body_text->bodies); ++index)
        {
            Body* body = body_text->bodies + index;
            Body_Frame* frame = body_text->frames + index;
            
            CF_MEMCPY(body->prev_particles, frame->particles, sizeof(*frame->particles) * cf_array_count(frame->particles));
            CF_MEMCPY(body->particles, frame->particles, sizeof(*frame->particles) * cf_array_count(frame->particles));
            body_acceleration_reset(body);
        }
    }
}

void draw_body_text()
{
    if (game_ui_peek_state() == Game_UI_State_Main_Menu)
    {
        Body_Text* body_text = &s_app->body_text;
        
        for (s32 index = 0; index < cf_array_count(body_text->bodies); ++index)
        {
            Body* body = body_text->bodies + index;
            body_draw_constraints(body, 5.0f);
        }
        
        cf_draw_push_color(cf_color_grey());
        for (s32 index = 0; index < cf_array_count(body_text->bodies); ++index)
        {
            Body* body = body_text->bodies + index;
            for (s32 particle_index = 0; particle_index < cf_array_count(body->particles); ++particle_index)
            {
                cf_draw_circle_fill2(body->particles[particle_index], 5.0f);
            }
        }
        cf_draw_pop_color();
        
        cf_draw_push_color(cf_color_green());
        if (body_text->handle)
        {
            cf_draw_circle_fill2(*body_text->handle, 5.0f);
        }
        else if (body_text->next_handle)
        {
            cf_draw_circle_fill2(*body_text->next_handle, 5.0f);
        }
        cf_draw_pop_color();
    }
}

void update(void* udata)
{
    PROFILE_BEGIN();
    UNUSED(udata);
    
    scratch_reset();
    handle_window_events();
    update_input();
    
    update_audio();
    
    update_cursor();
    update_world();
    update_body_text();
    
    PROFILE_FUNC(update_game_ui);
    
    if (cf_key_just_pressed(CF_KEY_F1))
    {
        BIT_TOGGLE(s_app->debug_state, Debug_State_Stats);
    }
    if (cf_key_just_pressed(CF_KEY_F2))
    {
        BIT_TOGGLE(s_app->debug_state, Debug_State_Body_Bounds);
    }
    if (cf_key_just_pressed(CF_KEY_F3))
    {
        BIT_TOGGLE(s_app->debug_state, Debug_State_Animation_Tool);
    }
    if (cf_key_just_pressed(CF_KEY_F4))
    {
        BIT_TOGGLE(s_app->debug_state, Debug_State_UI);
        s_app->ui.debug = BIT_IS_SET(s_app->debug_state, Debug_State_UI);
    }
    
    PROFILE_FUNC(draw_world);
    PROFILE_FUNC(draw_ui);
    
    draw_body_text();
    
    // draw cursor hand
    if (BIT_IS_SET(s_app->debug_state, Debug_State_Animation_Tool))
    {
        animation_tool_update();
    }
    else
    {
        world_push_camera();
        Body* body = &s_app->cursor.body;
        
        if (!BIT_IS_SET(s_app->debug_state, Debug_State_Body_Bounds))
        {
            cf_draw_push_color(s_app->cursor.color);
            body_draw(body, make_default_body_proportions());
            cf_draw_pop_color();
        }
        else
        {
            body_draw_constraints(body, 1.0f);
        }
        world_pop_camera();
    }
    
    cf_render_to(cf_app_get_canvas(), false);
    
    PROFILE_END();
}

inline void push_scissor(CF_Aabb bounds)
{
    CF_V2 screen_size = s_app->screen_size;
    CF_V2 scale = cf_v2(screen_size.x / GAME_WIDTH, screen_size.y / GAME_HEIGHT);
    CF_V2 scissor_top_left = cf_top_left(bounds);
    CF_V2 scissor_extents = cf_extents(bounds);
    scissor_extents = cf_mul(scissor_extents, scale);
    
    scissor_top_left = cf_world_to_screen(scissor_top_left);
    
    CF_Rect scissor = { 0 };
    scissor.x = (s32)scissor_top_left.x;
    scissor.y = (s32)scissor_top_left.y;
    scissor.w = (s32)scissor_extents.x;
    scissor.h = (s32)scissor_extents.y;
    
#ifdef __EMSCRIPTEN__
    // emscripten origin is bottom left so need to flip scissor rect
    if (scissor_extents.y - scissor.h > 0.5f)
    {
        ++scissor.h;
    }
    scissor.y = (s32)(screen_size.y - scissor.y - scissor.h);
#endif
    
    cf_draw_push_scissor(scissor);
}

inline void pop_scissor()
{
    cf_draw_pop_scissor();
}

CF_V2 get_cursor_position()
{
    Body* body = &s_app->cursor.body;
    return body->particles[Body_Hand_Index_Tip];
}

#include "common/memory.c"
#include "common/profiler.c"
#include "common/strings.c"
#include "game/audio.c"
#include "game/assets.c"
#include "game/body.c"
#include "game/condition.c"
#include "game/inventory.c"
#include "game/items.c"
#include "game/names.c"
#include "game/spells.c"
#include "game/spatial_map.c"
#include "game/world.c"
#include "game/ui.c"
#include "game/game_ui.c"
#include "game/overworld.c"
#include "tools/animation.c"