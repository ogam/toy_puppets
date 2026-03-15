#include "game/overworld.h"
#include <time.h>

enum
{
    UI_Item_State_Overworld_Room_Cannot_Reach = 10,
    UI_Item_State_Overworld_Room_Done = 11,
    UI_Item_State_Overworld_Room_Current = 12,
};

void init_overworld()
{
    Overworld* overworld = &s_app->overworld;
    
    overworld->level = 0;
    
    overworld->arena = cf_make_arena(16, MB(1));
    // debug
    //overworld->rnd = cf_rnd_seed((u64)0xbeefbead);
    {
        time_t seconds = time(NULL);
        overworld->rnd = cf_rnd_seed(cf_get_tick_frequency() + seconds);
    }
}

void update_overworld()
{
    Overworld* overworld = &s_app->overworld;
}

void draw_overworld()
{
    Overworld* overworld = &s_app->overworld;
}

void overworld_advance_lanes(CF_Aabb* lanes, s32 count, CF_V2 offset)
{
    for (s32 index = 0; index < count; ++index)
    {
        lanes[index] = move_aabb(lanes[index], offset);
    }
}

void overworld_generate()
{
    Overworld* overworld = &s_app->overworld;
    CF_Rnd* rnd = &overworld->rnd;
    s32 depth = 9;
    
    CF_V2 depth_offset = cf_v2(0.2f, 0.0f);
    CF_V2 min = cf_v2(0.0f, 0.0f);
    CF_V2 max = cf_v2(0.0f, 0.9f);
    max = cf_add(max, depth_offset);
    CF_Aabb depth_bounds = cf_make_aabb(min, max);
    CF_V2 extents = cf_extents(depth_bounds);
    CF_V2 padding = cf_v2(-0.1f, -0.1f);
    
    // 3 horizontal lanes
    CF_Aabb lanes[] = 
    {
        cf_make_aabb_from_top_left(cf_top_left(depth_bounds), depth_offset.x, extents.y / 3.0f),
        cf_make_aabb_from_top_left(cf_top_left(depth_bounds), depth_offset.x, extents.y / 3.0f),
        cf_make_aabb_from_top_left(cf_top_left(depth_bounds), depth_offset.x, extents.y / 3.0f),
    };
    
    lanes[1] = move_aabb(lanes[1], cf_v2(0.0f, -extents.y / 3.0f));
    lanes[2] = move_aabb(lanes[2], cf_v2(0.0f, 2.0f * -extents.y / 3.0f));
    
    s32 room_count = 3 * depth + 2;
    fixed Overworld_Room* rooms = NULL;
    MAKE_ARENA_ARRAY(&overworld->arena, rooms, room_count);
    
    // place rooms in random spots in each lane
    {
        // first room 
        {
            Overworld_Room room = { 0 };
            room.position = aabb_random_point(cf_expand_aabb(lanes[1], padding), rnd);
            room.lane = 1;
            room.is_connected = true;
            MAKE_ARENA_ARRAY(&overworld->arena, room.next_rooms, 3);
            cf_array_push(rooms, room);
            overworld_advance_lanes(lanes, CF_ARRAY_SIZE(lanes), depth_offset);
        }
        
        // intermediate rooms
        for (s32 index = 1; index <= depth; ++index)
        {
            for (s32 lane_index = 0; lane_index < CF_ARRAY_SIZE(lanes); ++lane_index)
            {
                Overworld_Room room = { 0 };
                room.position = aabb_random_point(cf_expand_aabb(lanes[lane_index], padding), rnd);
                room.depth = index;
                room.lane = lane_index;
                MAKE_ARENA_ARRAY(&overworld->arena, room.next_rooms, 3);
                cf_array_push(rooms, room);
            }
            
            depth_bounds = move_aabb(depth_bounds, depth_offset);
            overworld_advance_lanes(lanes, CF_ARRAY_SIZE(lanes), depth_offset);
        }
        
        // last room
        {
            Overworld_Room room = { 0 };
            room.position = aabb_random_point(cf_expand_aabb(lanes[1], padding), rnd);
            room.depth = depth + 1;
            room.lane = 1;
            MAKE_ARENA_ARRAY(&overworld->arena, room.next_rooms, 3);
            cf_array_push(rooms, room);
        }
    }
    
    // connect rooms
    {
        for (s32 index = 0; index < cf_array_count(rooms) - 1; ++index)
        {
            Overworld_Room* room = rooms + index;
            if (!room->is_connected)
            {
                continue;
            }
            s32 current_depth = room->depth;
            
            s32 start_depth_index = index;
            s32 end_depth_index = index;
            
            // find next set rooms at next depth
            for (s32 next_index = index + 1; next_index < cf_array_count(rooms); ++next_index)
            {
                Overworld_Room* next_room = rooms + next_index;
                if (next_room->depth == current_depth + 1)
                {
                    if (start_depth_index == index)
                    {
                        start_depth_index = next_index;
                        end_depth_index = next_index + 1;
                    }
                }
                else if (next_room->depth > current_depth + 1)
                {
                    end_depth_index = next_index;
                    break;
                }
            }
            
            // if room is all ready connected, keep trying to roll for connections
            // this is to avoid hitting a corner
            while (cf_array_count(room->next_rooms) < 1)
            {
                s32 connection_mask = cf_rnd_range_int(rnd, -29185401, 201583093) & 0b111;
                for (s32 next_index = start_depth_index; next_index < end_depth_index; ++next_index)
                {
                    Overworld_Room* next_room = rooms + next_index;
                    
                    if (cf_abs(next_room->lane - room->lane) <= 1)
                    {
                        if (BIT_IS_SET(connection_mask, next_room->lane))
                        {
                            cf_array_push(room->next_rooms, next_index);
                            next_room->is_connected = true;
                        }
                    }
                }
            }
        }
    }
    
    // roll room types
    {
        for (s32 index = 1; index < cf_array_count(rooms) - 1; ++index)
        {
            Overworld_Room* room = rooms + index;
            room->seed = cf_rnd_uint64(rnd);
            if (room->depth < 3)
            {
                room->type = cf_rnd_float(rnd) < 0.9f ? Overworld_Room_Type_Normal : Overworld_Room_Type_Shop;
            }
            else
            {
                room->type = cf_rnd_range_int(rnd, Overworld_Room_Type_Normal, Overworld_Room_Type_Shop);
            }
            
        }
        
        cf_array_last(rooms).type = Overworld_Room_Type_Boss;
    }
    
    overworld->rooms = rooms;
    MAKE_ARENA_ARRAY(&overworld->arena, overworld->path, depth + 2);
    cf_array_push(overworld->path, 0);
}

void overworld_clear()
{
    Overworld* overworld = &s_app->overworld;
    cf_arena_reset(&overworld->arena);
    overworld->rooms = NULL;
    overworld->path = NULL;
    overworld->level = 0;
}

void overworld_generate_shop(s32 depth, u64 seed)
{
    Overworld* overworld = &s_app->overworld;
    Overworld_Shop* shop = &overworld->shop;
    
    CF_Rnd rnd = cf_rnd_seed(seed);
    
    if (shop->spells == NULL)
    {
        cf_array_fit(shop->spells, Spell_Type_Count);
        cf_array_setlen(shop->spells, Spell_Type_Count);
    }
    
    if (shop->bodies == NULL)
    {
        cf_array_fit(shop->bodies, Body_Type_Count);
        cf_array_setlen(shop->bodies, Body_Type_Count);
    }
    
    if (shop->items == NULL)
    {
        cf_array_fit(shop->items, Item_Type_Count);
        cf_array_setlen(shop->items, Item_Type_Count);
    }
    
    CF_MEMSET(shop->bodies, 0, sizeof(*shop->bodies) * Body_Type_Count);
    CF_MEMSET(shop->spells, 0, sizeof(*shop->spells) * Spell_Type_Count);
    CF_MEMSET(shop->items, 0, sizeof(*shop->items) * Item_Type_Count);
    
    s32 spell_count = cf_rnd_range_int(&rnd, 3, 8);
    s32 body_count = cf_rnd_range_int(&rnd, 1, 3);
    s32 item_count = cf_rnd_range_int(&rnd, 1, 2);
    
    s32 level_offset = overworld->level * 10;
    s32 cost_limit = (level_offset + depth) * 5;
    
    for (s32 index = 0; index < spell_count; ++index)
    {
        Spell_Type type = cf_rnd_range_int(&rnd, Spell_Type_None + 1, Spell_Type_Count - 1);
        s32 attempts = 5;
        while (overworld_shop_get_spell_cost(type) > cost_limit && attempts > 0)
        {
            type = cf_rnd_range_int(&rnd, Spell_Type_None + 1, Spell_Type_Count - 1);
            --attempts;
        }
        
        if (attempts == 0)
        {
            continue;
        }
        shop->spells[type] += 1;
    }
    
    for (s32 index = 0; index < body_count; ++index)
    {
        Body_Type type = cf_rnd_range_int(&rnd, Body_Type_Human, Body_Type_Tubeman);
        s32 attempts = 5;
        while (overworld_shop_get_body_cost(type) > cost_limit && attempts > 0)
        {
            type = cf_rnd_range_int(&rnd, Body_Type_Human, Body_Type_Slime);
            --attempts;
        }
        
        if (attempts == 0)
        {
            continue;
        }
        
        shop->bodies[type] += 1;
    }
    
    for (s32 index = 0; index < item_count; ++index)
    {
        Item_Type type = cf_rnd_range_int(&rnd, Item_Type_None + 1, Item_Type_Count - 1);
        shop->items[type] += 1;
    }
}

b32 overworld_buy_body(Overworld_Shop* shop, Body_Type type)
{
    b32 buy = false;
    if (shop->bodies[type] > 0)
    {
        --shop->bodies[type];
        buy = true;
    }
    
    return buy;
}

b32 overworld_buy_spell(Overworld_Shop* shop, Spell_Type type)
{
    b32 buy = false;
    if (shop->spells[type] > 0)
    {
        --shop->spells[type];
        buy = true;
    }
    
    return buy;
}

b32 overworld_buy_item(Overworld_Shop* shop, Item_Type type)
{
    b32 buy = false;
    if (shop->items[type] > 0)
    {
        --shop->items[type];
        buy = true;
    }
    
    return buy;
}

s32 overworld_shop_get_body_cost(Body_Type type)
{
    s32 gold = 0;
    
    switch (type)
    {
        case Body_Type_Human:
        {
            gold = 10;
            break;
        }
        case Body_Type_Tentacle:
        {
            gold = 5;
            break;
        }
        case Body_Type_Slime:
        {
            gold = 8;
            break;
        }
        case Body_Type_Tubeman:
        {
            gold = 15;
            break;
        }
    }
    
    return gold;
}

s32 overworld_shop_get_spell_cost(Spell_Type type)
{
    s32 gold = 0;
    
    switch (type)
    {
        case Spell_Type_Big_Hand:
        {
            gold = 10;
            break;
        }
        case Spell_Type_Enlarge:
        {
            gold = 15;
            break;
        }
        case Spell_Type_Shrink:
        {
            gold = 15;
            break;
        }
        case Spell_Type_Death:
        {
            gold = 100;
            break;
        }
        case Spell_Type_Ignite:
        {
            gold = 25;
            break;
        }
        case Spell_Type_Regen:
        {
            gold = 40;
            break;
        }
        case Spell_Type_Vigor:
        {
            gold = 25;
            break;
        }
        case Spell_Type_Charm:
        {
            gold = 50;
            break;
        }
    }
    
    return gold;
}

s32 overworld_shop_get_item_cost(Item_Type type)
{
    s32 gold = 0;
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            gold = 30;
            break;
        }
        case Item_Type_Speed_Boots:
        {
            gold = 30;
            break;
        }
        case Item_Type_Burning_Hands:
        {
            gold = 20;
            break;
        }
        case Item_Type_Heart_Container:
        {
            gold = 25;
            break;
        }
    }
    
    return gold;
}

CF_Sprite overworld_get_room_sprite(Overworld_Room_Type type)
{
    CF_Sprite sprite = assets_get_sprite(ASSETS_DEFAULT);
    
    switch (type)
    {
        case Overworld_Room_Type_Normal:
        {
            sprite = assets_get_sprite("sprites/chess_pawn.ase");
            break;
        }
        case Overworld_Room_Type_Elite:
        {
            sprite = assets_get_sprite("sprites/chess_knight.ase");
            break;
        }
        case Overworld_Room_Type_Boss:
        {
            sprite = assets_get_sprite("sprites/chess_king.ase");
            break;
        }
        case Overworld_Room_Type_Shop:
        {
            sprite = assets_get_sprite("sprites/pouch.ase");
            break;
        }
    }
    
    return sprite;
}

void overworld_draw_room_button(UI_Layout* layout, UI_Item* item)
{
    ui_draw_sprite(layout, item);
    
    f32 thickness = 10.0f;
    
    // cross
    if (BIT_IS_SET(item->state, UI_Item_State_Overworld_Room_Cannot_Reach))
    {
        cf_draw_push_color(cf_color_red());
        
        cf_draw_line(item->aabb.min, item->aabb.max, thickness);
        cf_draw_line(cf_top_left(item->aabb), cf_bottom_right(item->aabb), thickness);
        
        cf_draw_pop_color();
    }
    
    // check mark
    if (BIT_IS_SET(item->state, UI_Item_State_Overworld_Room_Done))
    {
        cf_draw_push_color(cf_color_green());
        
        CF_V2 p0 = cf_left(item->aabb);
        CF_V2 p1 = cf_bottom(item->aabb);
        CF_V2 p2 = cf_top_right(item->aabb);
        
        cf_draw_line(p0, p1, thickness);
        cf_draw_line(p1, p2, thickness);
        
        cf_draw_pop_color();
    }
    
    // circle
    if (BIT_IS_SET(item->state, UI_Item_State_Overworld_Room_Current))
    {
        cf_draw_push_color(cf_color_yellow());
        
        f32 radius = cf_half_width(item->aabb) * 0.8f;
        cf_draw_circle2(cf_center(item->aabb), radius, thickness);
        
        cf_draw_pop_color();
    }
}

b32 overworld_room_button(CF_Sprite sprite, CF_Aabb aabb)
{
    CF_V2 extents = cf_extents(aabb);
    sprite.scale = cf_v2(extents.x / sprite.w, extents.y / sprite.h);
    
    b32 clicked = ui_do_sprite_button(sprite);
    UI_Item* item = ui_layout_peek_item();
    item->aabb = aabb;
    item->text_aabb = aabb;
    item->interactable_aabb = aabb;
    
    item->custom_draw = overworld_draw_room_button;
    
    if (clicked)
    {
        audio_play_ui("sounds/ui_button.wav");
    }
    
    return clicked;
}

typedef struct Overworld_Draw_Line
{
    CF_Aabb start;
    CF_Aabb end;
} Overworld_Draw_Line;

// horizontal bias on curve
void overworld_draw_room_connection(UI_Layout* layout, UI_Item* item)
{
    Overworld_Draw_Line* line = (Overworld_Draw_Line*)item->custom_data;
    
    CF_V2 offset = ui_layout_get_scroll_offset(layout);
    // multiplying this by 3 so it's centered, adding extents will get this to bottom left
    offset = cf_add(offset, cf_mul_v2_f(cf_half_extents(item->aabb), 3.0f));
    
    CF_V2 p0 = cf_center(line->start);
    CF_V2 p3 = cf_center(line->end);
    p0 = cf_add(p0, offset);
    p3 = cf_add(p3, offset);
    
    CF_V2 p1 = cf_lerp(p0, p3, 0.5f);
    CF_V2 p2 = p1;
    
    if (p1.y > p0.y)
    {
        p1.y = p0.y;
        p2.y = p3.y;
    }
    else if (p1.y < p0.y)
    {
        p1.y = p3.y;
        p2.y = p0.y;
    }
    
    cf_draw_push_color(item->background_color);
    cf_draw_bezier_line2(p0, p1, p2, p3, 32, 5.0f);
    cf_draw_pop_color();
}

void overworld_room_connection(CF_Aabb start, CF_Aabb end)
{
    UI_Item* item = ui_make_item();
    
    CF_Aabb aabb = start;
    
    Overworld_Draw_Line* line = (Overworld_Draw_Line*)scratch_alloc(sizeof(Overworld_Draw_Line));
    line->start = start;
    line->end = end;
    
    item->aabb = aabb;
    item->text_aabb = aabb;
    item->interactable_aabb = aabb;
    
    item->custom_draw = overworld_draw_room_connection;
    item->custom_data = line;
    item->custom_size = sizeof(*line);
}

void overworld_ui()
{
    static b32 do_scroll = true;
    
    Overworld* overworld = &s_app->overworld;
    
    fixed Overworld_Room* rooms = overworld->rooms;
    
    CF_V2 button_half_extents = cf_v2(48.0f, 48.0f);
    
    CF_V2 top_left = cf_v2(UI_WIDTH * 0.05f, UI_HEIGHT * 0.95f);
    CF_V2 extents = cf_v2(UI_WIDTH * 0.9f, UI_HEIGHT * 0.9f);
    CF_V2 layout_padding = cf_v2(-24.0f, -24.0f);
    CF_Aabb aabb = cf_make_aabb_from_top_left(top_left, extents.x, extents.y);
    
    CF_V2 inner_extents = cf_add(extents, layout_padding);
    
    ui_push_layout_background_color(cf_color_black());
    ui_push_layout_border_thickness(1.0f);
    ui_push_layout_corner_radius(5.0f);
    UI_Layout* layout = ui_layout_begin("Overworld");
    ui_pop_layout_corner_radius();
    ui_pop_layout_border_thickness();
    ui_pop_layout_background_color();
    
    ui_layout_set_aabb(aabb);
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Bottom) | BIT(UI_Layout_Alignment_Horizontal_Left));
    ui_layout_set_direction(0);
    ui_layout_expand_usable_aabb(layout_padding);
    ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Horizontal), cf_v2(0, 0));
    ui_layout_do_scrollbar(false);
    
    ui_push_item_alignment(UI_Item_Alignment_Left);
    
    // handle first time open scrolling, this is horizontal only
    if (do_scroll)
    {
        Overworld_Room* current_room = rooms + cf_array_last(overworld->path);
        Overworld_Room last_room = cf_array_last(rooms);
        
        f32 current_x = current_room->position.x * inner_extents.x - button_half_extents.x;
        f32 last_x = last_room.position.x * inner_extents.x + button_half_extents.x;
        
        f32 scroll_x = cf_clamp01(current_x / last_x);
        // close enough to the end, just show the last section
        if (scroll_x > 0.7f)
        {
            scroll_x = 1.0f;
        }
        layout->scroll.x = cf_lerp(layout->scroll.x, scroll_x, CF_DELTA_TIME * 3.0f);
        layout->scroll.x = cf_clamp01(layout->scroll.x);
        
        if (cf_abs(layout->scroll.x - scroll_x) < 1e-3f)
        {
            do_scroll = false;
        }
    }
    
    // draw connections
    for (s32 index = 0; index < cf_array_count(rooms); ++index)
    {
        Overworld_Room* room = rooms + index;
        CF_V2 position = cf_mul(inner_extents, room->position);
        CF_Aabb start = cf_make_aabb_center_half_extents(position, button_half_extents);
        
        // mkake current path options a bit more colorful
        CF_Color color = cf_color_white();
        if (cf_array_last(overworld->path) == index)
        {
            f32 t = (cf_cos((f32)CF_SECONDS * 5.0f) + 1.0f) * 0.5f;
            color = cf_color_lerp(cf_color_white(), cf_color_green(), t);
        }
        
        ui_push_background_color(color);
        for (s32 connection_index = 0; connection_index < cf_array_count(room->next_rooms); ++connection_index)
        {
            Overworld_Room* connection_room = rooms + room->next_rooms[connection_index];
            position = cf_mul(inner_extents, connection_room->position);
            CF_Aabb end = cf_make_aabb_center_half_extents(position, button_half_extents);
            
            overworld_room_connection(start, end);
        }
        ui_pop_background_color();
    }
    
    // room buttons
    {
        Overworld_Room* current = rooms + cf_array_last(overworld->path);
        for (s32 index = 0; index < cf_array_count(rooms); ++index)
        {
            Overworld_Room* room = rooms + index;
            if (!room->is_connected)
            {
                continue;
            }
            
            CF_Sprite sprite = overworld_get_room_sprite(room->type);
            
            b32 is_current = current == room;
            b32 is_path = false;
            b32 is_connection = false;
            
            for (s32 path_index = 0; path_index < cf_array_count(overworld->path); ++path_index)
            {
                if (overworld->path[path_index] == index)
                {
                    is_path = true;
                    break;
                }
            }
            
            if (!is_path)
            {
                for (s32 connection_index = 0; connection_index < cf_array_count(current->next_rooms); ++connection_index)
                {
                    if (current->next_rooms[connection_index] == index)
                    {
                        is_connection = true;
                        break;
                    }
                }
            }
            
            CF_V2 position = cf_mul(inner_extents, room->position);
            CF_Aabb aabb = cf_make_aabb_center_half_extents(position, button_half_extents);
            
            b32 is_button_disabled = is_current || is_path || !is_connection;
            if (is_button_disabled)
            {
                ui_push_is_disabled(true);
            }
            
            if (overworld_room_button(sprite, aabb))
            {
                cf_array_push(overworld->path, index);
                do_scroll = true;
                
                if (room->type == Overworld_Room_Type_Shop)
                {
                    overworld_generate_shop(room->depth, room->seed);
                    world_set_state(World_State_Shop);
                }
                else
                {
                    CF_Rnd room_rnd = cf_rnd_seed(room->seed);
                    s32 enemy_count = 0;
                    s32 elite_count = 0;
                    s32 boss_count = 0;
                    if (room->type == Overworld_Room_Type_Normal)
                    {
                        enemy_count = cf_rnd_range_int(&room_rnd, cf_min(room->depth, 5), 5);
                    }
                    else if (room->type == Overworld_Room_Type_Elite)
                    {
                        enemy_count = cf_rnd_range_int(&room_rnd, cf_min(room->depth, 5), 5);
                        elite_count = cf_rnd_range_int(&room_rnd, cf_min(room->depth / 2, 3), 5);
                    }
                    else if (room->type == Overworld_Room_Type_Boss)
                    {
                        boss_count = cf_rnd_range_int(&room_rnd, 1, 2);
                    }
                    
                    world_arena_enter_placement(&room_rnd, enemy_count, elite_count, boss_count);
                }
            }
            if (is_button_disabled)
            {
                ui_pop_is_disabled();
            }
            
            // draw different icons on top of button to denote if you can path to next room
            UI_Item* item = ui_layout_peek_item();
            
            if (is_current)
            {
                BIT_SET(item->state, UI_Item_State_Overworld_Room_Current);
            }
            else
            {
                if (is_path)
                {
                    BIT_SET(item->state, UI_Item_State_Overworld_Room_Done);
                }
                else if (!is_connection)
                {
                    // don't allow button to be interactable if you can't reach it
                    if (current->depth >= room->depth)
                    {
                        BIT_SET(item->state, UI_Item_State_Overworld_Room_Cannot_Reach);
                    }
                }
            }
            
            game_ui_tooltip(Overworld_Room_Type_names[room->type]);
        }
    }
    
    ui_pop_item_alignment();
    ui_layout_end();
}

Overworld_Room* overworld_get_current_room()
{
    Overworld* overworld = &s_app->overworld;
    Overworld_Room* room = NULL;
    
    if (cf_array_count(overworld->rooms) && 
        cf_array_last(overworld->path) < cf_array_count(overworld->rooms))
    {
        room = overworld->rooms + cf_array_last(overworld->path);
    }
    
    return room;
}

void overworld_current_room_add_gold(s32 gold)
{
    Overworld_Room* room = overworld_get_current_room();
    if (room)
    {
        room->gold = cf_max(room->gold + gold, 0);
    }
}

void overworld_current_room_enemy_died(b32 is_normal, b32 is_elite, b32 is_boss)
{
    if (is_normal)
    {
        overworld_current_room_add_gold(5);
    }
    else if (is_elite)
    {
        overworld_current_room_add_gold(15);
    }
    else if (is_boss)
    {
        overworld_current_room_add_gold(100);
    }
}

b32 overworld_can_advance()
{
    Overworld* overworld = &s_app->overworld;
    return cf_array_last(overworld->path) < cf_array_count(overworld->rooms) - 1;
}

b32 overworld_is_level_finished()
{
    Overworld* overworld = &s_app->overworld;
    return cf_array_last(overworld->path) >= cf_array_count(overworld->rooms) - 1;
}