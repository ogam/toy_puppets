#include "tools/animation.h"

typedef struct Animation_Tool
{
    Body body;
    Body_Proportions proportions;
    CF_Coroutine action_co;
    
    Body_Type body_type;
    b32 run;
    b32 draw_constraints;
    CF_V2* handle;
} Animation_Tool;

Animation_Tool s_animation_tool;

void animation_human_body_update()
{
    static f32 wheel_angle = 0.5f * CF_PI;
    static f32 stride_width = 20.0f;
    static f32 stride_height = 10.0f;
    static f32 spin_speed = 0.025f;
    static f32 move_speed = 100.0f;
    
    Body* body = &s_animation_tool.body;
    Body_Proportions* proportions = &s_animation_tool.proportions;
    
    ImGui_Begin("Animation", 0, 0);
    {
        b32 try_kick = ImGui_Button("Kick");
        ImGui_SameLine();
        b32 try_punch = ImGui_Button("Punch");
        ImGui_SameLine();
        b32 try_fix_head = ImGui_Button("Fix Head");
        CF_CoroutineFn* co_fn = NULL;
        
        if (try_kick)
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                co_fn = body_human_kick_co;
            }
        }
        
        if (try_punch)
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                co_fn = body_human_punch_co;
            }
        }
        
        if (try_fix_head)
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                co_fn = body_human_fix_head_co;
            }
        }
        
        if (co_fn)
        {
            cf_destroy_coroutine(s_animation_tool.action_co);
            s_animation_tool.action_co = cf_make_coroutine(co_fn, 0, body);
        }
        
        ImGui_SliderFloat("Stride Width", &stride_width, 0.0f, 200.0f);
        ImGui_SliderFloat("Stride Height", &stride_height, 0.0f, 200.0f);
        ImGui_SliderFloat("Spin Speed", &spin_speed, 0.0f, 1.0f);
        ImGui_SliderFloat("Move Speed", &move_speed, 0.0f, 500.0f);
        ImGui_SliderFloat("Ground Height", &body->position.y, -400.0f, 400.0f);
        ImGui_Separator();
        b32 do_rescale = false;
        do_rescale |= ImGui_SliderFloat("Upper Arm Length", &proportions->upper_arm_length, 0.1f, 2.0f);
        do_rescale |= ImGui_SliderFloat("Lower Arm Length", &proportions->lower_arm_length, 0.1f, 2.0f);
        do_rescale |= ImGui_SliderFloat("Upper Leg Length", &proportions->upper_leg_length, 0.1f, 2.0f);
        do_rescale |= ImGui_SliderFloat("Lower Leg Length", &proportions->lower_leg_length, 0.1f, 2.0f);
        do_rescale |= ImGui_SliderFloat("Leg Length", &proportions->leg_length, 0.1f, 2.0f);
        ImGui_Separator();
        ImGui_SliderFloat("Head Scale", &proportions->head_scale, 0.1f, 2.0f);
        ImGui_SliderFloat("Neck Thickness", &proportions->neck_thickness, 0.1f, 2.0f);
        ImGui_SliderFloat("Torso Chubbiness", &proportions->torso_chubbiness, 0.1f, 2.0f);
        ImGui_SliderFloat("Upper Arm Thickness", &proportions->upper_arm_thickness, 0.1f, 2.0f);
        ImGui_SliderFloat("Lower Arm Thickness", &proportions->lower_arm_thickness, 0.1f, 2.0f);
        ImGui_SliderFloat("Hand Thickness", &proportions->hand_thickness, 0.1f, 2.0f);
        ImGui_SliderFloat("Upper Leg Thickness", &proportions->upper_leg_thickness, 0.1f, 2.0f);
        ImGui_SliderFloat("Lower Leg Thickness", &proportions->lower_leg_thickness, 0.1f, 2.0f);
        ImGui_SliderFloat("Foot Thickness", &proportions->foot_thickness, 0.1f, 2.0f);
        
        if (ImGui_Button("Copy and Dump Proportions"))
        {
            const char* text = scratch_fmt("proportions.scale = %.3ff;\n" \
                                           "proportions.head_scale = %.3ff;\n" \
                                           "proportions.neck_thickness = %.3ff;\n" \
                                           "proportions.torso_chubbiness = %.3ff;\n" \
                                           "proportions.upper_arm_thickness = %.3ff;\n" \
                                           "proportions.lower_arm_thickness = %.3ff;\n" \
                                           "proportions.hand_thickness = %.3ff;\n" \
                                           "proportions.upper_leg_thickness = %.3ff;\n" \
                                           "proportions.lower_leg_thickness = %.3ff;\n" \
                                           "proportions.foot_thickness = %.3ff;\n" \
                                           "proportions.upper_arm_length = %.3ff;\n" \
                                           "proportions.lower_arm_length = %.3ff;\n" \
                                           "proportions.upper_leg_length = %.3ff;\n" \
                                           "proportions.lower_leg_length = %.3ff;\n" \
                                           "proportions.leg_length = %.3ff;\n",
                                           proportions->scale,
                                           proportions->head_scale,
                                           proportions->neck_thickness,
                                           proportions->torso_chubbiness,
                                           proportions->upper_arm_thickness,
                                           proportions->lower_arm_thickness,
                                           proportions->hand_thickness,
                                           proportions->upper_leg_thickness,
                                           proportions->lower_leg_thickness,
                                           proportions->foot_thickness,
                                           proportions->upper_arm_length,
                                           proportions->lower_arm_length,
                                           proportions->upper_leg_length,
                                           proportions->lower_leg_length,
                                           proportions->leg_length
                                           );
            
            cf_clipboard_set(text);
            printf("%s", text);
        }
        
        if (do_rescale)
        {
            body_scale(body, *proportions);
        }
    }
    ImGui_End();
    
    CF_V2 direction = cf_v2(0, 0);
    if (cf_key_down(CF_KEY_W))
    {
        direction.y += 1.0f;
    }
    if (cf_key_down(CF_KEY_S))
    {
        direction.y += -1.0f;
    }
    if (cf_key_down(CF_KEY_A))
    {
        direction.x += -1.0f;
    }
    if (cf_key_down(CF_KEY_D))
    {
        direction.x += 1.0f;
    }
    
    CF_V2 stride = cf_v2(stride_width, stride_height);
    
    if (cf_len_sq(direction) == 0.0f)
    {
        body_human_idle(body, stride, spin_speed);
    }
    else
    {
        body_human_walk(body, direction, stride, move_speed, spin_speed);
    }
}

void animation_tentacle_body_update()
{
    Body* body = &s_animation_tool.body;
    
    ImGui_Begin("Animation", 0, 0);
    {
        if (ImGui_Button("Whip"))
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                cf_destroy_coroutine(s_animation_tool.action_co);
                s_animation_tool.action_co = cf_make_coroutine(body_tentacle_whip_co, 0, body);
            }
        }
    }
    ImGui_End();
    
    if (COROUTINE_IS_DONE(s_animation_tool.action_co))
    {
        body_tentacle_idle(body);
    }
    
    body->particles[Body_Tentacle_Root] = body->position;
    body->is_locked[Body_Tentacle_Root] = true;
}

void animation_slime_body_update()
{
    static f32 move_speed = 100.0f;
    
    Body* body = &s_animation_tool.body;
    
    ImGui_Begin("Animation", 0, 0);
    {
        ImGui_SliderFloat("Move Speed", &move_speed, 0.0f, 500.0f);
        
        if (ImGui_Button("Lunge"))
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                cf_destroy_coroutine(s_animation_tool.action_co);
                s_animation_tool.action_co = cf_make_coroutine(body_slime_lunge_co, 0, body);
            }
        }
        ImGui_SameLine();
        if (ImGui_Button("Spike"))
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                cf_destroy_coroutine(s_animation_tool.action_co);
                s_animation_tool.action_co = cf_make_coroutine(body_slime_forward_spike_co, 0, body);
            }
        }
    }
    ImGui_End();
    
    CF_V2 direction = cf_v2(0, 0);
    if (cf_key_down(CF_KEY_W))
    {
        direction.y += 1.0f;
    }
    if (cf_key_down(CF_KEY_S))
    {
        direction.y += -1.0f;
    }
    if (cf_key_down(CF_KEY_A))
    {
        direction.x += -1.0f;
    }
    if (cf_key_down(CF_KEY_D))
    {
        direction.x += 1.0f;
    }
    
    body_slime_walk(body, direction, move_speed);
}

void animation_hand_body_update()
{
    Body* body = &s_animation_tool.body;
    
    ImGui_Begin("Animation", 0, 0);
    {
        ImGui_Text("Left Click to Pinch");
        ImGui_Text("Shift + Left Click to Gesture Draw");
        ImGui_Text("Right Click to Slap");
    }
    ImGui_End();
    
    body_hand_move(body, s_app->input.world_position);
    
    if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT))
    {
        if (cf_key_shift())
        {
            body_hand_gesture(body);
        }
        else
        {
            body_hand_pinch(body);
        }
    }
    if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT))
    {
        if (COROUTINE_IS_DONE(s_animation_tool.action_co))
        {
            cf_destroy_coroutine(s_animation_tool.action_co);
            s_animation_tool.action_co = cf_make_coroutine(body_hand_slap_co, 0, body);
        }
    }
}

void animation_tubeman_body_update()
{
    static f32 blow_turn_rate = 1.0f;
    static f32 blow_force_x = 50.0f;
    static f32 blow_force_y = 150.0f;
    static f32 blow_duration = 0.0f;
    
    Body* body = &s_animation_tool.body;
    
    ImGui_Begin("Animation", 0, 0);
    {
        if (ImGui_Button("Blow"))
        {
            blow_duration = 15.0f;
        }
        ImGui_SameLine();
        if (ImGui_Button("Punch"))
        {
            if (COROUTINE_IS_DONE(s_animation_tool.action_co))
            {
                cf_destroy_coroutine(s_animation_tool.action_co);
                s_animation_tool.action_co = cf_make_coroutine(body_tubeman_punch_co, 0, body);
            }
        }
        
        ImGui_SliderFloat("Blow Turn Rate", &blow_turn_rate, 0.0f, 100.0f);
        ImGui_SliderFloat("Blow Force X", &blow_force_x, 0.0f, 5000.0f);
        ImGui_SliderFloat("Blow Force Y", &blow_force_y, 0.0f, 5000.0f);
        
        if (blow_duration > 0)
        {
            ImGui_Text("Blow Time Remaining %.2f", blow_duration);
        }
    }
    ImGui_End();
    
    body->is_locked[Body_Tubeman_Segment_Left_0] = true;
    body->is_locked[Body_Tubeman_Segment_Right_0] = true;
    
    if (blow_duration > 0)
    {
        f32 accel_x = cf_sin((f32)CF_SECONDS * blow_turn_rate) > 0.0f ? blow_force_x : 0.0f;
        CF_V2 force = cf_v2(accel_x, blow_force_y);
        
        body_tubeman_sway(body, force);
        
        blow_duration = cf_max(blow_duration - CF_DELTA_TIME, 0.0f);
    }
}

void animation_body_update()
{
    Body* body = &s_animation_tool.body;
    switch (s_animation_tool.body_type)
    {
        case Body_Type_Human:
        {
            animation_human_body_update();
            body_human_hand_guard(body);
            break;
        }
        case Body_Type_Tentacle:
        {
            animation_tentacle_body_update();
            break;
        }
        case Body_Type_Slime:
        {
            animation_slime_body_update();
            break;
        }
        case Body_Type_Hand:
        {
            animation_hand_body_update();
            break;
        }
        case Body_Type_Tubeman:
        {
            animation_tubeman_body_update();
            break;
        }
    }
}

void animation_tool_init()
{
    s_animation_tool.body = make_human_body(HUMAN_HEIGHT);
    
    s_animation_tool.handle = NULL;
    s_animation_tool.action_co = (CF_Coroutine){ 0 };
}

void animation_tool_reset()
{
    destroy_body(&s_animation_tool.body);
    
    switch (s_animation_tool.body_type)
    {
        case Body_Type_Tentacle:
        {
            s_animation_tool.body = make_tentacle_body(TENTACLE_HEIGHT);
            break;
        }
        case Body_Type_Slime:
        {
            s_animation_tool.body = make_slime_body(SLIME_HEIGHT);
            break;
        }
        case Body_Type_Hand:
        {
            s_animation_tool.body = make_hand_body(HAND_HEIGHT);
            break;
        }
        case Body_Type_Tubeman:
        {
            s_animation_tool.body = make_tubeman_body(TUBEMAN_HEIGHT);
            break;
        }
        default:
        case Body_Type_Human:
        {
            s_animation_tool.body = make_human_body(HUMAN_HEIGHT);
            break;
        }
    }
    s_animation_tool.handle = NULL;
    
    s_animation_tool.proportions = make_default_body_proportions();
    
    cf_destroy_coroutine(s_animation_tool.action_co);
    s_animation_tool.action_co = (CF_Coroutine){ 0 };
}

void animation_tool_update()
{
    world_push_camera();
    
    ImGui_PushFontFloat(NULL, 24.0f);
    
    animation_tool_ui();
    
    CF_V2 game_resolution = cf_v2(GAME_WIDTH, GAME_HEIGHT);
    CF_V2 min = cf_mul_v2_f(game_resolution, -0.4f);
    CF_V2 max = cf_mul_v2_f(game_resolution,  0.4f);
    CF_Aabb bounds = cf_make_aabb(min, max);
    Body* body = &s_animation_tool.body;
    body_acceleration_reset(body);
    
    b32 is_attacking = COROUTINE_IS_RUNNING(s_animation_tool.action_co);
    
    b32 is_human = body->type == Body_Type_Human;
    b32 is_tentacle = body->type == Body_Type_Tentacle;
    b32 is_teleporting = cf_key_down(CF_KEY_T);
    
    CF_V2 world_position = s_app->input.world_position;
    
    if (!is_attacking)
    {
        animation_body_update();
    }
    
    if (is_attacking)
    {
        cf_coroutine_resume(s_animation_tool.action_co);
    }
    
    if (cf_key_just_pressed(CF_KEY_SPACE) || cf_key_down(CF_KEY_Z) ||
        s_animation_tool.run)
    {
        CF_Aabb local_bounds = bounds;
        local_bounds.min.y = cf_max(bounds.min.y, body->position.y);
        
        if (body->type == Body_Type_Hand)
        {
            local_bounds = cf_make_aabb(cf_v2(-10000.0f, -10000.0f), cf_v2(10000.0f, 10000.0f));
        }
        
        body_stabilize(body);
        body_verlet(body);
        body_satisfy_constraints(body, local_bounds);
    }
    
    if (is_teleporting)
    {
        body_teleport(body, world_position);
    }
    
    // handles
    {
        f32 min_grab_radius = 10.0f;
        f32 closest_distance = F32_MAX;
        CF_V2* closest_handle = NULL;
        for (s32 index = 0; index < cf_array_count(body->particles); ++index)
        {
            f32 distance = cf_distance(body->particles[index], world_position);
            if (distance < closest_distance && distance < min_grab_radius)
            {
                distance = closest_distance;
                closest_handle = body->particles + index;
            }
        }
        
        if (closest_handle)
        {
            if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT))
            {
                s_animation_tool.handle = closest_handle;
            }
        }
        
        if (cf_mouse_just_released(CF_MOUSE_BUTTON_LEFT))
        {
            s_animation_tool.handle = NULL;
        }
        
        if (s_animation_tool.handle)
        {
            *s_animation_tool.handle = world_position;
        }
    }
    
    CF_V2 centeroid = body_centeroid(body);
    CF_V2 prev_centeroid = body_prev_centeroid(body);
    
    cf_draw_box(bounds, 1.0f, 0.0f);
    
    if (s_animation_tool.draw_constraints)
    {
        body_draw_constraints(body, 1.0f);
    }
    else
    {
        body_draw(body, s_animation_tool.proportions);
    }
    
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        if (s_animation_tool.handle == body->particles + index)
        {
            cf_draw_push_color(cf_color_green());
        }
        else
        {
            cf_draw_push_color(cf_color_grey());
        }
        cf_draw_circle2(body->particles[index], 1.0f, 1.0f);
        cf_draw_pop_color();
    }
    
    cf_draw_push_color(cf_color_brown());
    cf_draw_circle_fill2(prev_centeroid, 1.0f);
    cf_draw_pop_color();
    
    cf_draw_push_color(cf_color_green());
    cf_draw_circle_fill2(centeroid, 1.0f);
    cf_draw_pop_color();
    
    ImGui_PopFont();
    
    world_pop_camera();
}

void animation_tool_ui()
{
    Body* body = &s_animation_tool.body;
    
    ImGui_Begin("Animation", 0, 0);
    {
        bool is_facing_right = body->facing_direction > 0;
        
        if (ImGui_ComboChar("Body", &s_animation_tool.body_type, Body_Type_names, Body_Type_Count))
        {
            animation_tool_reset();
        }
        
        if (ImGui_SliderFloat("Scale", &s_animation_tool.proportions.scale, 0.25f, 2.0f))
        {
            body_scale(body, s_animation_tool.proportions);
        }
        
        if (ImGui_Checkbox("Face Right", &is_facing_right))
        {
            if (is_facing_right)
            {
                body->facing_direction = 1.0f;
            }
            else
            {
                body->facing_direction = -1.0f;
            }
        }
        
        ImGui_Checkbox("Run", (bool*)&s_animation_tool.run);
        ImGui_Checkbox("Constraints", (bool*)&s_animation_tool.draw_constraints);
    }
    ImGui_End();
}