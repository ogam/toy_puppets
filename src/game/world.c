#include "game/world.h"

void init_world()
{
    World* world = &s_app->world;
    world->ecs = ecs_new(ECS_ENTITY_COUNT, NULL);
    
    init_ecs();
    init_conditions();
    cf_array_fit(world->floating_texts, 8);
    
    world->rnd = cf_rnd_seed((u64)0xa832fc9b);
    world->name_rnd = cf_rnd_seed((u64)0x9df7212d);
    
    for (s32 index = 0; index < CF_ARRAY_SIZE(world->damage_arenas); ++index)
    {
        world->damage_arenas[index] = cf_make_arena(16, MB(1));
    }
    
    inventory_reset(&world->inventory);
    init_overworld();
    
    {
        CF_Aabb bounds = cf_make_aabb_center_half_extents(cf_v2(0, 0), cf_v2(GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f));
        spatial_map_init(&world->spatial_map, bounds, 8, 8);
    }
}

void update_world()
{
    World* world = &s_app->world;
    
    world->dt = CF_DELTA_TIME;
    world->condition_dt = CF_DELTA_TIME;
    if (world->state != World_State_Arena_In_Progress)
    {
        world->condition_dt = 0;
    }
    
    {
        world->damage_arena_index = (world->damage_arena_index + 1) % CF_ARRAY_SIZE(world->damage_arenas);
        cf_arena_reset(world->damage_arenas + world->damage_arena_index);
    }
    
    PERF(system_handle_events) 
    {
        ECS_RUN_SYSTEM(system_handle_events);
    }
    
    switch (world->state)
    {
        case World_State_Overworld:
        {
            PERF(update_overworld) 
            {
                update_overworld();
            }
            break;
        }
        case World_State_Arena_Placement:
        case World_State_Arena_In_Progress:
        case World_State_Arena_End:
        {
            PERF(world_arena_update) 
            {
                world_arena_update();
            }
            break;
        }
    }
}

void draw_world()
{
    World* world = &s_app->world;
    
    // background
    {
        CF_Sprite background = assets_get_sprite("sprites/bg_0.png");
        
        world_push_background_camera();
        cf_draw_sprite(&background);
        world_pop_camera();
    }
    
    switch (world->state)
    {
        case World_State_Overworld:
        {
            draw_overworld();
            break;
        }
        case World_State_Arena_Placement:
        case World_State_Arena_In_Progress:
        case World_State_Arena_End:
        {
            world_arena_draw();
            break;
        }
    }
    cf_render_to(cf_app_get_canvas(), true);
}

void init_ecs()
{
    // components
    {
        ECS_REGISTER_COMP_CB(C_Puppet, NULL, comp_puppet_dtor);
        ECS_REGISTER_COMP_CB(C_Condition, comp_condition_ctor, comp_condition_dtor);
        ECS_REGISTER_COMP(C_AI);
        
        ECS_REGISTER_COMP(C_Human);
        ECS_REGISTER_COMP(C_Tentacle);
        ECS_REGISTER_COMP(C_Slime);
        
        ECS_REGISTER_COMP_CB(C_Team, NULL, comp_team_dtor);
        ECS_REGISTER_COMP_CB(C_Hurt_Box, comp_hurt_box_ctor, comp_hurt_box_dtor);
        
        ECS_REGISTER_COMP(C_Health);
        ECS_REGISTER_COMP(C_Creature);
        
        ECS_REGISTER_COMP(C_Hand_Caster);
        
        ECS_REGISTER_COMP(C_Movement);
        ECS_REGISTER_COMP(C_Creature_Normal);
        ECS_REGISTER_COMP(C_Creature_Elite);
        ECS_REGISTER_COMP(C_Creature_Boss);
        
        ECS_REGISTER_COMP(C_Event);
    }
    
    // system updates
    {
        ECS_REGISTER_SYSTEM(system_handle_events);
        ECS_REQUIRE_COMP(system_handle_events, C_Event);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_conditions);
        ECS_REQUIRE_COMP(system_update_conditions, C_Condition);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_healths);
        ECS_REQUIRE_COMP(system_update_healths, C_Health);
        ECS_REQUIRE_COMP(system_update_healths, C_Creature);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_creature_teams);
        ECS_REQUIRE_COMP(system_update_creature_teams, C_Team);
        ECS_REQUIRE_COMP(system_update_creature_teams, C_Puppet);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_puppet_animations);
        ECS_REQUIRE_COMP(system_update_puppet_animations, C_Puppet);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_prebuild_spatial_grid);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_build_spatial_grid);
        ECS_REQUIRE_COMP(system_update_build_spatial_grid, C_Puppet);
        ECS_REQUIRE_COMP(system_update_build_spatial_grid, C_Team);
        ECS_REQUIRE_COMP(system_update_build_spatial_grid, C_AI);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_hurt_boxes);
        ECS_REQUIRE_COMP(system_update_hurt_boxes, C_Puppet);
        ECS_REQUIRE_COMP(system_update_hurt_boxes, C_Team);
        ECS_REQUIRE_COMP(system_update_hurt_boxes, C_Hurt_Box);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_ai_humans);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_Puppet);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_AI);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_Team);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_Human);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_Hurt_Box);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_Creature);
        ECS_REQUIRE_COMP(system_update_ai_humans, C_Movement);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_ai_tentacles);
        ECS_REQUIRE_COMP(system_update_ai_tentacles, C_Puppet);
        ECS_REQUIRE_COMP(system_update_ai_tentacles, C_AI);
        ECS_REQUIRE_COMP(system_update_ai_tentacles, C_Team);
        ECS_REQUIRE_COMP(system_update_ai_tentacles, C_Tentacle);
        ECS_REQUIRE_COMP(system_update_ai_tentacles, C_Hurt_Box);
        ECS_REQUIRE_COMP(system_update_ai_tentacles, C_Creature);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_ai_slimes);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_Puppet);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_AI);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_Team);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_Slime);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_Hurt_Box);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_Creature);
        ECS_REQUIRE_COMP(system_update_ai_slimes, C_Movement);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_movements);
        ECS_REQUIRE_COMP(system_update_movements, C_Puppet);
        ECS_REQUIRE_COMP(system_update_movements, C_Creature);
        ECS_REQUIRE_COMP(system_update_movements, C_Movement);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_hand_caster);
        ECS_REQUIRE_COMP(system_update_hand_caster, C_Hand_Caster);
        ECS_REQUIRE_COMP(system_update_hand_caster, C_Team);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_event_cleanup_hand_caster);
        ECS_REQUIRE_COMP(system_event_cleanup_hand_caster, C_Hand_Caster);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_puppet_picking);
        ECS_REQUIRE_COMP(system_update_puppet_picking, C_Puppet);
        ECS_REQUIRE_COMP(system_update_puppet_picking, C_Creature);
        ECS_REQUIRE_COMP(system_update_puppet_picking, C_Team);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_floating_texts);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_update_cursor);
        ECS_REQUIRE_COMP(system_update_cursor, C_Hand_Caster);
    }
    
    // system draws
    {
        ECS_REGISTER_SYSTEM(system_draw_puppets);
        ECS_REQUIRE_COMP(system_draw_puppets, C_Puppet);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_draw_healthbars);
        ECS_REQUIRE_COMP(system_draw_healthbars, C_Puppet);
        ECS_REQUIRE_COMP(system_draw_healthbars, C_Creature);
        ECS_REQUIRE_COMP(system_draw_healthbars, C_Health);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_draw_hand_caster);
        ECS_REQUIRE_COMP(system_draw_hand_caster, C_Hand_Caster);
        ECS_REQUIRE_COMP(system_draw_hand_caster, C_Team);
    }
    
    {
        ECS_REGISTER_SYSTEM(system_draw_floating_texts);
    }
}

void world_push_background_camera()
{
    CF_V2 scale = cf_v2(GAME_WIDTH / BACKGROUND_WIDTH, GAME_HEIGHT / BACKGROUND_HEIGHT);
    
    cf_draw_push();
    cf_draw_scale_v2(scale);
}

void world_push_camera()
{
    cf_draw_push();
}

void world_pop_camera()
{
    cf_draw_pop();
}

void world_set_state(World_State state)
{
    World* world = &s_app->world;
    
    if (state == World_State_Overworld)
    {
        world_clear();
    }
    
    world->state = state;
}

void world_clear()
{
    ecs_reset(s_app->world.ecs);
}

void world_arena_update()
{
    World* world = &s_app->world;
    
    ECS_RUN_SYSTEM(system_update_conditions);
    ECS_RUN_SYSTEM(system_update_healths);
    ECS_RUN_SYSTEM(system_update_creature_teams);
    ECS_RUN_SYSTEM(system_update_puppet_animations);
    
    if (COROUTINE_IS_RUNNING(world->state_transition_co))
    {
        cf_coroutine_resume(world->state_transition_co);
    }
    else
    {
        switch (world->state)
        {
            case World_State_Arena_Placement:
            {
                world_arena_do_placement();
                break;
            }
            case World_State_Arena_In_Progress:
            {
                ECS_RUN_SYSTEM(system_update_prebuild_spatial_grid);
                ECS_RUN_SYSTEM(system_update_build_spatial_grid);
                ECS_RUN_SYSTEM(system_update_hurt_boxes);
                ECS_RUN_SYSTEM(system_update_ai_humans);
                ECS_RUN_SYSTEM(system_update_ai_tentacles);
                ECS_RUN_SYSTEM(system_update_ai_slimes);
                ECS_RUN_SYSTEM(system_update_hand_caster);
                
                u64* teams = cf_map_keys(world->team_creatures);
                s32 player_team_creature_count = 0;
                s32 other_team_creature_count = 0;
                for (s32 team_index = 0; team_index < cf_map_size(world->team_creatures); ++team_index)
                {
                    dyna ecs_entity_t* creatures = world->team_creatures[team_index];
                    
                    if (teams[team_index] == TOY_PLAYER_INDEX)
                    {
                        player_team_creature_count += cf_array_count(creatures);
                    }
                    else
                    {
                        other_team_creature_count += cf_array_count(creatures);
                    }
                }
                
                if (other_team_creature_count == 0)
                {
                    Overworld_Room* current_room = overworld_get_current_room();
                    inventory_add_gold(&world->inventory, current_room->gold);
                    world_set_state(World_State_Arena_End);
                }
                else if (player_team_creature_count == 0)
                {
                    world_set_state(World_State_Arena_End);
                }
                
                break;
            }
            case World_State_Arena_End:
            case World_State_Shop:
            default:
            {
                break;
            }
        }
    }
    
    ECS_RUN_SYSTEM(system_update_movements);
    ECS_RUN_SYSTEM(system_update_puppet_picking);
    ECS_RUN_SYSTEM(system_update_floating_texts);
    ECS_RUN_SYSTEM(system_update_cursor);
}

void world_arena_draw()
{
    Input* input = &s_app->input;
    World* world = &s_app->world;
    Inventory* inventory = &world->inventory;
    
    CF_V2 world_position = input->world_position;
    
    Body_Type selected_body_type = world->placement_type;
    
    world_push_camera();
    
    if (world->state == World_State_Arena_Placement)
    {
        CF_V2 center = world->placement_center;
        CF_V2 size = world->placement_size;
        
        cf_draw_box(world->walk_bounds, 1.0f, 0.0f);
        
        push_scissor(world->walk_bounds);
        draw_ellipse(center, size);
        
        // grid
        pop_scissor();
        CF_V2 min = world->walk_bounds.min;
        CF_V2 max = world->walk_bounds.max;
        
        cf_draw_push_color(cf_color_cyan());
        for (s32 y = 0; y < 20; ++y)
        {
            for (s32 x = 0; x < 20; ++x)
            {
                CF_V2 point = cf_v2(x * 0.05f, y * 0.05f);
                point.x = cf_lerp(min.x, max.x, point.x);
                point.y = cf_lerp(min.y, max.y, point.y);
                
                if (ellipse_contains(center, size, point))
                {
                    cf_draw_circle_fill2(point, 2.0f);
                }
            }
        }
        cf_draw_pop_color();
        
        // visual indicator 
        if (ellipse_contains(center, size, world_position) && cf_contains_point(world->walk_bounds, world_position))
        {
            cf_draw_push_layer(1);
            
            cf_push_font_size(48.0f);
            CF_Sprite place_sprite = assets_get_sprite("sprites/character_place.ase");
            CF_V2 sprite_position = world_position;
            sprite_position.y += place_sprite.h * 0.5f;
            place_sprite.transform.p = sprite_position;
            
            CF_V2 text_position = sprite_position;
            text_position.y += place_sprite.h * 0.5f;
            
            if (inventory_has_body(inventory, selected_body_type))
            {
                CF_V2 text_size = cf_text_size(Body_Type_names[selected_body_type], -1);
                text_position.x -= text_size.x * 0.5f;
                text_position.y += text_size.y;
                cf_draw_text(Body_Type_names[selected_body_type], text_position, -1);
            }
            else
            {
                char buffer[1024];
                CF_SNPRINTF(buffer, sizeof(buffer), "Cannot place %s!", Body_Type_names[selected_body_type]);
                
                CF_V2 text_size = cf_text_size(buffer, -1);
                text_position.x -= text_size.x * 0.5f;
                text_position.y += text_size.y;
                cf_draw_text(buffer, text_position, -1);
                
                place_sprite.blend_index = ASSETS_BLEND_LAYER_NEG;
            }
            cf_draw_sprite(&place_sprite);
            
            cf_pop_font_size();
            cf_draw_pop_layer();
        }
    }
    
    ECS_RUN_SYSTEM(system_draw_puppets);
    ECS_RUN_SYSTEM(system_draw_healthbars);
    if (world->state == World_State_Arena_In_Progress)
    {
        ECS_RUN_SYSTEM(system_draw_hand_caster);
    }
    ECS_RUN_SYSTEM(system_draw_floating_texts);
    
    world_pop_camera();
}

// small spawning animation
void world_arena_transition_to_in_progress_co(CF_Coroutine co)
{
    World* world = &s_app->world;
    
    CF_V2 bottom = world->placement_center;
    CF_V2 top = bottom;
    top.y = world->bounds.max.y + 100.0f;
    CF_V2 size = world->placement_size;
    
    f32 fall_duration = 1.0f;
    f32 duration = 2.0f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        CF_Color color = cf_color_white();
        
        if (delay > fall_duration)
        {
            f32 color_t = (delay - fall_duration) / (duration - fall_duration);
            color_t = cf_clamp01(color_t);
            color.a = cf_lerp(1.0f, 0.0f, color_t);
        }
        
        cf_draw_push_color(color);
        for (s32 ring_index = 0; ring_index < 50; ++ring_index)
        {
            f32 t = (delay - ring_index * 0.02f) / duration;
            t = cf_quint_in_out(t);
            t = cf_clamp01(t);
            
            CF_V2 center = cf_lerp(top, bottom, t);
            world_push_camera();
            cf_draw_push_layer(1);
            draw_ellipse(center, size);
            cf_draw_pop_layer();
            world_pop_camera();
        }
        cf_draw_pop_color();
        
        delay += CF_DELTA_TIME;
        cf_coroutine_yield(co);
    }
    
    world_set_state(World_State_Arena_In_Progress);
    
    duration = 0.25f;
    delay = 0.0f;
    while (delay < duration)
    {
        delay += CF_DELTA_TIME;
        cf_coroutine_yield(co);
    }
}

void world_arena_enter_placement(CF_Rnd* rnd, s32 enemy_count, s32 elite_count, s32 boss_count)
{
    UNUSED(elite_count);
    UNUSED(boss_count);
    
    World* world = &s_app->world;
    world_set_state(World_State_Arena_Placement);
    
    world_clear();
    world->placement_type = Body_Type_Human;
    
    //  @todo:  setup bounds based off of room
    {
        CF_V2 min = cf_v2(GAME_WIDTH * -0.5f, GAME_HEIGHT * -0.5f);
        CF_V2 max = cf_neg(min);
        
        CF_V2 walk_min = cf_v2(GAME_WIDTH * -0.4f, GAME_HEIGHT * -0.4f);
        CF_V2 walk_max = cf_v2(GAME_WIDTH * 0.4f, -0.1f);
        
        world->bounds = cf_make_aabb(min, max);
        world->walk_bounds = cf_make_aabb(walk_min, walk_max);
        
        world->placement_center = cf_center(world->walk_bounds);
        world->placement_center.y -= cf_extents(world->walk_bounds).y * 0.25f;
        world->placement_size = cf_v2(400.0f, 200.0f);
    }
    
    enum
    {
        Enemy_Type_Human,
        Enemy_Type_Slime,
        Enemy_Type_Tentacle,
    };
    
    // spawn normal
    for (s32 index = 0; index < enemy_count; ++index)
    {
        s32 type = cf_rnd_range_int(rnd, Enemy_Type_Human, Enemy_Type_Tentacle);
        ecs_entity_t entity;
        switch (type)
        {
            case Enemy_Type_Slime:
            {
                entity = make_slime(1, 1);
                break;
            }
            case Enemy_Type_Tentacle:
            {
                entity = make_tentacle(1, 1);
                break;
            }
            default:
            {
                entity = make_human(1, 1);
                break;
            }
        }
        
        CF_V2 position = cf_v2(0, 0);
        do
        {
            position = aabb_random_point(world->walk_bounds, rnd);
        } while (ellipse_contains(world->placement_center, world->placement_size, position));
        
        ECS_ADD(entity, C_Creature_Normal);
        
        set_team_base(entity, 2);
        set_team(entity, 2);
        teleport(entity, position);
        entity_apply_items(entity);
    }
    
    // spawn elites
    for (s32 index = 0; index < elite_count; ++index)
    {
        s32 type = cf_rnd_range_int(rnd, Enemy_Type_Human, Enemy_Type_Tentacle);
        ecs_entity_t entity;
        switch (type)
        {
            case Enemy_Type_Slime:
            {
                entity = make_slime(2, 1.5f);
                break;
            }
            case Enemy_Type_Tentacle:
            {
                entity = make_tentacle(2, 1.5f);
                break;
            }
            default:
            {
                entity = make_human(2, 1.5f);
                break;
            }
        }
        
        CF_V2 position = cf_v2(0, 0);
        do
        {
            position = aabb_random_point(world->walk_bounds, rnd);
        } while (ellipse_contains(world->placement_center, world->placement_size, position));
        
        ECS_ADD(entity, C_Creature_Elite);
        
        set_team_base(entity, 2);
        set_team(entity, 2);
        teleport(entity, position);
        entity_apply_items(entity);
    }
    
    // spawn bosses
    for (s32 index = 0; index < boss_count; ++index)
    {
        s32 type = cf_rnd_range_int(rnd, Enemy_Type_Human, Enemy_Type_Tentacle);
        ecs_entity_t entity;
        switch (type)
        {
            case Enemy_Type_Slime:
            {
                entity = make_slime(10, 2.0f);
                break;
            }
            case Enemy_Type_Tentacle:
            {
                entity = make_tentacle(10, 2.0f);
                break;
            }
            default:
            {
                entity = make_human(10, 2.0f);
                break;
            }
        }
        
        CF_V2 position = cf_v2(0, 0);
        do
        {
            position = aabb_random_point(world->walk_bounds, rnd);
        } while (ellipse_contains(world->placement_center, world->placement_size, position));
        
        ECS_ADD(entity, C_Creature_Boss);
        
        set_team_base(entity, 2);
        set_team(entity, 2);
        teleport(entity, position);
        entity_apply_items(entity);
    }
}

void world_arena_do_placement()
{
    Input* input = &s_app->input;
    World* world = &s_app->world;
    Inventory* inventory = &world->inventory;
    
    CF_V2 center = world->placement_center;
    CF_V2 size = world->placement_size;
    
    CF_V2 world_position = input->world_position;
    CF_Aabb walk_bounds = world->walk_bounds;
    
    Body_Type selected_body_type = world->placement_type;
    
    // place creatures
    if (ellipse_contains(center, size, world_position) && cf_contains_point(walk_bounds, world_position))
    {
        if (cf_button_binding_consume_press(input->place_creature))
        {
            if (inventory_remove_body(inventory, selected_body_type, 1))
            {
                ecs_entity_t entity = (ecs_entity_t){ -1 };
                
                switch (selected_body_type)
                {
                    case Body_Type_Human:
                    {
                        entity = make_human(1, 1);
                        break;
                    }
                    case Body_Type_Slime:
                    {
                        entity = make_slime(1, 1);
                        break;
                    }
                    case Body_Type_Tentacle:
                    {
                        entity = make_tentacle(1, 1);
                        break;
                    }
                }
                
                if (ENTITY_IS_VALID(entity))
                {
                    set_team_base(entity, TOY_PLAYER_INDEX);
                    set_team(entity, TOY_PLAYER_INDEX);
                    teleport(entity, world_position);
                    entity_apply_items(entity);
                }
            }
        }
    }
    
    // remove creatures
    if (cf_button_binding_consume_press(input->remove_creature))
    {
        if (cf_map_has(world->team_creatures, TOY_PLAYER_INDEX))
        {
            dyna ecs_entity_t* creatures = cf_map_get(world->team_creatures, TOY_PLAYER_INDEX);
            for (s32 index = 0; index < cf_array_count(creatures); ++index)
            {
                ecs_entity_t creature = creatures[index];
                C_Puppet* puppet = ECS_GET(creature, C_Puppet);
                CF_Aabb bounds = body_get_bounds(&puppet->body);
                
                if (cf_contains_point(bounds, world_position))
                {
                    inventory_add_body(inventory, puppet->body.type, 1);
                    ecs_destroy(world->ecs, creature);
                    break;
                }
            }
        }
    }
}

// comps
void comp_puppet_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr)
{
    C_Puppet* puppet = (C_Puppet*)comp_ptr;
    cf_map_free(puppet->proportions);
    destroy_body(&puppet->body);
}

void comp_condition_ctor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr, void* args)
{
    C_Condition* condition = (C_Condition*)comp_ptr;
    condition->handle = make_condition();
}

void comp_condition_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr)
{
    C_Condition* condition = (C_Condition*)comp_ptr;
    destroy_condition(condition->handle);
}

void comp_team_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr)
{
    World* world = &s_app->world;
    C_Team* team = (C_Team*)comp_ptr;
    
    team_remove_creature(entity, team->current);
}

void comp_creature_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr)
{
    C_Creature* creature = (C_Creature*)comp_ptr;
    cf_map_free(creature->attributes);
}

void comp_hurt_box_ctor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr, void* args)
{
    C_Hurt_Box* hurt_box = (C_Hurt_Box*)comp_ptr;
    cf_array_fit(hurt_box->hits, 128);
}

void comp_hurt_box_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr)
{
    C_Hurt_Box* hurt_box = (C_Hurt_Box*)comp_ptr;
    cf_array_free(hurt_box->hits);
}

// events handling
// everything here is only modifications for other systems to handle, so do not destroy
// any entities in any of these event_handle*() functions
void event_handle_condition_added(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type type)
{
    C_Puppet* puppet = ECS_GET(victim, C_Puppet);
    C_Creature* creature = ECS_GET(victim, C_Creature);
    C_Health* health = ECS_GET(victim, C_Health);
    Attributes attributes = get_condition_attributes(type);
    
    u64 modifier_mask = CONDITION_MASK | type;
    
    {
        Body_Proportions proportions = get_condition_body_proportions(type);
        Body_Proportions empty = { 0 };
        if (CF_MEMCMP(&proportions, &empty, sizeof(proportions)))
        {
            cf_map_set(puppet->proportions, modifier_mask, get_condition_body_proportions(type));
            body_scale(&puppet->body, accumulate_body_proportions(puppet->proportions));
        }
    }
    
    {
        Attributes empty = { 0 };
        if (CF_MEMCMP(&attributes, &empty, sizeof(attributes)))
        {
            cf_map_set(creature->attributes, modifier_mask, attributes);
        }
    }
    
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            floating_text_add(victim, Floating_Text_Type_Neutral, get_condition_name(type));
            floating_text_add(victim, Floating_Text_Type_Positive, get_condition_name(Condition_Effect_Type_Str_Up));
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            floating_text_add(victim, Floating_Text_Type_Neutral, get_condition_name(type));
            floating_text_add(victim, Floating_Text_Type_Positive, get_condition_name(Condition_Effect_Type_Str_Up));
            floating_text_add(victim, Floating_Text_Type_Negative, get_condition_name(Condition_Effect_Type_Agi_Down));
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            floating_text_add(victim, Floating_Text_Type_Neutral, get_condition_name(type));
            floating_text_add(victim, Floating_Text_Type_Positive, get_condition_name(Condition_Effect_Type_Agi_Up));
            floating_text_add(victim, Floating_Text_Type_Negative, get_condition_name(Condition_Effect_Type_Str_Down));
            break;
        }
        case Condition_Effect_Type_Death:
        {
            *health = 0;
            
            floating_text_add(victim, Floating_Text_Type_Neutral, get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            *health += attributes.health;
            
            const char* tagged_victim = get_color_tagged_name(victim);
            game_ui_log_condition_tick(tagged_victim, type, -attributes.health);
            
            floating_text_add(victim, Floating_Text_Type_Neutral, get_condition_name(type));
            floating_text_add_fmt(victim, Floating_Text_Type_Positive, "+%d", attributes.health);
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            floating_text_add(victim, Floating_Text_Type_Positive, get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            floating_text_add(victim, Floating_Text_Type_Positive, get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            Attributes accumulated_attributes = accumulate_attributes(creature->attributes);
            if (*health > accumulated_attributes.health)
            {
                s32 delta = *health - accumulated_attributes.health;
                const char* tagged_victim = get_color_tagged_name(victim);
                game_ui_log_condition_tick(tagged_victim, type, delta);
            }
            
            floating_text_add(victim, Floating_Text_Type_Negative, get_condition_name(type));
            floating_text_add_fmt(victim, Floating_Text_Type_Negative, "-%d", attributes.health);
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            floating_text_add(victim, Floating_Text_Type_Negative, get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            floating_text_add(victim, Floating_Text_Type_Negative, get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_Charmed:
        {
            u64 conversion_team = TOY_PLAYER_INDEX;
            if (ENTITY_IS_VALID(attacker))
            {
                C_Team* attacker_team = ECS_GET(attacker, C_Team);
                conversion_team = attacker_team->current;
            }
            set_team(victim, conversion_team);
            
            floating_text_add(victim, Floating_Text_Type_Negative, get_condition_name(type));
            break;
        }
        default:
        {
            floating_text_add(victim, Floating_Text_Type_Neutral, get_condition_name(type));
            break;
        }
    }
}

void event_handle_condition_removed(ecs_entity_t entity, Condition_Effect_Type type)
{
    C_Puppet* puppet = ECS_GET(entity, C_Puppet);
    C_Creature* creature = ECS_GET(entity, C_Creature);
    C_Health* health = ECS_GET(entity, C_Health);
    Attributes attributes = get_condition_attributes(type);
    
    u64 modifier_mask = CONDITION_MASK | type;
    
    if (cf_map_has(puppet->proportions, modifier_mask))
    {
        cf_map_del(puppet->proportions, modifier_mask);
        body_scale(&puppet->body, accumulate_body_proportions(puppet->proportions));
    }
    if (cf_map_has(creature->attributes, modifier_mask))
    {
        cf_map_del(creature->attributes, modifier_mask);
    }
    
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(Condition_Effect_Type_Str_Up));
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(Condition_Effect_Type_Str_Up));
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(Condition_Effect_Type_Agi_Down));
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(Condition_Effect_Type_Agi_Up));
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(Condition_Effect_Type_Str_Down));
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            floating_text_add_fmt(entity, Floating_Text_Type_Negative, "-%d", attributes.health);
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            floating_text_add_fmt(entity, Floating_Text_Type_Positive, "+%d", attributes.health);
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            break;
        }
        case Condition_Effect_Type_Charmed:
        {
            C_Team* team = ECS_GET(entity, C_Team);
            team->next = team->base;
            
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            break;
        }
        default:
        {
            floating_text_add_fmt(entity, Floating_Text_Type_Neutral, "<strike>%s</strike>", get_condition_name(type));
            break;
        }
    }
}

s32 event_handle_condition_tick(ecs_entity_t entity, Condition_Effect_Type type, s32 tick_count)
{
    s32 damage = get_condition_tick_damage(type, tick_count);
    if (damage != 0)
    {
        floating_text_add_damage(entity, damage);
        C_Health* health = ECS_GET(entity, C_Health);
        *health -= damage;
    }
    
    return damage;
}

void event_handle_hand_caster_select_spell(Spell_Type spell_type)
{
    World* world = &s_app->world;
    ECS_RUN_SYSTEM(system_event_cleanup_hand_caster);
    
    ecs_entity_t entity = ecs_create(world->ecs);
    C_Team* team = ECS_ADD(entity, C_Team);
    C_Hand_Caster* hand_caster = ECS_ADD(entity, C_Hand_Caster);
    
    team->current = TOY_PLAYER_INDEX;
    team->next = TOY_PLAYER_INDEX;
    
    Spell_Data spell = get_spell_data(spell_type);
    
    hand_caster->spell_type = spell_type;
    hand_caster->size = spell.size;
    hand_caster->target_type = spell.target_type;
}

// system updates
ecs_ret_t system_handle_events(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    Inventory* inventory = &world->inventory;
    
    ecs_comp_t comp_event = ECS_GET_COMP(C_Event);
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        C_Event* event = ecs_get(ecs, entities[index], comp_event);
        
        switch (event->type)
        {
            case Event_Type_Condition_Gained:
            {
                if (ENTITY_IS_VALID(event->condition.victim))
                {
                    event_handle_condition_added(event->condition.attacker, event->condition.victim, event->condition.type);
                    
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_condition_gained(tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Condition_Loss:
            {
                if (ENTITY_IS_VALID(event->condition.victim))
                {
                    event_handle_condition_removed(event->condition.victim, event->condition.type);
                    
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_condition_loss(tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Condition_Expired:
            {
                if (ENTITY_IS_VALID(event->condition.victim))
                {
                    event_handle_condition_removed(event->condition.victim, event->condition.type);
                    
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_condition_expired(tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Condition_Tick:
            {
                if (ENTITY_IS_VALID(event->condition.victim))
                {
                    s32 damage = event_handle_condition_tick(event->condition.victim, event->condition.type, event->condition.tick_count);
                    
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_condition_tick(tagged_victim, event->condition.type, damage);
                }
                break;
            }
            case Event_Type_Condition_Inflict:
            {
                if (ENTITY_IS_VALID(event->condition.attacker) && ENTITY_IS_VALID(event->condition.victim))
                {
                    event_handle_condition_added(event->condition.attacker, event->condition.victim, event->condition.type);
                    
                    const char* tagged_attacker = get_color_tagged_name(event->condition.attacker);
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_condition_inflict(tagged_attacker, tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Condition_Purge:
            {
                if (ENTITY_IS_VALID(event->condition.attacker) && ENTITY_IS_VALID(event->condition.victim))
                {
                    event_handle_condition_removed(event->condition.victim, event->condition.type);
                    
                    const char* tagged_attacker = get_color_tagged_name(event->condition.attacker);
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_condition_purge(tagged_attacker, tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Damage:
            {
                if (ENTITY_IS_VALID(event->hit.victim))
                {
                    s32 damage = accumulate_damage_sources(event->hit.sources);
                    floating_text_add_damage(event->hit.victim, damage);
                    C_Health* health = ECS_GET(event->hit.victim, C_Health);
                    *health -= damage;
                    
                    const char* tagged_attacker = get_color_tagged_name(event->hit.attacker);
                    const char* tagged_victim = get_color_tagged_name(event->hit.victim);
                    
                    game_ui_log_damage(tagged_attacker, tagged_victim, event->hit.sources);
                }
                break;
            }
            case Event_Type_Hit:
            {
                audio_play_sfx_rand_pitch("sounds/hit.wav");
                break;
            }
            case Event_Type_Died:
            {
                const char* tagged_victim = get_color_tagged_name_ex(event->died.name, event->died.team);
                game_ui_log_died(tagged_victim);
                break;
            }
            case Event_Type_Hand_Caster_Select_Spell:
            {
                game_ui_log_hand_select_spell(event->hand_caster_spell.spell_type);
                event_handle_hand_caster_select_spell(event->hand_caster_spell.spell_type);
                break;
            }
            case Event_Type_Hand_Caster_Cast_Spell:
            {
                game_ui_log_hand_cast_spell(event->hand_caster_spell.spell_type);
                const char* sound_name = get_spell_sound_name(event->hand_caster_spell.spell_type);
                audio_play_sfx_rand_pitch(sound_name);
                break;
            }
            case Event_Type_Hand_Caster_Cast_Spell_Missed:
            {
                game_ui_log_hand_cast_spell(event->hand_caster_spell.spell_type);
                audio_play_sfx_rand_pitch("sounds/spell_missed.wav");
                break;
            }
            case Event_Type_Hand_Caster_Condition_Add:
            {
                if (ENTITY_IS_VALID(event->hit.victim))
                {
                    event_handle_condition_added(event->condition.attacker, event->condition.victim, event->condition.type);
                    
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_hand_condition_add(tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Hand_Caster_Condition_Remove:
            {
                if (ENTITY_IS_VALID(event->hit.victim))
                {
                    event_handle_condition_removed(event->condition.victim, event->condition.type);
                    
                    const char* tagged_victim = get_color_tagged_name(event->condition.victim);
                    
                    game_ui_log_hand_condition_remove(tagged_victim, event->condition.type);
                }
                break;
            }
            case Event_Type_Hand_Caster_Damage:
            {
                if (ENTITY_IS_VALID(event->hand_caster_spell.entity))
                {
                    ecs_entity_t entity = event->hand_caster_spell.entity;
                    s32 damage = event->hand_caster_spell.damage;
                    Spell_Type spell_type = event->hand_caster_spell.spell_type;
                    
                    floating_text_add_damage(entity, damage);
                    C_Health* health = ECS_GET(entity, C_Health);
                    *health -= damage;
                    
                    const char* tagged_victim = get_color_tagged_name(entity);
                    
                    game_ui_log_hand_damage(tagged_victim, spell_type, damage);
                }
                break;
            }
            case Event_Type_Hand_Slap_Hit:
            {
                //  @todo:  event->slap_hit.slap_region
                ecs_entity_t victim = event->slap_hit.victim;
                
                audio_play_sfx_rand_pitch("sounds/slap_hit.wav");
                const char* tagged_victim = get_color_tagged_name(victim);
                game_ui_log_hand_slap(tagged_victim);
                break;
            }
            case Event_Type_World_Arena_Start:
            {
                if (world->state == World_State_Arena_Placement)
                {
                    if (COROUTINE_IS_DONE(world->state_transition_co))
                    {
                        cf_destroy_coroutine(world->state_transition_co);
                        world->state_transition_co = cf_make_coroutine(world_arena_transition_to_in_progress_co, 0, NULL);
                    }
                }
                break;
            }
        }
        
        ecs_destroy(ecs, entities[index]);
    }
    
    return 0;
}

ecs_ret_t system_update_conditions(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    ecs_comp_t comp_condition = ECS_GET_COMP(C_Condition);
    
    f32 dt = s_app->world.condition_dt;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Condition* condition = ecs_get(ecs, entity, comp_condition);
        
        dyna Condition_Effect* effects = get_condition_effects(condition->handle);
        for (s32 index = cf_array_count(effects) - 1; index >= 0; --index)
        {
            Condition_Effect* effect = effects + index;
            if (effect->duration_type == Condition_Duration_Type_Finite)
            {
                f32 prev_duration = effect->duration;
                effect->duration = cf_max(effect->duration - dt, 0.0f);
                
                if (effect->tick_rate > 0)
                {
                    s32 prev_ticks = (s32)(prev_duration / effect->tick_rate);
                    s32 ticks = (s32)(effect->duration / effect->tick_rate);
                    s32 tick_count = prev_ticks - ticks;
                    if (tick_count > 0)
                    {
                        make_event_condition_tick(entity, effect->type, tick_count);
                    }
                }
                
                if (effect->duration <= 0.0f)
                {
                    make_event_condition_expired(entity, effect->type);
                    cf_array_del(effects, index);
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_healths(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    ecs_comp_t comp_health = ECS_GET_COMP(C_Health);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    ecs_comp_t comp_creature_normal = ECS_GET_COMP(C_Creature_Normal);
    ecs_comp_t comp_creature_elite = ECS_GET_COMP(C_Creature_Elite);
    ecs_comp_t comp_creature_boss = ECS_GET_COMP(C_Creature_Boss);
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Health* health = ecs_get(ecs, entity, comp_health);
        C_Creature* creature = ecs_get(ecs, entity, comp_creature);
        Attributes attributes = accumulate_attributes(creature->attributes);
        
        *health = cf_min(*health, attributes.health);
        
        if (*health <= 0)
        {
            u64 team = 0;
            if (ecs_has(ecs, entity, comp_team))
            {
                C_Team* c_team = ecs_get(ecs, entity, comp_team);
                team = c_team->current;
            }
            
            if (team != TOY_PLAYER_INDEX)
            {
                b32 is_normal = ecs_has(ecs, entity, comp_creature_normal);
                b32 is_elite = ecs_has(ecs, entity, comp_creature_elite);
                b32 is_boss = ecs_has(ecs, entity, comp_creature_boss);
                
                overworld_current_room_enemy_died(is_normal, is_elite, is_boss);
            }
            
            make_event_died(entity, creature->name, team);
            ecs_destroy(ecs, entity);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_creature_teams(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        
        if (team->next != team->current)
        {
            team_remove_creature(entity, team->current);
            team_add_creature(entity, team->next);
            team->current = team->next;
        }
        
        if (team->current == 1)
        {
            if (world->state == World_State_Arena_Placement)
            {
                puppet->color.a = 0.15f;
            }
            else
            {
                puppet->color.a = 1.0f;
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_puppet_animations(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    
    f32 dt = world->dt;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        Body* body = &puppet->body;
        
        CF_Aabb local_bounds = world->bounds;
        local_bounds.min.y = cf_max(local_bounds.min.y, body->position.y);
        
        body_stabilize(body);
        body_verlet(body);
        body_satisfy_constraints(body, local_bounds);
        body_acceleration_reset(body);
    }
    
    return 0;
}

ecs_ret_t system_update_prebuild_spatial_grid(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    spatial_map_clear(&s_app->world.spatial_map);
    
    return 0;
}

ecs_ret_t system_update_build_spatial_grid(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    ecs_comp_t comp_ai = ECS_GET_COMP(C_AI);
    
    UNUSED(comp_ai);
    
    Spatial_Map* spatial_map = &s_app->world.spatial_map;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        
        spatial_map_push(spatial_map, team->current, entity, body_get_bounds(&puppet->body));
    }
    
    if (BIT_IS_SET(s_app->debug_state, Debug_State_Body_Bounds))
    {
        if (cf_map_size(spatial_map->grids))
        {
            cf_draw_push_layer(2);
            CF_Aabb bounds = cf_make_aabb_center_half_extents(s_app->input.world_position, cf_v2(10, 10));
            fixed ecs_entity_t* queries = spatial_map_query(spatial_map, ~(0), bounds);
            
            cf_draw_push_color(cf_color_magenta());
            cf_draw_box(bounds, 1.0f, 0.0f);
            
            for (s32 index = 0; index < cf_array_count(queries); ++index)
            {
                C_Puppet* puppet = ecs_get(ecs, queries[index], comp_puppet);
                CF_Aabb body_bounds = body_get_bounds(&puppet->body);
                cf_draw_box(body_bounds, 1.0f, 0.0f);
            }
            cf_draw_pop_color();
            
            Spatial_Grid grid = spatial_map->grids[0];
            Spatial_Grid_Indices indices = spatial_grid_get_indices(grid, bounds);
            
            cf_draw_push_color(cf_color_green());
            for (s32 y = indices.min_y; y <= indices.max_y; ++y)
            {
                for (s32 x = indices.min_x; x <= indices.max_x; ++x)
                {
                    s32 index = x + y * grid.partition_w;
                    CF_Aabb section = grid.partitions[index].bounds;
                    cf_draw_box(section, 1.0f, 0.0f);
                }
            }
            cf_draw_pop_color();
            cf_draw_pop_layer();
        }
    }
    
    return 0;
}

ecs_ret_t system_update_hurt_boxes(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    Inventory* inventory = &world->inventory;
    Spatial_Map* spatial_map = &world->spatial_map;
    CF_Arena* arena = world->damage_arenas + world->damage_arena_index;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    ecs_comp_t comp_hurt_box = ECS_GET_COMP(C_Hurt_Box);
    
    ecs_comp_t comp_condition = ECS_GET_COMP(C_Condition);
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        C_Hurt_Box* hurt_box = ecs_get(ecs, entity, comp_hurt_box);
        Body* body = &puppet->body;
        
        CF_Aabb bounds = body_get_particle_bounds(body, body->hurt_particles, cf_array_count(body->hurt_particles));
        fixed ecs_entity_t * queries = spatial_map_query(spatial_map, ~BIT(team->current), bounds);
        
        if (cf_array_count(body->hurt_particles))
        {
            if (cf_array_count(queries))
            {
                // setup damage sources
                fixed Condition_Effect_Type* added_effects = NULL;
                fixed Damage_Source* sources = NULL;
                MAKE_ARENA_ARRAY(arena, sources, 16);
                MAKE_SCRATCH_ARRAY(added_effects, 16);
                {
                    Damage_Source source = { 0 };
                    source.count = 1;
                    source.damage = hurt_box->damage;
                    cf_array_push(sources, source);
                }
                
                // condition added damages
                {
                    if (ecs_has(ecs, entity, comp_condition))
                    {
                        C_Condition* condition = ecs_get(ecs, entity, comp_condition);
                        dyna Condition_Effect* effects = get_condition_effects(condition->handle);
                        
                        for (s32 effect_index = 0; effect_index < cf_array_count(effects); ++effect_index)
                        {
                            Condition_Effect* effect = effects + effect_index;
                            s32 damage = get_condition_added_damage(effect->type);
                            if (damage != 0)
                            {
                                Damage_Source source = { 0 };
                                source.count = 1;
                                source.damage = damage;
                                source.effect = effect->type;
                                cf_array_push(sources, source);
                            }
                            
                            Condition_Effect_Type added_effect = get_condition_added_effect(effect->type);
                            if (added_effect != Condition_Effect_Type_None)
                            {
                                cf_array_push(added_effects, added_effect);
                            }
                        }
                    }
                }
                
                // items
                if (inventory->items)
                {
                    for (s32 index = 0; index < cf_array_count(inventory->items); ++index)
                    {
                        if (inventory->items[index] <= 0)
                        {
                            continue;
                        }
                        
                        Item_Type item_type = (Item_Type)index;
                        if (BIT_IS_SET(get_item_team_mask(item_type), team->base))
                        {
                            s32 damage = get_item_added_damage(item_type);
                            if (damage)
                            {
                                Damage_Source source = { 0 };
                                source.count = inventory->items[item_type];
                                source.damage = damage;
                                source.item = item_type;
                                cf_array_push(sources, source);
                            }
                            
                            Condition_Effect_Type added_effect = get_item_added_effect(item_type);
                            if (added_effect != Condition_Effect_Type_None)
                            {
                                cf_array_push(added_effects, added_effect);
                            }
                        }
                    }
                }
                
                // actual hits
                for (s32 query_index = 0; query_index < cf_array_count(queries); ++query_index)
                {
                    if (array_add_unique(hurt_box->hits, queries[query_index]))
                    {
                        ecs_entity_t victim = queries[query_index];
                        make_event_damage(entity, victim, sources);
                        make_event_hit(entity, victim);
                        
                        for (s32 effect_index = 0; effect_index < cf_array_count(added_effects); ++effect_index)
                        {
                            attacker_add_condition(entity, victim, added_effects[effect_index]);
                        }
                    }
                }
            }
        }
        else
        {
            cf_array_clear(hurt_box->hits);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_ai_humans(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_ai = ECS_GET_COMP(C_AI);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    ecs_comp_t comp_human = ECS_GET_COMP(C_Human);
    ecs_comp_t comp_hurt_box = ECS_GET_COMP(C_Hurt_Box);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    ecs_comp_t comp_movement = ECS_GET_COMP(C_Movement);
    
    UNUSED(comp_human);
    
    f32 dt = world->dt;
    CF_Rnd* rnd = &world->rnd;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_AI* ai = ecs_get(ecs, entity, comp_ai);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        C_Hurt_Box* hurt_box = ecs_get(ecs, entity, comp_hurt_box);
        C_Creature* creature = ecs_get(ecs, entity, comp_creature);
        C_Movement* movement = ecs_get(ecs, entity, comp_movement);
        
        Body* body = &puppet->body;
        b32 is_in_action = COROUTINE_IS_RUNNING(puppet->action_co);
        movement->direction = cf_v2(0, 0);
        
        if (is_in_action)
        {
            ai->attack_delay = agility_to_attack_rate(creature);
            cf_coroutine_resume(puppet->action_co);
            continue;
        }
        
        // check if head is stuck in torso
        {
            CF_V2 head_direction = cf_sub(body->particles[Body_Human_Head], body->particles[Body_Human_Neck]);
            CF_V2 torso_direction = cf_sub(body->particles[Body_Human_Left_Shoulder], body->particles[Body_Human_Left_Hip]);
            
            if (cf_dot(head_direction, torso_direction) < 0.0f)
            {
                cf_destroy_coroutine(puppet->action_co);
                puppet->action_co = cf_make_coroutine(body_human_fix_head_co, 0, body);
                continue;
            }
        }
        
        ai->attack_delay -= dt;
        Body* target_body = NULL;
        
        if (ENTITY_IS_VALID(ai->target) && !is_entity_friendly(entity, ai->target))
        {
            C_Puppet* target_puppet = ecs_get(ecs, ai->target, comp_puppet);
            target_body = &target_puppet->body;
        }
        else
        {
            ai->target = find_nearest_creature(body->position, ~(team->current));
            continue;
        }
        
        CF_V2 move_direction = walk_direction_to_attack(body, target_body);
        
        if (cf_len_sq(move_direction) == 0.0f)
        {
            body->facing_direction = target_body->position.x - body->position.x < 0 ? -1.0f : 1.0f;
            if (ai->attack_delay <= 0.0f)
            {
                b32 do_kick = (target_body->height < body->height * 0.6f) || cf_rnd_float(rnd) < 0.5f;
                CF_CoroutineFn* action_co = NULL;
                cf_destroy_coroutine(puppet->action_co);
                
                hurt_box->damage = strength_to_damage(creature);
                
                if (do_kick)
                {
                    action_co = body_human_kick_co;
                }
                else
                {
                    action_co = body_human_punch_co;
                }
                
                puppet->action_co = cf_make_coroutine(action_co, 0, body);
                cf_coroutine_push(puppet->action_co, target_body, sizeof(*target_body));
            }
            else
            {
                // close enough to attack but waiting for next attack interval
                body_human_idle(body, puppet->stride, puppet->spin_speed);
            }
        }
        else
        {
            movement->direction = move_direction;
        }
        
        body_human_hand_guard(body);
    }
    
    return 0;
}

ecs_ret_t system_update_ai_tentacles(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_ai = ECS_GET_COMP(C_AI);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    ecs_comp_t comp_tentacle = ECS_GET_COMP(C_Tentacle);
    ecs_comp_t comp_hurt_box = ECS_GET_COMP(C_Hurt_Box);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    
    UNUSED(comp_tentacle);
    
    f32 dt = world->dt;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_AI* ai = ecs_get(ecs, entity, comp_ai);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        C_Hurt_Box* hurt_box = ecs_get(ecs, entity, comp_hurt_box);
        C_Creature* creature = ecs_get(ecs, entity, comp_creature);
        Body* body = &puppet->body;
        
        if (COROUTINE_IS_RUNNING(puppet->action_co))
        {
            ai->attack_delay = agility_to_attack_rate(creature);
            cf_coroutine_resume(puppet->action_co);
            continue;
        }
        
        ai->attack_delay -= dt;
        f32 attack_range = body->height;
        
        if (ai->attack_delay <= 0.0f)
        {
            ecs_entity_t target = find_nearest_creature(body->position, ~(team->current));
            
            if (ENTITY_IS_VALID(target))
            {
                ai->target = target;
                C_Puppet* target_puppet = ecs_get(ecs, target, comp_puppet);
                CF_V2 dp = cf_sub(target_puppet->body.position, body->position);
                CF_V2 abs_dp = cf_abs(dp);
                f32 dx = abs_dp.x - cf_half_width(body_get_bounds(&target_puppet->body));
                
                if (abs_dp.y < body->height * 0.1f && dx < attack_range)
                {
                    body->facing_direction = dp.x < 0 ? -1.0f : 1.0f;
                    cf_destroy_coroutine(puppet->action_co);
                    puppet->action_co = cf_make_coroutine(body_tentacle_whip_co, 0, body);
                    hurt_box->damage = strength_to_damage(creature);
                }
            }
        }
        
        body_tentacle_idle(&puppet->body);
    }
    
    return 0;
}

ecs_ret_t system_update_ai_slimes(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_ai = ECS_GET_COMP(C_AI);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    ecs_comp_t comp_slime = ECS_GET_COMP(C_Slime);
    ecs_comp_t comp_hurt_box = ECS_GET_COMP(C_Hurt_Box);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    ecs_comp_t comp_movement = ECS_GET_COMP(C_Movement);
    
    UNUSED(comp_slime);
    
    f32 dt = world->dt;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_AI* ai = ecs_get(ecs, entity, comp_ai);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        C_Hurt_Box* hurt_box = ecs_get(ecs, entity, comp_hurt_box);
        C_Creature* creature = ecs_get(ecs, entity, comp_creature);
        C_Movement* movement = ecs_get(ecs, entity, comp_movement);
        Body* body = &puppet->body;
        
        movement->direction = cf_v2(0, 0);
        
        if (COROUTINE_IS_RUNNING(puppet->action_co))
        {
            ai->attack_delay = agility_to_attack_rate(creature);
            cf_coroutine_resume(puppet->action_co);
            continue;
        }
        
        ai->attack_delay -= dt;
        Body* target_body = NULL;
        
        if (ENTITY_IS_VALID(ai->target) && !is_entity_friendly(entity, ai->target))
        {
            C_Puppet* target_puppet = ecs_get(ecs, ai->target, comp_puppet);
            target_body = &target_puppet->body;
        }
        else
        {
            ai->target = find_nearest_creature(body->position, ~(team->current));
            continue;
        }
        
        CF_V2 move_direction = walk_direction_to_attack(body, target_body);
        
        s32 grounded_particles = body_particles_touching_ground(body);
        b32 is_grounded = grounded_particles >= 3;
        {
            f32 target_center_y = body->position.y + body->height * 0.3f;
            is_grounded = is_grounded || body->particles[Body_Slime_Center].y  < target_center_y;
        }
        
        if (is_grounded)
        {
            // once grounded stay on the ground, avoids the slime from flying all over the place
            // or tunneling into the floor
            body->is_locked[Body_Slime_Center] = true;
            
            if (cf_len_sq(move_direction) == 0.0f)
            {
                if (ai->attack_delay <= 0.0f)
                {
                    body->facing_direction = target_body->position.x - body->position.x < 0 ? -1.0f : 1.0f;
                    cf_destroy_coroutine(puppet->action_co);
                    puppet->action_co = cf_make_coroutine(body_slime_forward_spike_co, 0, body);
                    hurt_box->damage = strength_to_damage(creature);
                }
            }
            else
            {
                movement->direction = move_direction;
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_movements(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    ecs_comp_t comp_movement = ECS_GET_COMP(C_Movement);
    
    if (world->state == World_State_Arena_In_Progress)
    {
        for (size_t index = 0; index < entity_count; ++index)
        {
            ecs_entity_t entity = entities[index];
            C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
            C_Creature* creature = ecs_get(ecs, entity, comp_creature);
            C_Movement* movement = ecs_get(ecs, entity, comp_movement);
            
            Attributes attributes = accumulate_attributes(creature->attributes);
            movement->speed = attributes.move_speed;
            
            Body* body = &puppet->body;
            
            switch (body->type)
            {
                case Body_Type_Human:
                {
                    if (COROUTINE_IS_DONE(puppet->action_co))
                    {
                        body_human_walk(body, movement->direction, puppet->stride, movement->speed, puppet->spin_speed);
                    }
                    break;
                }
                case Body_Type_Slime:
                {
                    body_slime_walk(body, movement->direction, movement->speed);
                    break;
                }
            }
        }
    }
    else
    {
        for (size_t index = 0; index < entity_count; ++index)
        {
            ecs_entity_t entity = entities[index];
            C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
            C_Movement* movement = ecs_get(ecs, entity, comp_movement);
            Body* body = &puppet->body;
            
            CF_V2 direction = cf_v2(0, 0);
            
            switch (body->type)
            {
                case Body_Type_Human:
                {
                    body_human_walk(body, direction, puppet->stride, movement->speed, puppet->spin_speed);
                    break;
                }
                case Body_Type_Slime:
                {
                    body_slime_walk(body, direction, movement->speed);
                    break;
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_hand_caster(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    Inventory* inventory = &world->inventory;
    
    ecs_comp_t comp_hand_caster = ECS_GET_COMP(C_Hand_Caster);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    
    CF_V2 world_position = s_app->input.world_position;
    b32 cast_spell = cf_button_binding_pressed(s_app->input.cast_spell) && !ui_is_any_layout_hovered();
    b32 cancel_spell = cf_button_binding_pressed(s_app->input.cancel_spell) && !ui_is_any_layout_hovered();
    
    s_app->cursor.state = entity_count > 0 ? Cursor_State_Cast : Cursor_State_None;
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Hand_Caster* hand_caster = ecs_get(ecs, entity, comp_hand_caster);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        
        hand_caster->position = world_position;
        Spell_Data spell = get_spell_data(hand_caster->spell_type);
        
        if (cancel_spell)
        {
            ecs_destroy(ecs, entity);
            continue;
        }
        
        if (cast_spell && inventory_remove_spell(inventory, hand_caster->spell_type, 1))
        {
            CF_V2 center = hand_caster->position;
            CF_V2 size = hand_caster->size;
            
            u64 team_mask = ~0;
            if (hand_caster->target_type == Cast_Target_Type_Enemies)
            {
                BIT_UNSET(team_mask, team->current);
            }
            else if (hand_caster->target_type == Cast_Target_Type_Allies)
            {
                BIT_ASSIGN(team_mask, team->current);
            }
            
            s32 hit_count = 0;
            u64* teams = cf_map_keys(world->team_creatures);
            for (s32 team_index = 0; team_index < cf_map_size(world->team_creatures); ++team_index)
            {
                if (!BIT_IS_SET(team_mask, teams[team_index]))
                {
                    continue;
                }
                
                dyna ecs_entity_t* creatures = world->team_creatures[team_index];
                
                for (s32 creature_index = 0; creature_index < cf_array_count(creatures); ++creature_index)
                {
                    ecs_entity_t creature = creatures[creature_index];
                    C_Puppet* puppet = ecs_get(ecs, creature, comp_puppet);
                    if (ellipse_contains(center, size, puppet->body.position))
                    {
                        for (s32 effect_index = 0; effect_index < cf_array_count(spell.effects); ++effect_index)
                        {
                            hand_cast_add_condition(creature, spell.effects[effect_index]);
                        }
                        if (spell.damage)
                        {
                            make_event_hand_caster_damage(creature, hand_caster->spell_type, spell.damage);
                        }
                        
                        ++hit_count;
                    }
                }
            }
            
            if (hit_count)
            {
                make_event_hand_caster_cast_spell(hand_caster->spell_type);
            }
            else
            {
                make_event_hand_caster_cast_spell_missed(hand_caster->spell_type);
            }
            
            ecs_destroy(ecs, entity);
        }
    }
    
    return 0;
}

ecs_ret_t system_event_cleanup_hand_caster(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_hand_caster = ECS_GET_COMP(C_Hand_Caster);
    
    UNUSED(comp_hand_caster);
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        ecs_destroy(ecs, entity);
    }
    
    return 0;
}

ecs_ret_t system_update_puppet_picking(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    
    UNUSED(comp_creature);
    
    f32 dt = world->dt;
    CF_V2 world_position = s_app->input.world_position;
    
    ecs_entity_t next_hover_entity = (ecs_entity_t){ -1 };
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        Body* body = &puppet->body;
        
        CF_Aabb bounds = body_get_bounds(body);
        if (cf_contains_point(bounds, world_position))
        {
            next_hover_entity = entity;
        }
        
        CF_Color color = get_team_color(team->current);
        puppet->color.r = color.r;
        puppet->color.g = color.g;
        puppet->color.b = color.b;
    }
    
    if (ENTITY_IS_VALID(next_hover_entity) && 
        world->hover_entity.id == next_hover_entity.id)
    {
        world->hover_duration += dt;
    }
    else
    {
        world->hover_duration = 0.0f;
    }
    world->hover_entity = next_hover_entity;
    
    return 0;
}

ecs_ret_t system_update_floating_texts(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    dyna Floating_Text* floating_texts = world->floating_texts;
    f32 dt = CF_DELTA_TIME;
    CF_V2 gravity = cf_v2(0.0f, -1000.0f * dt);
    
    for (s32 index = cf_array_count(floating_texts) - 1; index >= 0; --index)
    {
        Floating_Text* text = floating_texts + index;
        text->time += dt;
        
        if (text->time > text->delay)
        {
            text->velocity = cf_add(text->velocity, gravity);
            CF_V2 offset = cf_mul_v2_f(text->velocity, dt);
            text->position = cf_add(text->position, offset);
            
            f32 t = (text->time - text->delay) / text->duration;
            t = cf_quint_in_out(t);
            t = cf_clamp01(t);
            text->color.a = 1.0f - t;
        }
        
        if (text->time > text->delay + text->duration)
        {
            cf_array_del(floating_texts, index);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_cursor(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    if (world->state == World_State_Arena_In_Progress && entity_count > 0)
    {
        s_app->cursor.state = Cursor_State_Cast;
    }
    else
    {
        s_app->cursor.state = Cursor_State_None;
    }
    return 0;
}

// system draws
ecs_ret_t system_draw_puppets(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    f32 bounds_max_y = world->bounds.max.y;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    
    if (!BIT_IS_SET(s_app->debug_state, Debug_State_Body_Bounds))
    {
        for (size_t index = 0; index < entity_count; ++index)
        {
            ecs_entity_t entity = entities[index];
            C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
            Body* body = &puppet->body;
            
            s32 draw_layer = (s32)(bounds_max_y - body->position.y);
            
            cf_draw_push_layer(draw_layer);
            cf_draw_push_color(puppet->color);
            body_draw(body, accumulate_body_proportions(puppet->proportions));
            cf_draw_pop_color();
            cf_draw_pop_layer();
        }
    }
    else
    {
        for (size_t index = 0; index < entity_count; ++index)
        {
            ecs_entity_t entity = entities[index];
            C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
            Body* body = &puppet->body;
            
            CF_Aabb body_bounds = body_get_bounds(body);
            cf_draw_box(body_bounds, 1.0f, 0.0f);
            body_draw_constraints(body, 1.0f);
            
            if (cf_array_count(body->hurt_particles))
            {
                cf_draw_push_color(cf_color_red());
                CF_Aabb hurt_bounds = body_get_particle_bounds(body, body->hurt_particles, cf_array_count(body->hurt_particles));
                cf_draw_box(hurt_bounds, 1.0f, 0.0f);
                cf_draw_pop_color();
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_draw_healthbars(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    f32 bounds_max_y = world->bounds.max.y;
    
    ecs_comp_t comp_puppet = ECS_GET_COMP(C_Puppet);
    ecs_comp_t comp_creature = ECS_GET_COMP(C_Creature);
    ecs_comp_t comp_health = ECS_GET_COMP(C_Health);
    
    CF_Color background_color = cf_color_grey();
    CF_Color border_color = cf_color_white();
    CF_Color inner_color = cf_color_red();
    CF_Color text_color = cf_color_white();
    
    f32 border_thickness = 1.0f;
    f32 corner_radius = 5.0f;
    
    f32 height = 16.0f;
    f32 width = 64.0f;
    
    char buffer[64];
    
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Puppet* puppet = ecs_get(ecs, entity, comp_puppet);
        C_Creature* creature = ecs_get(ecs, entity, comp_creature);
        C_Health* health = ecs_get(ecs, entity, comp_health);
        Body* body = &puppet->body;
        
        Attributes attributes = accumulate_attributes(creature->attributes);
        CF_SNPRINTF(buffer, sizeof(buffer), "%d / %d", *health, attributes.health);
        
        f32 t = (f32)*health / attributes.health;
        
        CF_V2 position = body->position;
        position.y -= height;
        
        s32 draw_layer = (s32)(bounds_max_y - body->position.y);
        cf_draw_push_layer(draw_layer);
        
        CF_V2 min = cf_sub(position, cf_v2(width * 0.5f, height));
        CF_V2 max = cf_add(min, cf_v2(width, height));
        
        CF_Aabb border = cf_make_aabb(min, max);
        CF_Aabb inner = border;
        inner.max.x = inner.min.x + width * t;
        
        cf_draw_push_color(background_color);
        cf_draw_box_fill(border, corner_radius);
        cf_draw_pop_color();
        
        cf_draw_push_color(inner_color);
        cf_draw_box_fill(inner, 0);
        cf_draw_pop_color();
        
        cf_draw_push_color(border_color);
        cf_draw_box(border, border_thickness, corner_radius);
        cf_draw_pop_color();
        
        cf_push_font_size(height);
        cf_draw_push_color(text_color);
        
        CF_V2 text_size = cf_text_size(buffer, -1);
        CF_V2 text_position = cf_v2(cf_top(border).x, cf_center(inner).y);
        text_position.x += text_size.x * -0.5f;
        text_position.y += text_size.y * 0.5f + border_thickness;
        cf_draw_text(buffer, text_position, -1);
        
        cf_draw_pop_color();
        cf_pop_font_size();
        
        cf_draw_pop_layer();
    }
    
    return 0;
}

ecs_ret_t system_draw_hand_caster(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    World* world = &s_app->world;
    
    ecs_comp_t comp_hand_caster = ECS_GET_COMP(C_Hand_Caster);
    ecs_comp_t comp_team = ECS_GET_COMP(C_Team);
    
    CF_V2 world_position = s_app->input.world_position;
    b32 cast_spell = cf_button_binding_pressed(s_app->input.cast_spell);
    
    f32 font_size = 48.0f;
    cf_push_font_size(font_size);
    for (size_t index = 0; index < entity_count; ++index)
    {
        ecs_entity_t entity = entities[index];
        C_Hand_Caster* hand_caster = ecs_get(ecs, entity, comp_hand_caster);
        C_Team* team = ecs_get(ecs, entity, comp_team);
        
        hand_caster->position = world_position;
        
        CF_V2 center = hand_caster->position;
        CF_V2 size = hand_caster->size;
        
        // draw affected area and pulse
        draw_ellipse(center, size);
        draw_ellipse(center, cf_lerp(cf_v2(0, 0), size, cf_cos((f32)CF_SECONDS * 3.0f)));
        
        // draw spell name
        {
            CF_V2 text_position = cf_sub(center, cf_v2(0, hand_caster->size.y));
            if (text_position.y < GAME_HEIGHT * -0.5f + font_size)
            {
                text_position.y = center.y + hand_caster->size.y + font_size;
            }
            const char* spell_name = get_spell_name(hand_caster->spell_type);
            CF_V2 text_size = cf_text_size(spell_name, -1);
            text_position.x -= text_size.x * 0.5f;
            cf_draw_push_color(cf_color_black());
            cf_draw_text(spell_name, cf_add(text_position, cf_v2(1, -1)), -1);
            cf_draw_pop_color();
            cf_draw_text(spell_name, text_position, -1);
        }
        
        // draw which creatures can be targeted
        //  @todo:  probably do an outline or something instead
        u64 team_mask = ~0;
        if (hand_caster->target_type == Cast_Target_Type_Enemies)
        {
            BIT_UNSET(team_mask, team->current);
        }
        else if (hand_caster->target_type == Cast_Target_Type_Allies)
        {
            BIT_ASSIGN(team_mask, team->current);
        }
        
        // draw which creatures are selected
        cf_draw_push_color(cf_color_green());
        u64* teams = cf_map_keys(world->team_creatures);
        for (s32 team_index = 0; team_index < cf_map_size(world->team_creatures); ++team_index)
        {
            if (!BIT_IS_SET(team_mask, teams[team_index]))
            {
                continue;
            }
            
            dyna ecs_entity_t* creatures = world->team_creatures[team_index];
            
            for (s32 creature_index = 0; creature_index < cf_array_count(creatures); ++creature_index)
            {
                C_Puppet* puppet = ECS_GET(creatures[creature_index], C_Puppet);
                CF_V2 position = puppet->body.position;
                
                if (ellipse_contains(center, size, position))
                {
                    cf_draw_circle_fill2(position, 10.0f);
                }
            }
        }
        cf_draw_pop_color();
    }
    cf_pop_font_size();
    
    return 0;
}

ecs_ret_t system_draw_floating_texts(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata)
{
    dyna Floating_Text* floating_texts = s_app->world.floating_texts;
    char buffer[1024];
    cf_push_font_size(36.0f);
    
    for (s32 index = 0; index < cf_array_count(floating_texts); ++index)
    {
        Floating_Text* text = floating_texts + index;
        if (text->time > text->delay)
        {
            CF_V2 position = text->position;
            cf_draw_push_color(text->color);
            
            if (text->is_damage)
            {
                CF_SNPRINTF(buffer, sizeof(buffer), "%d", text->damage);
                position.x -= cf_text_width(buffer, -1) * 0.5f;
                cf_draw_text(buffer, position, -1);
            }
            else
            {
                position.x -= cf_text_width(text->text, -1) * 0.5f;
                cf_draw_text(text->text, position, -1);
            }
            cf_draw_pop_color();
        }
    }
    
    cf_pop_font_size();
    
    return 0;
}


C_Event* make_event(Event_Type type)
{
    ecs_entity_t entity = ecs_create(s_app->world.ecs);
    C_Event* event = ECS_ADD(entity, C_Event);
    event->type = type;
    
    return event;
}

void make_event_condition_gained(ecs_entity_t entity, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Condition_Gained);
    event->condition.attacker = entity;
    event->condition.victim = entity;
    event->condition.type = effect_type;
}

void make_event_condition_loss(ecs_entity_t entity, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Condition_Loss);
    event->condition.attacker = entity;
    event->condition.victim = entity;
    event->condition.type = effect_type;
}

void make_event_condition_expired(ecs_entity_t entity, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Condition_Expired);
    event->condition.attacker = entity;
    event->condition.victim = entity;
    event->condition.type = effect_type;
}

void make_event_condition_tick(ecs_entity_t entity, Condition_Effect_Type effect_type, s32 tick_count)
{
    C_Event* event = make_event(Event_Type_Condition_Tick);
    event->condition.victim = entity;
    event->condition.type = effect_type;
    event->condition.tick_count = tick_count;
}

void make_event_condition_inflict(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Condition_Inflict);
    event->condition.attacker = attacker;
    event->condition.victim = victim;
    event->condition.type = effect_type;
}

void make_event_condition_purge(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Condition_Purge);
    event->condition.attacker = attacker;
    event->condition.victim = victim;
    event->condition.type = effect_type;
}

void make_event_damage(ecs_entity_t attacker, ecs_entity_t victim, fixed Damage_Source* sources)
{
    C_Event* event = make_event(Event_Type_Damage);
    event->hit.attacker = attacker;
    event->hit.victim = victim;
    event->hit.sources = sources;
}

void make_event_hit(ecs_entity_t attacker, ecs_entity_t victim)
{
    C_Event* event = make_event(Event_Type_Hit);
    event->hit.attacker = attacker;
    event->hit.victim = victim;
}

void make_event_died(ecs_entity_t victim, const char* name, u64 team)
{
    C_Event* event = make_event(Event_Type_Hit);
    event->died.victim = victim;
    event->died.name = name;
    event->died.team = team;
}

void make_event_hand_caster_select_spell(Spell_Type spell_type)
{
    C_Event* event = make_event(Event_Type_Hand_Caster_Select_Spell);
    event->hand_caster_spell.spell_type = spell_type;
}

void make_event_hand_caster_cast_spell(Spell_Type spell_type)
{
    C_Event* event = make_event(Event_Type_Hand_Caster_Cast_Spell);
    event->hand_caster_spell.spell_type = spell_type;
}

void make_event_hand_caster_cast_spell_missed(Spell_Type spell_type)
{
    C_Event* event = make_event(Event_Type_Hand_Caster_Cast_Spell_Missed);
    event->hand_caster_spell.spell_type = spell_type;
}

void make_event_hand_caster_condition_add(ecs_entity_t entity, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Hand_Caster_Condition_Add);
    event->condition.attacker = (ecs_entity_t){ -1 };
    event->condition.victim = entity;
    event->condition.type = effect_type;
}

void make_event_hand_caster_condition_remove(ecs_entity_t entity, Condition_Effect_Type effect_type)
{
    C_Event* event = make_event(Event_Type_Hand_Caster_Condition_Remove);
    event->condition.attacker = (ecs_entity_t){ -1 };
    event->condition.victim = entity;
    event->condition.type = effect_type;
}

void make_event_hand_caster_damage(ecs_entity_t entity, Spell_Type spell_type, s32 damage)
{
    C_Event* event = make_event(Event_Type_Hand_Caster_Damage);
    event->hand_caster_spell.entity = entity;
    event->hand_caster_spell.spell_type = spell_type;
    event->hand_caster_spell.damage = damage;
}

void make_event_hand_slap_hit(ecs_entity_t victim, CF_Aabb slap_region)
{
    Inventory* inventory = &s_app->world.inventory;
    
    C_Event* event = make_event(Event_Type_Hand_Slap_Hit);
    event->slap_hit.victim = victim;
    event->slap_hit.slap_region = slap_region;
    
    hand_cast_add_condition(victim, Condition_Effect_Type_Enrage);
    
    fixed Damage_Source* damage_sources = NULL;
    MAKE_SCRATCH_ARRAY(damage_sources, cf_array_count(inventory->items));
    
    for (s32 index = 0; index < cf_array_count(inventory->items); ++index)
    {
        if (inventory->items[index] <= 0)
        {
            continue;
        }
        
        Item_Type item_type = (Item_Type)index;
        s32 damage = get_item_slap_damage(item_type);
        Condition_Effect_Type effect = get_item_slap_condition_effect(item_type);
        
        if (damage)
        {
            Damage_Source damage_source = { 0 };
            damage_source.item = item_type;
            damage_source.count = inventory->items[item_type];
            damage_source.damage = damage;
            cf_array_push(damage_sources, damage_source);
        }
        
        if (effect != Condition_Effect_Type_None)
        {
            hand_cast_add_condition(victim, effect);
        }
    }
    
    if (cf_array_count(damage_sources))
    {
        make_event_hand_slap_damage(victim, damage_sources);
    }
}

void make_event_hand_slap_damage(ecs_entity_t victim, fixed Damage_Source* sources)
{
    C_Event* event = make_event(Event_Type_Hand_Slap_Hit);
    event->slap_damage.victim = victim;
    event->slap_damage.sources = sources;
}

void make_event_world_arena_start()
{
    make_event(Event_Type_World_Arena_Start);
}

ecs_entity_t make_creature()
{
    ecs_t* ecs = s_app->world.ecs;
    ecs_entity_t entity = ecs_create(ecs);
    C_Puppet* puppet = ECS_ADD(entity, C_Puppet);
    ECS_ADD(entity, C_Condition);
    C_AI* ai = ECS_ADD(entity, C_AI);
    ECS_ADD(entity, C_Team);
    ECS_ADD(entity, C_Hurt_Box);
    ECS_ADD(entity, C_Health);
    ECS_ADD(entity, C_Creature);
    ECS_ADD(entity, C_Floating_Text_Queue);
    ECS_ADD(entity, C_Movement);
    
    puppet->color = cf_color_white();
    cf_map_set(puppet->proportions, (u64)Condition_Effect_Type_None, make_default_body_proportions());
    ai->target = (ecs_entity_t){ -1 };
    
    return entity;
}

ecs_entity_t make_human(s32 stat_scaling, f32 body_scaling)
{
    ecs_entity_t entity = make_creature();
    C_Puppet* puppet = ECS_GET(entity, C_Puppet);
    C_AI* ai = ECS_GET(entity, C_AI);
    ECS_ADD(entity, C_Human);
    C_Health* health = ECS_GET(entity, C_Health);
    C_Creature* creature = ECS_GET(entity, C_Creature);
    C_Movement* movement = ECS_GET(entity, C_Movement);
    
    puppet->body = make_human_body(HUMAN_HEIGHT);
    puppet->stride = cf_v2(20.0f, 10.0f);
    puppet->spin_speed = 0.025f;
    
    creature->name = generate_human_name();
    Attributes attributes = { 0 };
    attributes.health = 10 * stat_scaling;
    attributes.strength = 4 * stat_scaling;
    attributes.agility = 1 * stat_scaling;
    attributes.move_speed = 40.0f;
    
    {
        Body* body = &puppet->body;
        
        Body_Proportions proportions = make_random_body_proportions();
        proportions.scale *= body_scaling;
        cf_map_set(puppet->proportions, (u64)Condition_Effect_Type_None, proportions);
        body_scale(body, proportions);
        
        if (proportions.leg_length < 1.0f)
        {
            body->particles[Body_Human_Left_Hip].y -= body->height;
            body->particles[Body_Human_Right_Hip].y -= body->height;
        }
        body_stabilize(body);
        body_verlet(body);
        body_satisfy_constraints(body, s_app->world.bounds);
        body_acceleration_reset(body);
    }
    
    *health = attributes.health;
    
    cf_map_set(creature->attributes, Condition_Effect_Type_None, attributes);
    
    ai->attack_delay = agility_to_attack_rate(creature);
    
    movement->speed = attributes.move_speed;
    
    return entity;
}

ecs_entity_t make_slime(s32 stat_scaling, f32 body_scaling)
{
    ecs_entity_t entity = make_creature();
    C_Puppet* puppet = ECS_GET(entity, C_Puppet);
    C_AI* ai = ECS_GET(entity, C_AI);
    ECS_ADD(entity, C_Slime);
    C_Health* health = ECS_GET(entity, C_Health);
    C_Creature* creature = ECS_GET(entity, C_Creature);
    C_Movement* movement = ECS_GET(entity, C_Movement);
    
    puppet->body = make_slime_body(SLIME_HEIGHT);
    puppet->stride = cf_v2(40.0f, 10.0f);
    puppet->spin_speed = 0.025f;
    
    creature->name = generate_slime_name();
    Attributes attributes = { 0 };
    attributes.health = 5 * stat_scaling;
    attributes.strength = 2 * stat_scaling;
    attributes.agility = 3 * stat_scaling;
    attributes.move_speed = 40.0f;
    {
        Body* body = &puppet->body;
        puppet->proportions->scale *= body_scaling;
        body_scale(body, *puppet->proportions);
    }
    
    *health = attributes.health;
    
    cf_map_set(creature->attributes, Condition_Effect_Type_None, attributes);
    
    ai->attack_delay = agility_to_attack_rate(creature);
    
    movement->speed = attributes.move_speed;
    
    return entity;
}

ecs_entity_t make_tentacle(s32 stat_scaling, f32 body_scaling)
{
    ecs_entity_t entity = make_creature();
    C_Puppet* puppet = ECS_GET(entity, C_Puppet);
    C_AI* ai = ECS_GET(entity, C_AI);
    ECS_ADD(entity, C_Tentacle);
    C_Health* health = ECS_GET(entity, C_Health);
    C_Creature* creature = ECS_GET(entity, C_Creature);
    
    puppet->body = make_tentacle_body(TENTACLE_HEIGHT);
    
    puppet->body.is_locked[Body_Tentacle_Root] = true;
    
    creature->name = generate_tentacle_name();
    Attributes attributes = { 0 };
    attributes.health = 3 * stat_scaling;
    attributes.strength = 1 * stat_scaling;
    attributes.agility = 6 * stat_scaling;
    
    {
        Body* body = &puppet->body;
        puppet->proportions->scale *= body_scaling;
        body_scale(body, *puppet->proportions);
    }
    
    *health = attributes.health;
    
    cf_map_set(creature->attributes, Condition_Effect_Type_None, attributes);
    
    ai->attack_delay = agility_to_attack_rate(creature);
    
    puppet->body.accelerations[Body_Tentacle_Count - 1] = cf_v2(1000.0f, -500.0f);
    
    return entity;
}

void hand_cast_add_condition(ecs_entity_t entity, Condition_Effect_Type type)
{
    if (ECS_HAS(entity, C_Condition))
    {
        C_Condition* condition = ECS_GET(entity, C_Condition);
        if (can_add_condition_effect(condition->handle, type))
        {
            s32 conflict_count = 0;
            s32 combine_count = 0;
            fixed Condition_Effect_Type* conflict_effect_types = get_conflict_condition_effects(condition->handle, type);
            fixed Condition_Combination* combinations = get_condition_combinations(type);
            fixed Condition_Effect_Type* combination_adds = NULL;
            MAKE_SCRATCH_ARRAY(combination_adds, cf_array_count(combinations));
            
            // do combining removes
            for (s32 index = 0; index < cf_array_count(combinations); ++index)
            {
                Condition_Combination combination = combinations[index];
                if (remove_condition_effect(condition->handle, combination.other))
                {
                    make_event_hand_caster_condition_remove(entity, combination.other);
                    cf_array_push(combination_adds, combination.combined);
                    ++combine_count;
                }
            }
            
            // do conflicting removes
            for (s32 index = 0; index < cf_array_count(conflict_effect_types); ++index)
            {
                Condition_Effect_Type conflict_effect_type = conflict_effect_types[index];
                if (remove_condition_effect(condition->handle, conflict_effect_type))
                {
                    make_event_hand_caster_condition_remove(entity, conflict_effect_type);
                    ++conflict_count;
                }
            }
            
            // check if can still add after removing anything
            b32 do_add = true;
            if (conflict_count && !can_add_condition_after_conflict(type))
            {
                do_add = false;
            }
            if (combine_count && !can_add_condition_after_combination(type))
            {
                do_add = false;
            }
            
            if (do_add)
            {
                add_condition_effect(condition->handle, type);
                make_event_hand_caster_condition_add(entity, type);
            }
            
            // recursive call to handle combinations this is to ensure this chains to create a chain 
            // bunch of effects
            for (s32 index = 0; index < cf_array_count(combination_adds); ++index)
            {
                hand_cast_add_condition(entity, combination_adds[index]);
            }
        }
    }
}

void attacker_add_condition(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type type)
{
    if (ECS_HAS(victim, C_Condition))
    {
        C_Condition* condition = ECS_GET(victim, C_Condition);
        if (can_add_condition_effect(condition->handle, type))
        {
            s32 conflict_count = 0;
            fixed Condition_Effect_Type* conflict_effect_types = get_conflict_condition_effects(condition->handle, type);
            fixed Condition_Combination* combinations = get_condition_combinations(type);
            fixed Condition_Effect_Type* combination_adds = NULL;
            MAKE_SCRATCH_ARRAY(combination_adds, cf_array_count(combinations));
            
            // do combining removes
            for (s32 index = 0; index < cf_array_count(combinations); ++index)
            {
                Condition_Combination combination = combinations[index];
                if (remove_condition_effect(condition->handle, combination.other))
                {
                    make_event_condition_purge(attacker, victim, combination.other);
                    cf_array_push(combination_adds, combination.combined);
                }
            }
            
            // do conflicting removes
            for (s32 index = 0; index < cf_array_count(conflict_effect_types); ++index)
            {
                Condition_Effect_Type conflict_effect_type = conflict_effect_types[index];
                if (remove_condition_effect(condition->handle, conflict_effect_type))
                {
                    make_event_condition_purge(attacker, victim, conflict_effect_type);
                    ++conflict_count;
                }
            }
            
            b32 do_add = true;
            // check if can still add after removing anything
            if (conflict_count && !can_add_condition_after_conflict(type))
            {
                do_add = false;
            }
            
            if (do_add)
            {
                add_condition_effect(condition->handle, type);
                make_event_condition_inflict(attacker, victim, type);
            }
            
            // recursive call to handle combinations this is to ensure this chains to create a chain 
            // bunch of effects
            for (s32 index = 0; index < cf_array_count(combination_adds); ++index)
            {
                attacker_add_condition(attacker, victim, combination_adds[index]);
            }
        }
    }
}

void entity_apply_items(ecs_entity_t entity)
{
    Inventory* inventory = &s_app->world.inventory;
    C_Creature* creature = ECS_GET(entity, C_Creature);
    C_Health* health = ECS_GET(entity, C_Health);
    C_Team* team = ECS_GET(entity, C_Team);
    
    for (s32 index = 0; index < cf_array_count(inventory->items); ++index)
    {
        if (inventory->items[index] <= 0)
        {
            continue;
        }
        
        Item_Type item_type = (Item_Type)index;
        u64 mask = ITEM_MASK | item_type;
        if (BIT_IS_SET(get_item_team_mask(item_type), team->base))
        {
            Attributes attributes = get_item_attributes(item_type);
            attributes = attributes_scale(attributes, inventory->items[item_type]);
            Attributes temp = { 0 };
            if ((CF_MEMCMP(&attributes, &temp, sizeof(attributes))))
            {
                cf_map_set(creature->attributes, mask, attributes);
            }
        }
    }
    
    {
        Attributes attributes = accumulate_attributes(creature->attributes);
        *health = cf_max(*health, attributes.health);
    }
}

CF_V2 floating_text_start_position(ecs_entity_t entity)
{
    CF_V2 start = cf_v2(0, 0);
    if (ENTITY_IS_VALID(entity))
    {
        if (ECS_HAS(entity, C_Puppet))
        {
            C_Puppet* puppet = ECS_GET(entity, C_Puppet);
            CF_Aabb bounds = body_get_bounds(&puppet->body);
            start = cf_top(bounds);
        }
    }
    
    return start;
}

Floating_Text* floating_text_add(ecs_entity_t entity, Floating_Text_Type type, const char* text)
{
    dyna Floating_Text* floating_texts = s_app->world.floating_texts;
    
    CF_V2 position = floating_text_start_position(entity);
    CF_V2 direction = cf_v2(0.0f, 1.0f);
    f32 delay = type == Floating_Text_Type_Neutral ? 0.075f : 0.025f;
    CF_Color color = cf_color_black();
    
    for (s32 index = 0; index < cf_array_count(floating_texts); ++index)
    {
        Floating_Text* floating_text = floating_texts + index;
        if (entity.id == floating_text->entity.id)
        {
            if (floating_text->time < floating_text->delay)
            {
                delay += floating_text->delay;
            }
        }
    }
    
    if (type == Floating_Text_Type_Negative)
    {
        direction.x += 0.25f;
        color = cf_color_red();
    }
    else if (type == Floating_Text_Type_Positive)
    {
        direction.x -= 0.25f;
        color = cf_color_green();
    }
    
    direction = cf_safe_norm(direction);
    
    Floating_Text new_text = { 0 };
    new_text.entity = entity;
    new_text.text = text != NULL ? cf_sintern(text) : NULL;
    new_text.color = color;
    new_text.position = position;
    new_text.velocity = cf_mul_v2_f(direction, 500.0f);
    new_text.duration = 1.5f;
    new_text.delay = delay;
    
    cf_array_push(s_app->world.floating_texts, new_text);
    return &cf_array_last(s_app->world.floating_texts);
}

Floating_Text* floating_text_add_fmt(ecs_entity_t entity, Floating_Text_Type type, const char* fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    return floating_text_add(entity, type, buffer);
}

void floating_text_add_damage(ecs_entity_t entity, s32 damage)
{
    Floating_Text_Type type = damage < 0 ? Floating_Text_Type_Positive : Floating_Text_Type_Negative;
    Floating_Text* new_text = floating_text_add(entity, type, NULL);
    if (new_text)
    {
        new_text->damage = cf_abs(damage);
        new_text->is_damage = true;
    }
}

// utility

ecs_entity_t find_nearest_creature(CF_V2 position, u64 team_mask)
{
    World* world = &s_app->world;
    ecs_entity_t found = (ecs_entity_t){ -1 };
    
    u64* masks = cf_map_keys(world->team_creatures);
    CF_V2 closest_distance = cf_v2(F32_MAX, F32_MAX);
    CF_V2 closest_position = position;
    
    for (s32 mask_index = 0; mask_index < cf_map_size(world->team_creatures); ++mask_index)
    {
        if (!BIT_IS_SET_EX(masks[mask_index], team_mask))
        {
            continue;
        }
        
        dyna ecs_entity_t* creatures = world->team_creatures[mask_index];
        for (s32 index = 0; index < cf_array_count(creatures); ++index)
        {
            ecs_entity_t creature = creatures[index];
            C_Puppet* puppet = ECS_GET(creature, C_Puppet);
            
            CF_V2 dp = cf_sub(puppet->body.position, position);
            CF_V2 abs_dp = cf_abs(dp);
            
            // try to find nearest by x before y, this is to avoid
            // targetting to go all the way to the backline rather than clear things closer
            if (abs_dp.x < closest_distance.x && abs_dp.y < closest_distance.y)
            {
                closest_distance = abs_dp;
                closest_position = puppet->body.position;
                found = creature;
            }
        }
    }
    
    return found;
}

void teleport(ecs_entity_t entity, CF_V2 position)
{
    if (ECS_HAS(entity, C_Puppet))
    {
        C_Puppet* puppet = ECS_GET(entity, C_Puppet);
        body_teleport(&puppet->body, position);
    }
}

CF_V2 aabb_random_point(CF_Aabb aabb, CF_Rnd* rnd)
{
    CF_V2 point;
    point.x = cf_rnd_range_float(rnd, aabb.min.x, aabb.max.x);
    point.y = cf_rnd_range_float(rnd, aabb.min.y, aabb.max.y);
    return point;
}

CF_V2 walk_direction_to_attack(Body* body, Body* target_body)
{
    World* world = &s_app->world;
    CF_Aabb walk_bounds = world->walk_bounds;
    
    CF_Aabb body_bounds = body_get_bounds(body);
    CF_Aabb target_body_bounds = body_get_bounds(target_body);
    
    CF_V2 start = body->position;
    CF_V2 end = target_body->position;
    f32 dy = end.y - start.y;
    f32 dx = end.x - start.x;
    f32 total_half_widths = cf_half_width(body_bounds) + cf_half_width(target_body_bounds);
    f32 aabb_distance = cf_abs(dx) - total_half_widths;
    
    f32 min_depth_offset = body->height * 0.1f;
    
    if (cf_abs(dy) > min_depth_offset)
    {
        end.x = start.x;
    }
    else if (aabb_distance < 0)
    {
        CF_V2 offset = cf_v2(total_half_widths, 0.0f);
        CF_V2 left_side = cf_sub(end, offset);
        CF_V2 right_side = cf_add(end, offset);
        
        b32 go_left = cf_distance(start, left_side) < cf_distance(start, right_side);
        
        if (go_left)
        {
            if (cf_contains_point(walk_bounds, left_side))
            {
                end = left_side;
            }
            else
            {
                go_left = !go_left;
            }
        }
        
        if (!go_left)
        {
            end = right_side;
        }
    }
    else if (aabb_distance <= body->height * 0.05f)
    {
        end = start;
    }
    
    CF_V2 dp = cf_sub(end, start);
    
    return cf_safe_norm(dp);
}

void set_team_base(ecs_entity_t entity, u64 team)
{
    C_Team* c_team = NULL;
    
    if (ECS_HAS(entity, C_Team))
    {
        c_team = ECS_GET(entity, C_Team);
    }
    else
    {
        c_team = ECS_ADD(entity, C_Team);
    }
    c_team->base = team;
}

void set_team(ecs_entity_t entity, u64 team)
{
    C_Team* c_team = NULL;
    
    if (ECS_HAS(entity, C_Team))
    {
        c_team = ECS_GET(entity, C_Team);
    }
    else
    {
        c_team = ECS_ADD(entity, C_Team);
    }
    c_team->next = team;
}

void team_add_creature(ecs_entity_t entity, u64 team)
{
    World* world = &s_app->world;
    
    if (!cf_map_has(world->team_creatures, team))
    {
        dyna ecs_entity_t* new_creatures = NULL;
        cf_array_fit(new_creatures, 128);
        cf_map_add(world->team_creatures, team, new_creatures);
    }
    
    dyna ecs_entity_t* creatures = cf_map_get(world->team_creatures, team);
    cf_array_push(creatures, entity);
}

void team_remove_creature(ecs_entity_t entity, u64 team)
{
    World* world = &s_app->world;
    
    if (cf_map_has(world->team_creatures, team))
    {
        dyna ecs_entity_t* creatures = cf_map_get(world->team_creatures, team);
        for (s32 index = cf_array_count(creatures) - 1; index >= 0; --index)
        {
            if (creatures[index].id == entity.id)
            {
                cf_array_del(creatures, index);
            }
        }
    }
}

b32 is_entity_friendly(ecs_entity_t self, ecs_entity_t other)
{
    b32 is_friendly = false;
    
    if (ECS_HAS(self, C_Team) && ECS_HAS(other, C_Team))
    {
        C_Team* self_team = ECS_GET(self, C_Team);
        C_Team* other_team = ECS_GET(other, C_Team);
        
        is_friendly = self_team->current == other_team->current;
    }
    
    return is_friendly;
}

b32 array_add_unique(dyna ecs_entity_t* list, ecs_entity_t item)
{
    b32 can_add = true;
    for (s32 index = 0; index < cf_array_count(list); ++index)
    {
        if (list[index].id == item.id)
        {
            can_add = false;
            break;
        }
    }
    
    if (can_add)
    {
        cf_array_push(list, item);
    }
    
    return can_add;
}

s32 strength_to_damage(C_Creature* creature)
{
    s32 strength = accumulate_attributes(creature->attributes).strength;
    s32 damage = 1 + cf_max(strength / 2, 0);
    return damage;
}

f32 agility_to_attack_rate(C_Creature* creature)
{
    f32 x = (f32)accumulate_attributes(creature->attributes).agility;
    x = x * x * x;
    f32 attack_rate = cf_max(5.0f - x / 200.0f, 1.0f);
    return attack_rate;
}

void draw_ellipse(CF_V2 center, CF_V2 size)
{
    f32 ratio = size.y / size.x;
    CF_V2 scale = cf_v2(ratio, 1.0f);
    if (size.x > size.y)
    {
        scale = cf_v2(1.0f, ratio);
    }
    
    cf_draw_push();
    cf_draw_TSR(center, scale, 0.0f);
    cf_draw_circle2(cf_v2(0, 0), size.x, 1.0f);
    cf_draw_pop();
}

b32 ellipse_contains(CF_V2 center, CF_V2 size, CF_V2 point)
{
    if (size.x < 1e-7f || size.y == 1e-7f)
    {
        return false;
    }
    
    CF_V2 dp = cf_sub(point, center);
    f32 a = (dp.x * dp.x) / (size.x * size.x);
    f32 b = (dp.y * dp.y) / (size.y * size.y);
    return (a + b) <= 1.0f;
}

Body_Proportions accumulate_body_proportions(CF_MAP(Body_Proportions) proportions)
{
    Body_Proportions total = { 0 };
    for (s32 index = 0; index < cf_map_size(proportions); ++index)
    {
        total.scale += proportions[index].scale;
        
        total.head_scale += proportions[index].head_scale;
        total.neck_thickness += proportions[index].neck_thickness;
        total.torso_chubiness += proportions[index].torso_chubiness;
        
        total.upper_arm_thickness += proportions[index].upper_arm_thickness;
        total.lower_arm_thickness += proportions[index].lower_arm_thickness;
        total.hand_thickness += proportions[index].hand_thickness;
        
        total.upper_leg_thickness += proportions[index].upper_leg_thickness;
        total.lower_leg_thickness += proportions[index].lower_leg_thickness;
        total.foot_thickness += proportions[index].foot_thickness;
        
        total.upper_arm_length += proportions[index].upper_arm_length;
        total.lower_arm_length += proportions[index].lower_arm_length;
        total.upper_leg_length += proportions[index].upper_leg_length;
        total.lower_leg_length += proportions[index].lower_leg_length;
        total.leg_length += proportions[index].leg_length;
    }
    
    return total;
}

s32 accumulate_damage_sources(dyna Damage_Source* sources)
{
    s32 total = 0;
    for (s32 index = 0; index < cf_array_count(sources); ++index)
    {
        total += sources[index].damage * sources[index].count;
    }
    return total;
}

Attributes accumulate_attributes(CF_MAP(Attributes) map)
{
    Attributes value = { 0 };
    for (s32 index = 0; index < cf_map_size(map); ++index)
    {
        value.health += map[index].health;
        value.strength += map[index].strength;
        value.agility += map[index].agility;
        value.move_speed += map[index].move_speed;
    }
    value.move_speed = cf_max(value.move_speed, 0);
    return value;
}

Attributes attributes_scale(Attributes attributes, s32 scale)
{
    Attributes value = { 0 };
    value.health = attributes.health * scale;
    value.strength = attributes.strength * scale;
    value.agility = attributes.agility * scale;
    value.move_speed = attributes.move_speed * scale;
    return value;
}

CF_Color get_team_color(u64 team)
{
    CF_Color color = cf_color_white();
    switch (team)
    {
        case 2:
        {
            color = cf_color_red();
            break;
        }
        case 3:
        {
            color = cf_color_orange();
            break;
        }
        case 4:
        {
            color = cf_color_cyan();
            break;
        }
        case 5:
        {
            color = cf_color_purple();
            break;
        }
        case 6:
        {
            color = cf_color_brown();
            break;
        }
        default:
        {
            color = cf_color_white();
        }
    }
    
    return color;
}

const char* get_color_tagged_name_ex(const char* name, u64 team)
{
    const char* tagged_name = NULL;
    if (name)
    {
        CF_Color color = get_team_color(team);
        tagged_name = scratch_fmt("<color color=#%X>%s</color>", cf_color_to_pixel(color).val, name);
    }
    return name;
}

const char* get_color_tagged_name(ecs_entity_t entity)
{
    u64 team = 0;
    const char* name = NULL;
    if (ECS_HAS(entity, C_Creature))
    {
        name = ((C_Creature*)ECS_GET(entity, C_Creature))->name;
    }
    if (ECS_HAS(entity, C_Team))
    {
        team = ((C_Team*)ECS_GET(entity, C_Team))->current;
    }
    
    return get_color_tagged_name_ex(name, team);
}
