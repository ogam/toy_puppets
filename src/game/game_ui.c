#include "game/game_ui.h"

CF_Color menu_layout_background_color()
{
    return cf_make_color_rgba_f(0.0f, 0.0f, 0.0f, 0.5f);
}

Layout_Cache* game_ui_cache_layout(dyna Layout_Cache* layout_caches, UI_Layout* layout, CF_Arena* arena)
{
    cf_array_push(layout_caches, (Layout_Cache){ 0 });
    Layout_Cache* cache = &cf_array_last(layout_caches);
    
    cache->direction = layout->direction;
    cache->grid_direction = layout->grid_direction;
    cache->alignment = layout->alignment;
    cache->scroll_direction = layout->scroll_direction;
    cache->state = layout->state;
    
    cache->background_color = layout->background_color;
    cache->border_color = layout->border_color;
    cache->title_color = layout->title_color;
    cache->border_thickness = layout->border_thickness;
    cache->corner_radius = layout->corner_radius;
    cache->title_font_size = layout->title_font_size;
    cache->item_padding = layout->item_padding;
    cache->sprite = layout->sprite;
    cache->sprite_mode = layout->sprite_mode;
    
    
    {
        UI_Layout_Var var = ui_layout_get_var_ex(layout, cf_sintern("aabb"));
        cache->aabb = var.aabb;
    }
    
    cache->title = layout->title != NULL ? arena_fmt(arena, layout->title) : NULL;
    cache->title_font_size = layout->title_font_size;
    
    if (cf_array_count(layout_caches) == 1 && cache->title == NULL)
    {
        cache->title = arena_fmt(arena, "Details");
    }
    
    MAKE_ARENA_ARRAY(arena, cache->items, cf_array_count(layout->items));
    
    for (s32 index = 0; index < cf_array_count(layout->items); ++index)
    {
        UI_Item item = ui_copy_item(layout->items + index, arena);
        if (item.layout)
        {
            item.layout = (UI_Layout*)game_ui_cache_layout(layout_caches, item.layout, arena);
        }
        cf_array_push(cache->items, item);
    }
    
    return cache;
}

s32 get_layout_count(UI_Layout* layout)
{
    s32 count = 0;
    if (layout)
    {
        ++count;
        for (s32 index = 0; index < cf_array_count(layout->items); ++index)
        {
            UI_Item item = layout->items[index];
            count += get_layout_count(item.layout);
        }
    }
    return count;
}

void game_ui_pin_tooltip()
{
    static s32 counter = 0;
    
    UI* ui = &s_app->ui;
    Game_UI* game_ui = &s_app->game_ui;
    if (game_ui->pin_tooltips == NULL)
    {
        cf_array_fit(game_ui->pin_tooltips, 64);
    }
    
    UI_Layout* layout = ui_peek_layout();
    
    Pin_Tooltip pin_tooltip = { 0 };
    pin_tooltip.id = counter++;
    pin_tooltip.arena = cf_make_arena(16, KB(8));
    
    MAKE_ARENA_ARRAY(&pin_tooltip.arena, pin_tooltip.layouts, get_layout_count(layout) + 1);
    game_ui_cache_layout(pin_tooltip.layouts, layout, &pin_tooltip.arena);
    
    cf_array_push(game_ui->pin_tooltips, pin_tooltip);
}

void game_ui_destroy_pin_tooltip(Pin_Tooltip* tooltip)
{
    Game_UI* game_ui = &s_app->game_ui;
    s32 index  = (s32)(tooltip - game_ui->pin_tooltips);
    
    if (index >= 0 && index < cf_array_count(game_ui->pin_tooltips))
    {
        Layout_Cache* layouts = tooltip->layouts;
        cf_destroy_arena(&tooltip->arena);
        
        void* dst = game_ui->pin_tooltips + index;
        void* src = game_ui->pin_tooltips + index + 1;
        s32 copy_count = cf_array_count(game_ui->pin_tooltips) - index - 1;
        CF_MEMCPY(dst, src, sizeof(*game_ui->pin_tooltips) * copy_count);
        cf_array_setlen(game_ui->pin_tooltips, cf_array_count(game_ui->pin_tooltips) - 1);
    }
}

// will show a tooltip of and details of spell
// example usage
// <condition type=0>Condition Name</condition>
bool game_ui_text_fx_spell(CF_TextEffect* fx)
{
    Condition_Effect_Type effect = (Condition_Effect_Type)cf_text_effect_get_number(fx, "type", 0);
    fx->color = get_spell_text_color(effect);
    
    return true;
}

bool game_ui_text_fx_spell_hover(CF_TextEffect *fx)
{
    Spell_Type spell = (Spell_Type)cf_text_effect_get_number(fx, "type", 0);
    
    game_ui_tooltip_begin();
    ui_layout_set_title(scratch_fmt("Spell: %s", get_spell_name(spell)));
    ui_do_text(get_spell_description(spell));
    game_ui_tooltip_end();
    
    return true;
}

// will show a tooltip of and details of condition
// example usage
// <condition type=0>Condition Name</condition>
bool game_ui_text_fx_condition(CF_TextEffect* fx)
{
    Condition_Effect_Type effect = (Condition_Effect_Type)cf_text_effect_get_number(fx, "type", 0);
    fx->color = get_condition_text_color(effect);
    
    return true;
}

bool game_ui_text_fx_condition_hover(CF_TextEffect *fx)
{
    Condition_Effect_Type effect = (Condition_Effect_Type)cf_text_effect_get_number(fx, "type", 0);
    
    game_ui_tooltip_begin();
    ui_layout_set_title(scratch_fmt("Condition: %s", get_condition_name(effect)));
    ui_do_text(get_condition_description(effect));
    game_ui_tooltip_end();
    
    return true;
}

// will show a tooltip of and details of item effects
// example usage
// <item type=0>Item Name</item>
bool game_ui_text_fx_item(CF_TextEffect* fx)
{
    Item_Type item_type = (Condition_Effect_Type)cf_text_effect_get_number(fx, "type", 0);
    fx->color = get_item_text_color(item_type);
    
    return true;
}

bool game_ui_text_fx_item_hover(CF_TextEffect *fx)
{
    Item_Type item_type = (Condition_Effect_Type)cf_text_effect_get_number(fx, "type", 0);
    
    game_ui_tooltip_begin();
    ui_layout_set_title(scratch_fmt("Item: %s", get_item_name(item_type)));
    ui_do_text(get_item_description(item_type));
    game_ui_tooltip_end();
    
    return true;
}

// shows damage tooltip breakdown from base + conditions + spells
// example usage
// <damage log_id=0>123</damage>
bool game_ui_text_fx_damage(CF_TextEffect* fx)
{
    Game_UI* game_ui = &s_app->game_ui;
    s32 log_id = (s32)cf_text_effect_get_number(fx, "log_id", -1);
    if (log_id >= 0 && cf_array_count(game_ui->logs))
    {
        Game_Log* log = game_ui->logs + log_id;
        s32 damage = accumulate_damage_sources(log->damage_sources);
        fx->color = damage < 0 ? cf_color_green() : cf_color_red();
    }
    
    return true;
}

bool game_ui_text_fx_damage_hover(CF_TextEffect *fx)
{
    Game_UI* game_ui = &s_app->game_ui;
    s32 log_id = (s32)cf_text_effect_get_number(fx, "log_id", -1);
    if (log_id >= 0 && cf_array_count(game_ui->logs))
    {
        Game_Log* log = game_ui->logs + log_id;
        fixed char* buffer = scratch_string(1024);
        
        s32 total = 0;
        for (s32 index = 0; index < cf_array_count(log->damage_sources); ++index)
        {
            Damage_Source source = log->damage_sources[index];
            cf_string_fmt_append(buffer, "%d ", source.damage);
            if (source.effect != Condition_Effect_Type_None)
            {
                const char* condition_name = get_condition_name(source.effect);
                cf_string_fmt_append(buffer, "(<condition type=%d>%s</condition>) ", source.effect, condition_name);
            }
            if (source.spell != Spell_Type_None)
            {
                const char* spell_name = get_spell_name(source.spell);
                cf_string_fmt_append(buffer, "(<spell type=%d>%s</spell>) ", source.spell, spell_name);
            }
            if (source.item != Item_Type_None)
            {
                const char* item_name = get_item_name(source.item);
                if (source.count > 1)
                {
                    cf_string_fmt_append(buffer, "(<item type=%d>%d x %s</item>) ", source.item, source.count, item_name);
                }
                else
                {
                    cf_string_fmt_append(buffer, "(<item type=%d>%s</item>) ", source.item, item_name);
                }
            }
            
            if (index + 1 < cf_array_count(log->damage_sources))
            {
                cf_string_append(buffer, "+ ");
            }
            total += source.damage;
        }
        cf_string_fmt_append(buffer, "= %d", total);
        
        game_ui_tooltip_begin();
        ui_layout_set_title("Damage");
        ui_do_text(buffer);
        game_ui_tooltip_end();
    }
    
    return true;
}

// embed_tooltip is when you want to have a tooltip within tooltip
// so lets put the top tooltip as the parent, and inner being child
// so whenever the parent tooltip gets pinned the child tooltip will still have the tagged text effects
// example usage
// <embed_tooltip title="My title" text="My Tooltip String">contents</embed_tooltip>
bool game_ui_text_fx_embed_tooltip(CF_TextEffect* fx)
{
    return true;
}

bool game_ui_text_fx_embed_tooltip_hover(CF_TextEffect *fx)
{
    UI* ui = &s_app->ui;
    const char* title = cf_text_effect_get_string(fx, "title", NULL);
    const char* text = cf_text_effect_get_string(fx, "text", NULL);
    // CF will send in a sanitized string whenever you pull out an embeded string
    // most likely there won't be 128 tags, if there is then this is most likley a bug
    fixed char* fixed_tagged_text = scratch_string((s32)CF_STRLEN(text) + 128);
    
    cf_string_append(fixed_tagged_text, text);
    fixed char* buffer_find = scratch_string(256);
    fixed char* buffer_replace = scratch_string(256);
    
    const char** keys = (const char**)cf_map_keys(ui->text_effect_map);
    for (s32 index = 0; index < cf_map_size(ui->text_effect_map); ++index)
    {
        cf_string_fmt(buffer_find, "<%s>", keys[index]);
        cf_string_fmt(buffer_replace, "</%s>", keys[index]);
        cf_string_replace(fixed_tagged_text, buffer_find, buffer_replace);
    }
    
    if (text)
    {
        game_ui_tooltip_begin();
        if (title)
        {
            ui_layout_set_title(title);
        }
        ui_do_text(fixed_tagged_text);
        game_ui_tooltip_end();
    }
    
    return true;
}

void init_game_ui()
{
    Game_UI* game_ui = &s_app->game_ui;
    
    // try to stay a little below the capacity of a layout->items, otherwise this can balloon memory
    // as you aren't gaurenteed the same layout when you switch between menus
    s32 log_capacity = UI_LAYOUT_ITEM_MIN_CAPACITY - 4;
    cf_array_fit(game_ui->logs, log_capacity);
    
    node_pool_init(&game_ui->log_pool);
    
    for (s32 index = 0; index < log_capacity; ++index)
    {
        Game_Log log = { 0 };
        log.arena = cf_make_arena(16, KB(8));
        cf_array_push(game_ui->logs, log);
        
        node_pool_add(&game_ui->log_pool, &cf_array_last(game_ui->logs).node);
    }
    
    game_ui->do_close_window = cf_make_button_binding(0, 0.1f);
    cf_button_binding_add_key(game_ui->do_close_window, CF_KEY_ESCAPE);
    
    cf_array_fit(game_ui->states, 8);
    game_ui_set_state(Game_UI_State_Splash);
    game_ui->state_co = cf_make_coroutine(game_ui_do_splash_co, 0, NULL);
    
    ui_register_text_effect("spell", game_ui_text_fx_spell, game_ui_text_fx_spell_hover);
    ui_register_text_effect("condition", game_ui_text_fx_condition, game_ui_text_fx_condition_hover);
    ui_register_text_effect("item", game_ui_text_fx_item, game_ui_text_fx_item_hover);
    ui_register_text_effect("damage", game_ui_text_fx_damage, game_ui_text_fx_damage_hover);
    ui_register_text_effect("embed_tooltip", game_ui_text_fx_embed_tooltip, game_ui_text_fx_embed_tooltip_hover);
}

void update_game_ui()
{
    ui_begin();
    
    switch (game_ui_peek_state())
    {
        case Game_UI_State_Splash:
        {
            game_ui_do_splash();
            break;
        }
        case Game_UI_State_Main_Menu:
        {
            game_ui_do_main_menu();
            break;
        }
        case Game_UI_State_Options:
        {
            game_ui_do_options();
            break;
        }
        case Game_UI_State_Pause:
        {
            game_ui_do_pause();
            break;
        }
        case Game_UI_State_Play:
        {
            game_ui_do_play();
            break;
        }
    }
    
    if (BIT_IS_SET(s_app->debug_state, Debug_State_Stats))
    {
        game_ui_do_debug_stats();
    }
    
    ui_end();
}

void game_ui_push_state(Game_UI_State state)
{
    cf_array_push(s_app->game_ui.states, state);
}

Game_UI_State game_ui_peek_state()
{
    return cf_array_last(s_app->game_ui.states);
}

Game_UI_State game_ui_pop_state()
{
    Game_UI* game_ui = &s_app->game_ui;
    Game_UI_State state = game_ui_peek_state();
    if (cf_array_count(game_ui->states) > 1)
    {
        cf_array_pop(game_ui->states);
    }
    return state;
}

void game_ui_set_state(Game_UI_State state)
{
    Game_UI* game_ui = &s_app->game_ui;
    cf_array_clear(game_ui->states);
    cf_array_push(game_ui->states, state);
}

void game_ui_push_tooltip_state()
{
    ui_push_layout_background_color(cf_color_black());
    ui_push_layout_border_color(cf_color_grey());
    ui_push_layout_corner_radius(5.0f);
    cf_push_font_size(24.0f);
}

void game_ui_pop_tooltip_state()
{
    cf_pop_font_size();
    ui_pop_layout_corner_radius();
    ui_pop_layout_border_color();
    ui_pop_layout_background_color();
}

void game_ui_tooltip_begin()
{
    game_ui_push_tooltip_state();
    ui_tooltip_begin(cf_v2(250.0f, 0.0f));
}

void game_ui_tooltip_end()
{
    if (game_ui_pin_tooltip_just_pressed())
    {
        game_ui_pin_tooltip();
    }
    
    ui_tooltip_end();
    game_ui_pop_tooltip_state();
}

void game_ui_tooltip(const char* fmt, ...)
{
    UI_Item* item = ui_layout_peek_item();
    if (item)
    {
        BIT_SET(item->state, UI_Item_State_Can_Hover);
    }
    if (ui_item_is_hovered())
    {
        game_ui_tooltip_begin();
        
        va_list args;
        va_start(args, fmt);
        ui_do_item_vfmt(fmt, args);
        va_end(args);
        
        game_ui_tooltip_end();
    }
}

void game_ui_close_all_tooltips()
{
    Game_UI* game_ui = &s_app->game_ui;
    for (s32 index = cf_array_count(game_ui->pin_tooltips) - 1; index >= 0; --index)
    {
        Pin_Tooltip* pin_tooltip = game_ui->pin_tooltips + index;
        game_ui_destroy_pin_tooltip(pin_tooltip);
    }
}

void game_ui_push_tooltip_content(UI_Layout* layout, Layout_Cache* layout_cache)
{
    PROFILE_BEGIN();
    ui_layout_set_direction(layout_cache->direction);
    ui_layout_set_grid_direction(layout_cache->grid_direction);
    ui_layout_set_alignment(layout_cache->alignment);
    ui_layout_set_scrollable(layout_cache->scroll_direction, cf_v2(0, 0));
    if (layout_cache->scroll_direction)
    {
        ui_layout_do_scrollbar(false);
    }
    
    for (s32 index = 0; index < cf_array_count(layout_cache->items); ++index)
    {
        UI_Item item = layout_cache->items[index];
        
        if (item.layout)
        {
            Layout_Cache* child_layout_cache = (Layout_Cache*)item.layout;
            UI_Layout* child_layout = ui_child_layout_begin(cf_extents(item.aabb));
            
            child_layout->background_color = child_layout_cache->background_color;
            child_layout->border_color = child_layout_cache->border_color;
            child_layout->title_color = child_layout_cache->title_color;
            child_layout->border_thickness = child_layout_cache->border_thickness;
            child_layout->corner_radius = child_layout_cache->corner_radius;
            child_layout->title_font_size = child_layout_cache->title_font_size;
            child_layout->item_padding = child_layout_cache->item_padding;
            child_layout->sprite = child_layout_cache->sprite;
            child_layout->sprite_mode = child_layout_cache->sprite_mode;
            
            child_layout->state = child_layout_cache->state;
            game_ui_push_tooltip_content(child_layout, child_layout_cache);
            ui_child_layout_end();
            
            ui_pop_layout_sprite();
            ui_pop_layout_sprite_mode();
        }
        else
        {
            UI_Item* new_item = ui_make_item();
            u64 hash = new_item->hash;
            *new_item = item;
            new_item->hash = hash;
            if (item.text)
            {
                new_item->text = scratch_fmt(item.text);
            }
            if (item.custom_data && item.custom_size)
            {
                new_item->custom_data = scratch_copy(item.custom_data, item.custom_size);
            }
            if (BIT_IS_SET(item.state, UI_Item_State_Scrollbar))
            {
                new_item->state = 0;
            }
        }
    }
    
    PROFILE_END();
}

void game_ui_do_tooltip_content(Pin_Tooltip* pin_tooltip)
{
    PROFILE_BEGIN();
    
    UI_Layout* layout = ui_peek_layout();
    
    Layout_Cache* layout_cache = pin_tooltip->layouts;
    
    for (s32 index = 0; index < cf_array_count(layout_cache->items); ++index)
    {
        UI_Item item = layout_cache->items[index];
        
        if (item.layout)
        {
            Layout_Cache* child_layout_cache = (Layout_Cache*)item.layout;
            
            UI_Layout* child_layout = ui_child_layout_begin(cf_extents(item.aabb));
            
            child_layout->background_color = child_layout_cache->background_color;
            child_layout->border_color = child_layout_cache->border_color;
            child_layout->title_color = child_layout_cache->title_color;
            child_layout->border_thickness = child_layout_cache->border_thickness;
            child_layout->corner_radius = child_layout_cache->corner_radius;
            child_layout->title_font_size = child_layout_cache->title_font_size;
            child_layout->item_padding = child_layout_cache->item_padding;
            child_layout->sprite = child_layout_cache->sprite;
            child_layout->sprite_mode = child_layout_cache->sprite_mode;
            
            child_layout->state = child_layout_cache->state;
            game_ui_push_tooltip_content(child_layout, child_layout_cache);
            ui_child_layout_end();
            
            ui_pop_layout_sprite();
            ui_pop_layout_sprite_mode();
        }
        else
        {
            UI_Item* new_item = ui_make_item();
            u64 hash = new_item->hash;
            *new_item = item;
            new_item->hash = hash;
            if (item.text)
            {
                new_item->text = scratch_fmt(item.text);
            }
            if (item.custom_data && item.custom_size)
            {
                new_item->custom_data = scratch_copy(item.custom_data, item.custom_size);
            }
        }
    }
    
    PROFILE_END();
}

void game_ui_do_tooltips()
{
    World* world = &s_app->world;
    UI* ui = &s_app->ui;
    Game_UI* game_ui = &s_app->game_ui;
    CF_V2 mouse_position = ui->input.mouse_position;
    
    // all pin tooltips
    b32 show_hover_entity_tooltip = true;
    s32 move_to_front = -1;
    for (s32 index = 0; index < cf_array_count(game_ui->pin_tooltips); ++index)
    {
        Pin_Tooltip* pin_tooltip = game_ui->pin_tooltips + index;
        
        game_ui_push_tooltip_state();
        cf_push_font_size(24.0f);
        
        // tooltip content
        Layout_Cache* layout_cache = pin_tooltip->layouts;
        cf_push_font_size(layout_cache->title_font_size);
        UI_Layout* layout = ui_layout_begin(scratch_fmt("pin_tooltip_%d", pin_tooltip->id));
        cf_pop_font_size();
        if (layout_cache->title)
        {
            ui_layout_set_title(layout_cache->title);
        }
        ui_layout_set_aabb(layout_cache->aabb);
        ui_layout_set_direction(layout_cache->direction);
        ui_layout_set_grid_direction(layout_cache->grid_direction);
        ui_layout_set_alignment(layout_cache->alignment);
        ui_layout_set_scrollable(layout_cache->scroll_direction, cf_v2(0, 0));
        BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_X);
        BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
        pin_tooltip->do_close |= ui_layout_do_close_button();
        
        game_ui_do_tooltip_content(pin_tooltip);
        
        if (ui_layout_is_hovering(layout))
        {
            if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT))
            {
                move_to_front = index;
            }
        }
        
        // handle window dragging
        if (ui_layout_is_down(layout))
        {
            CF_V2 drag_offset = cf_sub(mouse_position, ui->down_root_layout_offset);
            drag_offset = cf_sub(drag_offset, ui_layout_get_anchor(layout));
            
            if (BIT_IS_SET(layout_cache->alignment, UI_Layout_Alignment_Vertical_Top))
            {
                drag_offset.y += layout->item_padding;
            }
            else if (BIT_IS_SET(layout_cache->alignment, UI_Layout_Alignment_Vertical_Bottom))
            {
                drag_offset.y -= layout->item_padding;
            }
            if (BIT_IS_SET(layout_cache->alignment, UI_Layout_Alignment_Horizontal_Left))
            {
                drag_offset.x -= layout->item_padding;
            }
            else if (BIT_IS_SET(layout_cache->alignment, UI_Layout_Alignment_Horizontal_Right))
            {
                drag_offset.x += layout->item_padding;
            }
            
            layout_cache->aabb = move_aabb(layout_cache->aabb, drag_offset);
            // update aabb again here to avoid having a slugglish drag
            ui_layout_set_aabb(layout_cache->aabb);
        }
        
        ui_layout_end();
        cf_pop_font_size();
        game_ui_pop_tooltip_state();
    }
    
    // move current tooltip to be drawn last / be put in front
    if (move_to_front >= 0)
    {
        Pin_Tooltip temp = game_ui->pin_tooltips[move_to_front];
        void* dst = game_ui->pin_tooltips + move_to_front;
        void* src = game_ui->pin_tooltips + move_to_front + 1;
        s32 copy_count = cf_array_count(game_ui->pin_tooltips) - move_to_front - 1;
        CF_MEMCPY(dst, src, sizeof(*game_ui->pin_tooltips) * copy_count);
        cf_array_last(game_ui->pin_tooltips) = temp;
    }
    
    // remove any closed pin tooltips
    for (s32 index = cf_array_count(game_ui->pin_tooltips) - 1; index >= 0; --index)
    {
        Pin_Tooltip* pin_tooltip = game_ui->pin_tooltips + index;
        if (pin_tooltip->do_close)
        {
            game_ui_destroy_pin_tooltip(pin_tooltip);
        }
    }
    
    // don't show tooltip if all ready hovering another layout
    if (ui->hover_layout_name)
    {
        world->hover_duration = 0.0f;
    }
    
    // show current entity tooltip info
    if (show_hover_entity_tooltip && 
        ENTITY_IS_VALID(world->hover_entity) && world->hover_duration > 0.5f)
    {
        ecs_entity_t entity = world->hover_entity;
        f32 t = cf_clamp01((world->hover_duration - 0.5f) / 0.1f);
        
        ui_push_layout_background_color(cf_color_black());
        game_ui_tooltip_begin();
        ui_layout_set_expand_animation_t(cf_v2(1.0f, t));
        
        const char* creature_name = "Details";
        s32 health_value = 0;
        CF_MAP(Attributes) map_attributes = NULL;
        Attributes attributes = { 0 };
        
        fixed Condition_Effect* effects = NULL;
        
        if (ECS_HAS(entity, C_Health))
        {
            C_Health* health = ECS_GET(entity, C_Health);
            health_value = *health;
        }
        if (ECS_HAS(entity, C_Creature))
        {
            C_Creature* creature = ECS_GET(entity, C_Creature);
            creature_name = creature->name;
            attributes = accumulate_attributes(creature->attributes);
            map_attributes = creature->attributes;
        }
        if (ECS_HAS(entity, C_Condition))
        {
            C_Condition* condition = ECS_GET(entity, C_Condition);
            effects = get_condition_effects(condition->handle);
        }
        
        ui_layout_set_title(creature_name);
        
        {
            ui_push_layout_border_color(cf_color_clear());
            ui_push_layout_background_color(cf_color_clear());
            
            UI_Layout* attributes_layout = ui_child_layout_begin(cf_v2(100.0f, 0));
            BIT_SET(attributes_layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
            
            ui_do_text("Health");
            ui_do_text("Strength");
            ui_do_text("Agility");
            ui_do_text("Speed");
            ui_child_layout_end();
            
            fixed char* health_tooltip = game_ui_attributes_health_tooltip(map_attributes);
            fixed char* strength_tooltip = game_ui_attributes_strength_tooltip(map_attributes);
            fixed char* agility_tooltip = game_ui_attributes_agility_tooltip(map_attributes);
            fixed char* move_speed_tooltip = game_ui_attributes_move_speed_tooltip(map_attributes);
            
            ui_do_same_line();
            
            attributes_layout = ui_child_layout_begin(cf_v2(100.0f, 0));
            BIT_SET(attributes_layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
            
            ui_do_text("<embed_tooltip title=\"Health\" text=\"%s\">%d/%d</embed_tooltip>", health_tooltip, health_value, attributes.health);
            ui_do_text("<embed_tooltip title=\"Strength\" text=\"%s\">%d</embed_tooltip>", strength_tooltip, attributes.strength);
            ui_do_text("<embed_tooltip title=\"Agility\" text=\"%s\">%d</embed_tooltip>", agility_tooltip, attributes.agility);
            ui_do_text("<embed_tooltip title=\"Speed\" text=\"%s\">%.2f</embed_tooltip>", move_speed_tooltip, attributes.move_speed);
            
            ui_child_layout_end();
            
            ui_pop_layout_border_color(cf_color_clear());
            ui_pop_layout_background_color();
        }
        
        for (s32 index = 0; index < cf_array_count(effects); ++index)
        {
            const char* text = scratch_fmt("<condition type=%d>%s</condition>", effects[index].type, effects[index].name);
            ui_do_sprite_ex(get_condition_sprite(effects[index].type), cf_v2(cf_peek_font_size(), cf_peek_font_size()));
            ui_do_same_line();
            ui_do_text(text);
        }
        
        ui_pop_layout_background_color();
        game_ui_tooltip_end();
    }
}

b32 game_ui_pin_tooltip_just_pressed()
{
    UI* ui = &s_app->ui;
    b32 pressed = false;
    if (cf_key_ctrl() && cf_button_binding_pressed(ui->input.select))
    {
        pressed = true;
        cf_button_binding_consume_press(ui->input.select);
    }
    return pressed;
}

void game_ui_creature_tooltip(Body_Type body_type)
{
    UI_Item* item = ui_layout_peek_item();
    if (item)
    {
        BIT_SET(item->state, UI_Item_State_Can_Hover);
    }
    if (ui_item_is_hovered())
    {
        game_ui_tooltip_begin();
        ui_layout_set_title("Creature");
        ui_do_text(Body_Type_names[body_type]);
        game_ui_tooltip_end();
    }
}

void game_ui_spell_tooltip(Spell_Type spell_type)
{
    UI_Item* item = ui_layout_peek_item();
    if (item)
    {
        BIT_SET(item->state, UI_Item_State_Can_Hover);
    }
    if (ui_item_is_hovered())
    {
        game_ui_tooltip_begin();
        Spell_Data spell = get_spell_data(spell_type);
        ui_layout_set_title(get_spell_name(spell_type));
        ui_do_text(get_spell_description(spell_type));
        ui_do_text("Target: %s", Cast_Target_Type_names[spell.target_type]);
        game_ui_tooltip_end();
    }
}

void game_ui_item_tooltip(Item_Type item_type)
{
    UI_Item* item = ui_layout_peek_item();
    if (item)
    {
        BIT_SET(item->state, UI_Item_State_Can_Hover);
    }
    if (ui_item_is_hovered())
    {
        game_ui_tooltip_begin();
        ui_layout_set_title(get_item_name(item_type));
        ui_do_text(get_item_description(item_type));
        game_ui_tooltip_end();
    }
}

b32 game_ui_do_button(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    b32 clicked = ui_do_button_vfmt(fmt, args);
    va_end(args);
    
    if (clicked)
    {
        audio_play_ui("sounds/ui_button.wav");
    }
    
    return clicked;
}

b32 game_ui_do_slider(f32 *value, f32 min, f32 max, f32 rate)
{
    b32 changed = ui_do_slider(value, min, max, rate);
    
    if (changed)
    {
        audio_play_ui("sounds/ui_slider.wav");
    }
    
    return changed;
}

void game_ui_clear_logs()
{
    Game_UI* game_ui = &s_app->game_ui;
    node_pool_reset(&game_ui->log_pool);
}

void game_ui_do_logs()
{
    Game_UI* game_ui = &s_app->game_ui;
    
    CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(UI_WIDTH * 0.5f, UI_HEIGHT), UI_WIDTH * 0.5f, UI_HEIGHT * 0.2f);
    
    CF_Color layout_background_color = cf_color_black();
    layout_background_color.a = 0.5f;
    
    ui_push_layout_background_color(layout_background_color);
    ui_layout_begin("Logs");
    ui_pop_layout_background_color();
    ui_layout_set_aabb(layout_aabb);
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Bottom) | BIT(UI_Layout_Alignment_Horizontal_Left));
    ui_layout_set_direction(UI_Layout_Direction_Up);
    ui_push_item_alignment(UI_Item_Alignment_Left);
    ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0.0f, 1.0f));
    ui_layout_do_scrollbar(true);
    
    ui_layout_expand_usable_aabb(cf_v2(-20, -20));
    cf_push_font_size(24.0f);
    
    FOREACH_LIST_REVERSE(node, &game_ui->log_pool.active_list)
    {
        Game_Log* log = CF_LIST_HOST(Game_Log, node, node);
        ui_do_text(log->text);
    }
    
    cf_pop_font_size();
    
    ui_layout_end();
}

typedef struct Inventory_Button_Data
{
    CF_Sprite sprite;
    s32 count;
} Inventory_Button_Data;

void game_ui_draw_inventory_button(UI_Layout* layout, UI_Item* item)
{
    Inventory_Button_Data* data = (Inventory_Button_Data*)item->custom_data;
    item->custom_data = &data->sprite;
    ui_draw_sprite(layout, item);
    
    char buffer[8];
    CF_SNPRINTF(buffer, sizeof(buffer), "%d", data->count);
    
    cf_push_font_size(item->font_size);
    
    CF_V2 text_size = cf_text_size(buffer, -1);
    CF_V2 text_position = cf_bottom_right(item->aabb);
    text_position.x -= text_size.x;
    text_position.y += text_size.y * 0.75f;
    
    cf_draw_push_color(item->text_shadow_color);
    cf_draw_text(buffer, cf_add(text_position, cf_v2(1, -1)), -1);
    cf_draw_pop_color();
    
    cf_draw_push_color(item->text_color);
    cf_draw_text(buffer, text_position, -1);
    cf_draw_pop_color();
    
    cf_pop_font_size();
}

b32 game_ui_inventory_button(CF_Sprite sprite, s32 count)
{
    b32 clicked = ui_do_sprite_button(sprite);
    
    Inventory_Button_Data* data = scratch_alloc(sizeof(Inventory_Button_Data));
    data->sprite = sprite;
    data->count = count;
    
    UI_Item* item = ui_layout_peek_item();
    item->custom_draw = game_ui_draw_inventory_button;
    item->custom_data = data;
    item->custom_size = sizeof(Inventory_Button_Data);
    
    if (clicked)
    {
        audio_play_ui("sounds/ui_button.wav");
    }
    
    return clicked;
}

void game_ui_do_creature_hud()
{
    World* world = &s_app->world;
    Inventory* inventory = &world->inventory;
    b32 is_waiting_coroutine = COROUTINE_IS_RUNNING(world->state_transition_co);
    
    s32 player_creature_deployments = 0;
    {
        if (cf_map_has(world->team_creatures, TOY_PLAYER_INDEX))
        {
            dyna ecs_entity_t* team_creatures = cf_map_get(world->team_creatures, TOY_PLAYER_INDEX);
            player_creature_deployments = cf_array_count(team_creatures);
        }
    }
    
    if (is_waiting_coroutine)
    {
        ui_push_is_disabled(true);
    }
    
    // body selection
    {
        Body_Type available_types[] =
        {
            Body_Type_Human,
            Body_Type_Tentacle,
            Body_Type_Slime,
        };
        
        CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT), UI_WIDTH * 0.5f, UI_HEIGHT * 0.2f);
        
        ui_push_layout_background_color(menu_layout_background_color());
        ui_layout_begin("Creature Hud");
        ui_pop_layout_background_color();
        ui_layout_set_aabb(layout_aabb);
        ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Top) | BIT(UI_Layout_Alignment_Horizontal_Left));
        ui_layout_set_direction(UI_Layout_Direction_Right);
        ui_layout_set_grid_direction(UI_Layout_Direction_Down);
        ui_push_item_alignment(UI_Item_Alignment_Left);
        ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
        ui_layout_do_scrollbar(true);
        
        ui_push_corner_radius(3.0f);
        ui_push_border_thickness(1.0f);
        
        ui_layout_expand_usable_aabb(cf_v2(-20, -20));
        
        for (s32 index = 0; index < CF_ARRAY_SIZE(available_types); ++index)
        {
            Body_Type type = available_types[index];
            
            if (inventory->bodies[type] == 0)
            {
                continue;
            }
            
            if (game_ui_inventory_button(get_body_sprite(type), inventory->bodies[type]))
            {
                world->placement_type = type;
            }
            game_ui_creature_tooltip(type);
        }
        
        ui_pop_corner_radius();
        ui_pop_border_thickness();
        
        ui_layout_end();
    }
    
    // controls to start
    {
        CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(UI_WIDTH * 0.9f, UI_HEIGHT * 0.1f), UI_WIDTH * 0.1f, UI_HEIGHT * 0.1f);
        
        ui_layout_begin("Creature Control Hud");
        
        ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Bottom) | BIT(UI_Layout_Alignment_Horizontal_Right));
        ui_layout_set_direction(UI_Layout_Direction_Up);
        ui_push_item_alignment(UI_Item_Alignment_Right);
        
        ui_layout_set_aabb(layout_aabb);
        
        if (player_creature_deployments == 0)
        {
            ui_push_is_disabled(true);
        }
        if (game_ui_do_button("Start"))
        {
            make_event_world_arena_start();
        }
        if (player_creature_deployments == 0)
        {
            ui_pop_is_disabled();
        }
        
        ui_pop_item_alignment();
        ui_layout_end();
    }
    
    if (is_waiting_coroutine)
    {
        ui_pop_is_disabled();
    }
}

void game_ui_do_spell_hud()
{
    Inventory* inventory = &s_app->world.inventory;
    
    CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT), UI_WIDTH * 0.5f, UI_HEIGHT * 0.2f);
    
    ui_push_layout_background_color(menu_layout_background_color());
    ui_layout_begin("Spell Hud");
    ui_pop_layout_background_color();
    ui_layout_set_aabb(layout_aabb);
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Top) | BIT(UI_Layout_Alignment_Horizontal_Left));
    ui_layout_set_direction(UI_Layout_Direction_Right);
    ui_layout_set_grid_direction(UI_Layout_Direction_Down);
    ui_push_item_alignment(UI_Item_Alignment_Left);
    ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
    ui_layout_do_scrollbar(true);
    
    ui_push_corner_radius(3.0f);
    ui_push_border_thickness(1.0f);
    
    ui_layout_expand_usable_aabb(cf_v2(-20, -20));
    
    for (s32 index = Spell_Type_None + 1; index < Spell_Type_Count; ++index)
    {
        Spell_Type spell_type = (Spell_Type)index;
        if (inventory->spells[index] == 0)
        {
            continue;
        }
        
        if (game_ui_inventory_button(get_spell_sprite(spell_type), inventory->spells[spell_type]))
        {
            make_event_hand_caster_select_spell(spell_type);
        }
        game_ui_spell_tooltip(spell_type);
    }
    
    ui_pop_corner_radius();
    ui_pop_border_thickness();
    
    ui_layout_end();
}

void game_ui_do_arena_placement()
{
    Game_UI* game_ui = &s_app->game_ui;
    
    game_ui_do_creature_hud();
    game_ui_do_logs();
}

void game_ui_do_arena_in_progress()
{
    Game_UI* game_ui = &s_app->game_ui;
    
    game_ui_do_spell_hud();
    game_ui_do_logs();
}

void game_ui_do_arena_result()
{
    Game_UI* game_ui = &s_app->game_ui;
    World* world = &s_app->world;
    Inventory* inventory  = &world->inventory;
    
    Body_Type body_types[] = 
    {
        Body_Type_Human,
        Body_Type_Tentacle,
        Body_Type_Slime,
    };
    
    s32 body_counts[] = 
    {
        0,
        0,
        0,
    };
    
    dyna ecs_entity_t* collectable_creatures = NULL;
    if (cf_map_has(world->team_creatures, TOY_PLAYER_INDEX))
    {
        collectable_creatures = cf_map_get(world->team_creatures, TOY_PLAYER_INDEX);
    }
    
    s32 available_creature_count = cf_array_count(collectable_creatures);
    for (s32 index = 0; index < cf_array_size(inventory->bodies); ++index)
    {
        available_creature_count += inventory->bodies[index];
    }
    
    game_ui_do_spell_hud();
    game_ui_do_logs();
    
    // arena results
    {
        CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(UI_WIDTH  * 0.25f, UI_HEIGHT * 0.75f), UI_WIDTH * 0.5f, UI_HEIGHT * 0.5f);
        CF_V2 layout_extents = cf_extents(layout_aabb);
        
        ui_push_layout_corner_radius(10.0f);
        ui_push_layout_background_color(menu_layout_background_color());
        UI_Layout* layout = ui_layout_begin("Arena End");
        BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_X);
        BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
        ui_pop_layout_background_color();
        ui_pop_layout_corner_radius();
        
        ui_layout_set_aabb(layout_aabb);
        
        if (cf_array_count(collectable_creatures) > 0)
        {
            ui_layout_set_title("Room Cleared");
        }
        else
        {
            ui_layout_set_title("Room Failed");
        }
        
        // list of collectable creatures
        {
            ui_child_layout_begin(layout_extents);
            ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
            ui_layout_do_scrollbar(true);
            
            for (s32 index = 0; index < cf_array_count(collectable_creatures); ++index)
            {
                C_Puppet* puppet = ECS_GET(collectable_creatures[index], C_Puppet);
                Body_Type body_type = puppet->body.type;
                
                for (s32 body_index = 0; body_index < CF_ARRAY_SIZE(body_types); ++body_index)
                {
                    if (body_type == body_types[body_index])
                    {
                        ++body_counts[body_index];
                        break;
                    }
                }
            }
            
            for (s32 body_index = 0; body_index < CF_ARRAY_SIZE(body_types); ++body_index)
            {
                Body_Type body_type = body_types[body_index];
                if (body_counts[body_index])
                {
                    cf_push_font_size(36.0f);
                    ui_do_text("%s: %d", Body_Type_names[body_type], body_counts[body_index]);
                    cf_pop_font_size();
                    
                    cf_push_font_size(24.0f);
                    for (s32 index = 0; index < cf_array_count(collectable_creatures); ++index)
                    {
                        C_Puppet* puppet = ECS_GET(collectable_creatures[index], C_Puppet);
                        C_Creature* creature = ECS_GET(collectable_creatures[index], C_Creature);
                        if (body_type == puppet->body.type)
                        {
                            ui_do_text(creature->name);
                        }
                    }
                    cf_pop_font_size();
                }
            }
            
            ui_child_layout_end();
        }
        
        if (available_creature_count > 0)
        {
            b32 advance = false;
            
            // pick next room
            if (overworld_can_advance())
            {
                advance = game_ui_do_button("Map");
            }
            else if (overworld_is_level_finished())
            {
                // generate new level
                advance = game_ui_do_button("Next Level");
                overworld_generate();
            }
            
            if (advance)
            {
                // collect remaining bodies
                for (s32 body_index = 0; body_index < CF_ARRAY_SIZE(body_types); ++body_index)
                {
                    if (body_counts[body_index] > 0)
                    {
                        inventory_add_body(inventory, body_types[body_index], body_counts[body_index]);
                    }
                }
                game_ui_clear_logs();
                world_set_state(World_State_Overworld);
            }
            
            Overworld_Room* room = overworld_get_current_room();
            if (room)
            {
                ui_do_same_line();
                CF_V2 gold_icon_size = cf_v2(cf_peek_font_size(), cf_peek_font_size());
                ui_do_sprite_ex(assets_get_sprite("sprites/dollar.ase"), gold_icon_size);
                ui_do_same_line();
                ui_do_text("%d", room->gold);
            }
        }
        else
        {
            if (game_ui_do_button("Main Menu"))
            {
                game_ui_clear_logs();
                game_ui_set_state(Game_UI_State_Main_Menu);
            }
        }
        
        ui_pop_item_alignment();
        ui_layout_end();
    }
}

typedef struct Shop_Button_Params
{
    CF_Sprite sprite;
    s32 count;
    b32 is_spell;
    b32 is_item;
    b32 show_cost;
    CF_V2 extents;
    union
    {
        Body_Type body_type;
        Spell_Type spell_type;
        Item_Type item_type;
    };
} Shop_Button_Params;

// yes this is an overloaded fucntion that should be trimmed down or use a params struct
b32 game_ui_do_shop_button(Shop_Button_Params params)
{
    Overworld_Shop* shop = &s_app->overworld.shop;
    Inventory* inventory = &s_app->world.inventory;
    
    cf_push_font_size(24.0f);
    UI_Layout* layout = ui_child_layout_begin(params.extents);
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Top) | BIT(UI_Layout_Alignment_Horizontal_Center));
    ui_layout_set_direction(UI_Layout_Direction_Down);
    BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
    
    if (params.is_spell)
    {
        ui_layout_set_title(get_spell_name(params.spell_type));
    }
    else if (params.is_item)
    {
        ui_layout_set_title(get_item_name(params.item_type));
    }
    else
    {
        ui_layout_set_title(Body_Type_names[params.body_type]);
    }
    cf_pop_font_size();
    
    ui_push_item_alignment(UI_Item_Alignment_Center);
    
    b32 buy = false;
    s32 gold_cost = 0;
    
    if (params.is_spell)
    {
        gold_cost = overworld_shop_get_spell_cost(params.spell_type);
        if (game_ui_inventory_button(params.sprite, params.count))
        {
            if (inventory_remove_gold(inventory, gold_cost) && overworld_buy_spell(shop, params.spell_type))
            {
                inventory_add_spell(inventory, params.spell_type, 1);
                buy = true;
            }
            audio_play_ui("sounds/ui_button.wav");
        }
        game_ui_spell_tooltip(params.spell_type);
    }
    else if (params.is_item)
    {
        gold_cost = overworld_shop_get_item_cost(params.item_type);
        if (game_ui_inventory_button(params.sprite, params.count))
        {
            if (inventory_remove_gold(inventory, gold_cost) && overworld_buy_item(shop, params.item_type))
            {
                inventory_add_item(inventory, params.item_type, 1);
                buy = true;
            }
            audio_play_ui("sounds/ui_button.wav");
        }
        game_ui_item_tooltip(params.item_type);
    }
    else
    {
        gold_cost = overworld_shop_get_body_cost(params.body_type);
        if (game_ui_inventory_button(params.sprite, params.count))
        {
            if (inventory_remove_gold(inventory, gold_cost) && overworld_buy_body(shop, params.body_type))
            {
                inventory_add_body(inventory, params.body_type, 1);
            }
            audio_play_ui("sounds/ui_button.wav");
        }
        game_ui_creature_tooltip(params.body_type);
    }
    
    if (params.show_cost)
    {
        ui_do_text("%d", gold_cost);
    }
    
    ui_pop_item_alignment();
    
    ui_child_layout_end();
    
    return buy;
}

void game_ui_do_shop()
{
    World* world = &s_app->world;
    Overworld_Shop* shop =  &s_app->overworld.shop;
    Inventory* inventory = &world->inventory;
    
    Body_Type body_types[] = 
    {
        Body_Type_Human,
        Body_Type_Tentacle,
        Body_Type_Slime,
    };
    
    {
        ui_push_layout_border_color(cf_color_white());
        ui_push_layout_border_thickness(1.0f);
        ui_push_layout_corner_radius(3.0f);
        
        CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(UI_WIDTH  * 0.1f, UI_HEIGHT * 0.9f), UI_WIDTH * 0.8f, UI_HEIGHT * 0.8f);
        CF_V2 layout_extents = cf_extents(layout_aabb);
        
        ui_push_layout_corner_radius(10.0f);
        ui_push_layout_background_color(menu_layout_background_color());
        UI_Layout* layout = ui_layout_begin("Shop");
        ui_pop_layout_background_color();
        ui_pop_layout_corner_radius();
        
        ui_layout_set_aabb(layout_aabb);
        
        CF_V2 section_extents = cf_v2(layout_extents.x * 0.5f, layout_extents.y * 0.4f);
        CF_V2 shop_item_extents = cf_v2(section_extents.x * 0.2f, layout_extents.y * 0.3f);
        
        // shop
        // bodies
        {
            ui_child_layout_begin(section_extents);
            ui_layout_set_title("Creatures");
            ui_layout_set_direction(UI_Layout_Direction_Right);
            ui_layout_set_grid_direction(UI_Layout_Direction_Down);
            ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
            ui_layout_do_scrollbar(true);
            
            for (s32 index = 0; index < cf_array_count(shop->bodies); ++index)
            {
                Body_Type body_type = (Body_Type)index;
                if (shop->bodies[index] > 0)
                {
                    s32 gold_cost = overworld_shop_get_body_cost(body_type);
                    b32 can_afford = inventory_has_gold(inventory, gold_cost);
                    if (!can_afford)
                    {
                        ui_push_is_disabled(true);
                    }
                    
                    Shop_Button_Params params = { 0 };
                    params.sprite = get_body_sprite(body_type);
                    params.count = shop->bodies[body_type];
                    params.is_spell = false;
                    params.show_cost = true;
                    params.extents = shop_item_extents;
                    params.body_type = body_type;
                    
                    game_ui_do_shop_button(params);
                    
                    if (!can_afford)
                    {
                        ui_pop_is_disabled();
                    }
                    
                }
            }
            
            ui_child_layout_end();
        }
        // spells
        {
            ui_do_same_line();
            ui_child_layout_begin(cf_v2(section_extents.x - layout->item_padding * 2, section_extents.y));
            ui_layout_set_title("Spells");
            ui_layout_set_direction(UI_Layout_Direction_Right);
            ui_layout_set_grid_direction(UI_Layout_Direction_Down);
            ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
            ui_layout_do_scrollbar(true);
            
            for (s32 index = 0; index < cf_array_count(shop->spells); ++index)
            {
                Spell_Type spell_type = (Spell_Type)index;
                if (shop->spells[index] > 0)
                {
                    s32 gold_cost = overworld_shop_get_spell_cost(spell_type);
                    b32 can_afford = inventory_has_gold(inventory, gold_cost);
                    if (!can_afford)
                    {
                        ui_push_is_disabled(true);
                    }
                    
                    Shop_Button_Params params = { 0 };
                    params.sprite = get_spell_sprite(spell_type);
                    params.count = shop->spells[spell_type];
                    params.is_spell = true;
                    params.show_cost = true;
                    params.extents = shop_item_extents;
                    params.spell_type = spell_type;
                    
                    game_ui_do_shop_button(params);
                    
                    if (!can_afford)
                    {
                        ui_pop_is_disabled();
                    }
                }
            }
            ui_child_layout_end();
        }
        // items
        {
            ui_child_layout_begin(cf_v2(section_extents.x * 2, section_extents.y));
            ui_layout_set_title("Items");
            ui_layout_set_direction(UI_Layout_Direction_Right);
            ui_layout_set_grid_direction(UI_Layout_Direction_Down);
            ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
            ui_layout_do_scrollbar(true);
            
            for (s32 index = 0; index < cf_array_count(shop->items); ++index)
            {
                if (shop->items[index] <= 0)
                {
                    continue;
                }
                
                Item_Type item_type = (Item_Type)index;
                s32 gold_cost = overworld_shop_get_item_cost(item_type);
                b32 can_afford = inventory_has_gold(inventory, gold_cost);
                if (!can_afford)
                {
                    ui_push_is_disabled(true);
                }
                
                Shop_Button_Params params = { 0 };
                params.sprite = get_item_sprite(item_type);
                params.count = shop->items[index];
                params.is_item = true;
                params.show_cost = true;
                params.extents = shop_item_extents;
                params.item_type = item_type;
                
                game_ui_do_shop_button(params);
                
                if (!can_afford)
                {
                    ui_pop_is_disabled();
                }
            }
            
            ui_child_layout_end();
        }
        
        ui_pop_layout_border_color();
        ui_pop_layout_border_thickness();
        ui_pop_layout_corner_radius();
        
        // footer
        CF_V2 footer_extents = cf_v2(section_extents.x, UI_HEIGHT * 0.2f);
        // shop control flow
        {
            ui_child_layout_begin(footer_extents);
            ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Left));
            ui_layout_set_direction(UI_Layout_Direction_Down);
            ui_layout_set_grid_direction(UI_Layout_Direction_Down);
            
            if (game_ui_do_button("Map"))
            {
                world_set_state(World_State_Overworld);
            }
            
            ui_child_layout_end();
        }
        // currency
        {
            ui_do_same_line();
            ui_child_layout_begin(cf_v2(footer_extents.x - layout->item_padding, footer_extents.y));
            ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Right));
            ui_layout_set_direction(UI_Layout_Direction_Left);
            ui_layout_set_grid_direction(UI_Layout_Direction_Down);
            ui_push_item_alignment(UI_Item_Alignment_Right);
            
            CF_V2 gold_icon_size = cf_v2(cf_peek_font_size(), cf_peek_font_size());
            ui_do_text("%d", inventory->gold);
            ui_do_sprite_ex(assets_get_sprite("sprites/dollar.ase"), gold_icon_size);
            
            ui_pop_item_alignment();
            ui_child_layout_end();
        }
        
        
        ui_pop_item_alignment();
        ui_layout_end();
    }
}

void game_ui_do_inventory()
{
    Inventory* inventory = &s_app->world.inventory;
    
    ui_push_layout_border_color(cf_color_white());
    ui_push_layout_border_thickness(1.0f);
    ui_push_layout_corner_radius(3.0f);
    
    CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(UI_WIDTH  * 0.1f, UI_HEIGHT * 0.9f), UI_WIDTH * 0.8f, UI_HEIGHT * 0.8f);
    CF_V2 layout_extents = cf_extents(layout_aabb);
    
    ui_push_layout_corner_radius(10.0f);
    ui_push_layout_background_color(cf_color_black());
    UI_Layout* layout = ui_layout_begin("Inventory");
    ui_pop_layout_background_color();
    ui_pop_layout_corner_radius();
    
    ui_layout_set_aabb(layout_aabb);
    
    CF_V2 section_extents = cf_v2(layout_extents.x * 0.5f, layout_extents.y * 0.4f);
    CF_V2 sub_section_extents = cf_v2(section_extents.x * 0.2f, layout_extents.y * 0.3f);
    
    ui_push_is_disabled(true);
    // bodies
    {
        ui_child_layout_begin(section_extents);
        ui_layout_set_title("Creatures");
        ui_layout_set_direction(UI_Layout_Direction_Right);
        ui_layout_set_grid_direction(UI_Layout_Direction_Down);
        ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
        ui_layout_do_scrollbar(true);
        
        for (s32 index = 0; index < cf_array_count(inventory->bodies); ++index)
        {
            if (inventory->bodies[index] > 0)
            {
                Shop_Button_Params params = { 0 };
                params.sprite = get_body_sprite((Body_Type)index);
                params.count = inventory->bodies[index];
                params.is_spell = false;
                params.show_cost = false;
                params.extents = sub_section_extents;
                params.spell_type = (Body_Type)index;
                
                game_ui_do_shop_button(params);
            }
        }
        
        ui_child_layout_end();
    }
    // spells
    {
        ui_do_same_line();
        ui_child_layout_begin(cf_v2(section_extents.x - layout->item_padding * 2, section_extents.y));
        ui_layout_set_title("Spells");
        ui_layout_set_direction(UI_Layout_Direction_Right);
        ui_layout_set_grid_direction(UI_Layout_Direction_Down);
        ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
        ui_layout_do_scrollbar(true);
        
        for (s32 index = 0; index < cf_array_count(inventory->spells); ++index)
        {
            if (inventory->spells[index] > 0)
            {
                Shop_Button_Params params = { 0 };
                params.sprite = get_spell_sprite((Spell_Type)index);
                params.count = inventory->spells[index];
                params.is_spell = true;
                params.show_cost = false;
                params.extents = sub_section_extents;
                params.spell_type = (Spell_Type)index;
                
                game_ui_do_shop_button(params);
            }
        }
        
        ui_child_layout_end();
    }
    
    // items
    {
        ui_child_layout_begin(cf_v2(section_extents.x * 2.0f, section_extents.y));
        ui_layout_set_title("Items");
        ui_layout_set_direction(UI_Layout_Direction_Right);
        ui_layout_set_grid_direction(UI_Layout_Direction_Down);
        ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
        ui_layout_do_scrollbar(true);
        
        for (s32 index = 0; index < cf_array_count(inventory->items); ++index)
        {
            if (inventory->items[index] <= 0)
            {
                continue;
            }
            
            
            Item_Type item_type = (Item_Type)index;
            
            Shop_Button_Params params = { 0 };
            params.sprite = get_item_sprite(item_type);
            params.count = inventory->items[index];
            params.is_item = true;
            params.show_cost = false;
            params.extents = sub_section_extents;
            params.item_type = item_type;
            
            game_ui_do_shop_button(params);
        }
        
        ui_child_layout_end();
    }
    
    ui_pop_is_disabled();
    
    ui_pop_layout_border_color();
    ui_pop_layout_border_thickness();
    ui_pop_layout_corner_radius();
    
    // footer
    CF_V2 footer_extents = cf_v2(section_extents.x, UI_HEIGHT * 0.2f);
    // shop control flow
    {
        ui_child_layout_begin(footer_extents);
        ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Left));
        ui_layout_set_direction(UI_Layout_Direction_Down);
        ui_layout_set_grid_direction(UI_Layout_Direction_Down);
        
        if (game_ui_do_button("Close"))
        {
            s_app->game_ui.show_inventory = false;
        }
        
        ui_child_layout_end();
    }
    // currency
    {
        ui_do_same_line();
        ui_child_layout_begin(cf_v2(footer_extents.x - layout->item_padding, footer_extents.y));
        ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Right));
        ui_layout_set_direction(UI_Layout_Direction_Left);
        ui_layout_set_grid_direction(UI_Layout_Direction_Down);
        ui_push_item_alignment(UI_Item_Alignment_Right);
        
        CF_V2 gold_icon_size = cf_v2(cf_peek_font_size(), cf_peek_font_size());
        ui_do_text("%d", inventory->gold);
        ui_do_sprite_ex(assets_get_sprite("sprites/dollar.ase"), gold_icon_size);
        
        ui_pop_item_alignment();
        ui_child_layout_end();
    }
    
    
    ui_pop_item_alignment();
    ui_layout_end();
}

Game_Log* make_game_log()
{
    Game_UI* game_ui = &s_app->game_ui;
    Game_Log* log = NULL;
    CF_ListNode* node = node_pool_alloc(&game_ui->log_pool);
    if (!node)
    {
        node = cf_list_pop_front(&game_ui->log_pool.active_list);
        cf_list_push_back(&game_ui->log_pool.active_list, node);
    }
    log = CF_LIST_HOST(Game_Log, node, node);
    
    log->text = NULL;
    log->damage_sources = NULL;
    cf_arena_reset(&log->arena);
    
    return log;
}

void game_ui_log_damage(const char* attacker, const char* victim, fixed Damage_Source* sources)
{
    Game_UI* game_ui = &s_app->game_ui;
    Game_Log* log = make_game_log();
    
    s32 damage = accumulate_damage_sources(sources);
    
    s32 log_id = (s32)(log - game_ui->logs);
    log->text = arena_fmt(&log->arena, "%s hit %s for <damage log_id=%d>%d</damage>", attacker, victim, log_id, damage);
    
    MAKE_ARENA_ARRAY(&log->arena, log->damage_sources, cf_array_count(sources));
    CF_MEMCPY(log->damage_sources, sources, sizeof(*sources) * cf_array_count(sources));
    cf_array_setlen(log->damage_sources, cf_array_count(sources));
}

void game_ui_log_died(const char* victim)
{
    Game_UI* game_ui = &s_app->game_ui;
    Game_Log* log = make_game_log();
    log->text = arena_fmt(&log->arena, "%s has died", victim);
}

void game_ui_log_condition_gained(const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "%s gained <condition type=%d>%s</condition>", victim, effect, condition_name);
}

void game_ui_log_condition_loss(const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();;
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "%s loss <condition type=%d>%s</condition>", victim, effect, condition_name);
}

void game_ui_log_condition_expired(const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();;
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "%s no longer has <condition type=%d>%s</condition>", victim, effect, condition_name);
}

//  @todo:  probably better to have each condition effect to have their own tick text format
void game_ui_log_condition_tick(const char* victim, Condition_Effect_Type effect, s32 damage)
{
    if (damage)
    {
        Game_UI* game_ui = &s_app->game_ui;
        Game_Log* log = make_game_log();
        s32 log_id = (s32)(log - game_ui->logs);
        const char* condition_name = get_condition_name(effect);
        
        if (damage > 0)
        {
            log->text = arena_fmt(&log->arena, "%s took <damage log_id=%d>%d</damage> damage from <condition type=%d>%s</condition>", victim, log_id, damage, effect, condition_name);
        }
        else
        {
            log->text = arena_fmt(&log->arena, "%s was healed for <damage log_id=%d>%d</damage> from <condition type=%d>%s</condition>", victim, log_id, cf_abs(damage), effect, condition_name);
        }
        
        Damage_Source source = { 0 };
        source.damage = damage;
        source.effect = effect;
        MAKE_ARENA_ARRAY(&log->arena, log->damage_sources, 1);
        cf_array_push(log->damage_sources, source);
    }
}

void game_ui_log_condition_inflict(const char* attacker, const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();;
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "%s inflicted %s with <condition type=%d>%s</condition>", attacker, victim, effect, condition_name);
}

void game_ui_log_condition_purge(const char* attacker, const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();;
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "%s purged <condition type=%d>%s</condition> from %s", attacker, effect, condition_name, victim);
}

void game_ui_log_hand_select_spell(Spell_Type spell)
{
    Game_Log* log = make_game_log();;
    const char* spell_name = get_spell_name(spell);
    log->text = arena_fmt(&log->arena, "The Hand selected <spell type=%d>%s</spell>", spell, spell_name);
}

void game_ui_log_hand_cast_spell(Spell_Type spell)
{
    Game_Log* log = make_game_log();;
    const char* spell_name = get_spell_name(spell);
    log->text = arena_fmt(&log->arena, "The Hand casted <spell type=%d>%s</spell>", spell, spell_name);
}

void game_ui_log_hand_condition_add(const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();;
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "The Hand gave %s <condition type=%d>%s</condition>", victim, effect, condition_name);
}

void game_ui_log_hand_condition_remove(const char* victim, Condition_Effect_Type effect)
{
    Game_Log* log = make_game_log();;
    const char* condition_name = get_condition_name(effect);
    log->text = arena_fmt(&log->arena, "The Hand removed <condition type=%d>%s</condition> from %s", effect, condition_name, victim);
}

void game_ui_log_hand_damage(const char* victim, Spell_Type spell, s32 damage)
{
    Game_UI* game_ui = &s_app->game_ui;
    
    if (damage)
    {
        Game_Log* log = make_game_log();
        s32 log_id = (s32)(log - game_ui->logs);
        Damage_Source source = { 0 };
        source.damage = damage;
        source.spell = spell;
        MAKE_ARENA_ARRAY(&log->arena, log->damage_sources, 1);
        cf_array_push(log->damage_sources, source);
        
        const char* spell_name = get_spell_name(spell);
        if (damage > 0)
        {
            log->text = arena_fmt(&log->arena, "The Hand dealt <damage log_id=%d>%d</damage> to %s with <spell type=%d>%s</spell>", log_id, damage, victim, spell, spell_name);
        }
        else
        {
            log->text = arena_fmt(&log->arena, "The Hand healed <damage log_id=%d>%d</damage> %s with <spell type=%d>%s</spell>", log_id, damage, victim, spell, spell_name);
        }
    }
}

void game_ui_log_hand_slap(const char* victim)
{
    Game_UI* game_ui = &s_app->game_ui;
    Game_Log* log = make_game_log();
    log->text = arena_fmt(&log->arena, "The Hand slaps %s", victim);
}

fixed char* game_ui_attributes_health_tooltip(CF_MAP(Attributes) attributes)
{
    fixed char* text = scratch_string(1024);
    s32 total = 0;
    
    if (attributes)
    {
        cf_string_fmt(text, "%d ", attributes->health);
        total = attributes->health;
    }
    else
    {
        cf_string_append(text, "0 ");
    }
    
    u64* keys = cf_map_keys(attributes);
    for (s32 index = 1; index < cf_map_size(attributes); ++index)
    {
        s32 value = attributes[index].health;
        u64 mask = keys[index];
        if (value)
        {
            if (BIT_IS_SET_EX(mask, CONDITION_MASK))
            {
                Condition_Effect_Type effect = get_condition_effect_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %d (<condition type=%d>%s</condition>) ", value, effect, get_condition_name(effect));
                }
                else
                {
                    cf_string_fmt_append(text, "- %d (<condition type=%d>%s</condition>) ", cf_abs(value), effect,  get_condition_name(effect));
                }
            }
            else if (BIT_IS_SET_EX(mask, ITEM_MASK))
            {
                Item_Type item_type = get_item_type_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %d (<item type=%d>%s</item>) ", value, item_type, get_item_name(item_type));
                }
                else
                {
                    cf_string_fmt_append(text, "- %d (<item type=%d>%s</item>) ", cf_abs(value), item_type,  get_item_name(item_type));
                }
            }
        }
        total += value;
    }
    
    cf_string_fmt_append(text, "= %d", total);
    
    return text;
}

fixed char* game_ui_attributes_strength_tooltip(CF_MAP(Attributes) attributes)
{
    fixed char* text = scratch_string(1024);
    s32 total = 0;
    
    if (attributes)
    {
        cf_string_fmt(text, "%d ", attributes->strength);
        total = attributes->strength;
    }
    else
    {
        cf_string_append(text, "0 ");
    }
    
    u64* keys = cf_map_keys(attributes);
    for (s32 index = 1; index < cf_map_size(attributes); ++index)
    {
        s32 value = attributes[index].strength;
        u64 mask = keys[index];
        if (value)
        {
            if (BIT_IS_SET_EX(mask, CONDITION_MASK))
            {
                Condition_Effect_Type effect = get_condition_effect_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %d (<condition type=%d>%s</condition>) ", value, effect, get_condition_name(effect));
                }
                else
                {
                    cf_string_fmt_append(text, "- %d (<condition type=%d>%s</condition>) ", cf_abs(value), effect,  get_condition_name(effect));
                }
            }
            else if (BIT_IS_SET_EX(mask, ITEM_MASK))
            {
                Item_Type item_type = get_item_type_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %d (<item type=%d>%s</item>) ", value, item_type, get_item_name(item_type));
                }
                else
                {
                    cf_string_fmt_append(text, "- %d (<item type=%d>%s</item>) ", cf_abs(value), item_type,  get_item_name(item_type));
                }
            }
        }
        total += value;
    }
    
    cf_string_fmt_append(text, "= %d", total);
    
    return text;
}

fixed char* game_ui_attributes_agility_tooltip(CF_MAP(Attributes) attributes)
{
    fixed char* text = scratch_string(1024);
    s32 total = 0;
    
    if (attributes)
    {
        cf_string_fmt(text, "%d ", attributes->agility);
        total = attributes->agility;
    }
    else
    {
        cf_string_append(text, "0 ");
    }
    
    u64* keys = cf_map_keys(attributes);
    for (s32 index = 1; index < cf_map_size(attributes); ++index)
    {
        s32 value = attributes[index].agility;
        u64 mask = keys[index];
        if (value)
        {
            if (BIT_IS_SET_EX(mask, CONDITION_MASK))
            {
                Condition_Effect_Type effect = get_condition_effect_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %d (<condition type=%d>%s</condition>) ", value, effect, get_condition_name(effect));
                }
                else
                {
                    cf_string_fmt_append(text, "- %d (<condition type=%d>%s</condition>) ", cf_abs(value), effect,  get_condition_name(effect));
                }
            }
            else if (BIT_IS_SET_EX(mask, ITEM_MASK))
            {
                Item_Type item_type = get_item_type_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %d (<item type=%d>%s</item>) ", value, item_type, get_item_name(item_type));
                }
                else
                {
                    cf_string_fmt_append(text, "- %d (<item type=%d>%s</item>) ", cf_abs(value), item_type,  get_item_name(item_type));
                }
            }
        }
        total += value;
    }
    
    cf_string_fmt_append(text, "= %d", total);
    
    return text;
}

fixed char* game_ui_attributes_move_speed_tooltip(CF_MAP(Attributes) attributes)
{
    fixed char* text = scratch_string(1024);
    f32 total = 0;
    
    if (attributes)
    {
        cf_string_fmt(text, "%.2f ", attributes->move_speed);
        total = attributes->move_speed;
    }
    else
    {
        cf_string_append(text, "0 ");
    }
    
    u64* keys = cf_map_keys(attributes);
    for (s32 index = 1; index < cf_map_size(attributes); ++index)
    {
        f32 value = attributes[index].move_speed;
        u64 mask = keys[index];
        if (cf_abs(value) > 1e-7f)
        {
            if (BIT_IS_SET_EX(mask, CONDITION_MASK))
            {
                Condition_Effect_Type effect = get_condition_effect_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %.2f (<condition type=%d>%s</condition>) ", value, effect, get_condition_name(effect));
                }
                else
                {
                    cf_string_fmt_append(text, "- %.2f (<condition type=%d>%s</condition>) ", cf_abs(value), effect,  get_condition_name(effect));
                }
            }
            else if (BIT_IS_SET_EX(mask, ITEM_MASK))
            {
                Item_Type item_type = get_item_type_from_mask(mask);
                if (value > 0)
                {
                    cf_string_fmt_append(text, "+ %.2f (<item type=%d>%s</item>) ", value, item_type, get_item_name(item_type));
                }
                else
                {
                    cf_string_fmt_append(text, "- %.2f (<item type=%d>%s</item>) ", cf_abs(value), item_type,  get_item_name(item_type));
                }
            }
        }
        total += value;
    }
    
    cf_string_fmt_append(text, "= %.2f", total);
    
    return text;
}

// ui states
void game_ui_do_splash_co(CF_Coroutine co)
{
    CF_Sprite cf_logo = assets_get_sprite("sprites/CF_Logo_Hifi.png");
    
    // start show
    f32 delay = 1.0f;
    while (delay > 0)
    {
        CF_Color text_color = cf_color_white();
        text_color.a = 1.0f - delay;
        
        cf_logo.opacity = text_color.a;
        
        ui_do_sprite(cf_logo);
        ui_push_text_color(text_color);
        ui_do_text("Cute Framework");
        ui_pop_text_color();
        
        delay -= CF_DELTA_TIME;
        cf_coroutine_yield(co);
    }
    
    // hold
    delay = 1.0f;
    while (delay > 0)
    {
        ui_do_sprite(cf_logo);
        ui_do_text("Cute Framework");
        
        delay -= CF_DELTA_TIME;
        cf_coroutine_yield(co);
    }
    
    // hide
    delay = 1.0f;
    while (delay > 0)
    {
        CF_Color text_color = cf_color_white();
        text_color.a = delay;
        
        cf_logo.opacity = text_color.a;
        
        ui_do_sprite(cf_logo);
        ui_push_text_color(text_color);
        ui_do_text("Cute Framework");
        ui_pop_text_color();
        
        delay -= CF_DELTA_TIME;
        cf_coroutine_yield(co);
    }
}

void game_ui_do_splash()
{
    Game_UI* game_ui = &s_app->game_ui;
    
    b32 go_to_main_menu = false;
    if (cf_button_binding_consume_press(game_ui->do_close_window))
    {
        go_to_main_menu = true;
    }
    
    if (COROUTINE_IS_RUNNING(game_ui->state_co))
    {
        UI_Layout_Alignment alignment = BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Center);
        
        ui_push_layout_background_color(cf_color_black());
        ui_layout_begin("Splash");
        ui_pop_layout_background_color();
        
        ui_layout_set_alignment(alignment);
        
        ui_push_item_alignment(UI_Item_Alignment_Center);
        cf_push_font_size(64.0f);
        
        cf_coroutine_resume(game_ui->state_co);
        
        cf_pop_font_size();
        ui_pop_item_alignment();
        
        ui_layout_end();
    }
    else
    {
        go_to_main_menu = true;
    }
    
    if (go_to_main_menu)
    {
        cf_destroy_coroutine(game_ui->state_co);
        game_ui_set_state(Game_UI_State_Main_Menu);
    }
}

void game_ui_do_main_menu()
{
    Overworld* overworld = &s_app->overworld;
    Inventory* inventory = &s_app->world.inventory;
    Game_UI* game_ui = &s_app->game_ui;
    
    ui_push_layout_background_color(menu_layout_background_color());
    ui_layout_begin("Main Menu");
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Center));
    ui_push_item_alignment(UI_Item_Alignment_Center);
    
    if (game_ui_do_button("Play"))
    {
        overworld_clear();
        overworld_generate();
        world_set_state(World_State_Overworld);
        inventory_reset(inventory);
        inventory_randomize(inventory, cf_rnd_uint64(&overworld->rnd));
        game_ui->show_inventory = false;
        
        game_ui_set_state(Game_UI_State_Play);
    }
    if (game_ui_do_button("Options"))
    {
        game_ui_push_state(Game_UI_State_Options);
    }
    if (game_ui_do_button("Exit"))
    {
        cf_app_signal_shutdown();
    }
    
    ui_pop_item_alignment();
    ui_layout_end();
    ui_pop_layout_background_color();
}

void game_ui_do_options()
{
    Audio* audio = &s_app->audio;
    
    CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT), UI_WIDTH, UI_HEIGHT * 0.9f);
    CF_V2 content_extents = cf_extents(layout_aabb);
    content_extents.y *= 0.8f;
    
    cf_push_font_size(64.0f);
    ui_push_layout_background_color(menu_layout_background_color());
    ui_layout_begin("Options");
    ui_pop_layout_background_color();
    ui_layout_set_title("Options");
    ui_layout_set_aabb(layout_aabb);
    
    cf_pop_font_size();
    
    typedef s32 Options_Tab;
    //  @todo:  get rid of the prefix here
#define Options_Tab(PREFIX, ENUM) \
ENUM(PREFIX, Audio) \
ENUM(PREFIX, Gameplay) \
ENUM(PREFIX, Count)
    
    MAKE_ENUM(Options_Tab);
#undef Options_Tab
    static Options_Tab tab = Options_Tab_Audio;
    
    if (ui_do_tabs(Options_Tab_names, Options_Tab_Count, &tab))
    {
        printf("%s\n", Options_Tab_names[tab]);
    }
    
    b32 go_back = false;
    
    if (cf_button_binding_consume_press(s_app->game_ui.do_close_window))
    {
        go_back = true;
    }
    
    // content
    {
        ui_child_layout_begin(content_extents);
        ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
        ui_layout_do_scrollbar(true);
        
        if (tab == Options_Tab_Audio)
        {
            UI_Layout* layout = ui_child_layout_begin(content_extents);
            BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
            {
                CF_V2 name_extents = content_extents;
                CF_V2 value_extents = content_extents;
                name_extents.x *= 0.25f;
                value_extents.x *= 0.75f;
                
                UI_Layout* child_layout = ui_child_layout_begin(name_extents);
                BIT_SET(child_layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
                ui_do_text("Master");
                ui_do_text("Music");
                ui_do_text("Sfx");
                ui_do_text("UI");
                ui_child_layout_end();
                
                ui_do_same_line();
                child_layout = ui_child_layout_begin(value_extents);
                BIT_SET(child_layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
                game_ui_do_slider(&audio->volume.master, 0.0f, 1.0f, 0.01f);
                game_ui_do_slider(&audio->volume.music, 0.0f, 1.0f, 0.01f);
                game_ui_do_slider(&audio->volume.sfx, 0.0f, 1.0f, 0.01f);
                game_ui_do_slider(&audio->volume.ui, 0.0f, 1.0f, 0.01f);
                ui_child_layout_end();
            }
            ui_layout_end();
        } 
        else if (tab == Options_Tab_Gameplay)
        {
            CF_Color* cursor_color = &s_app->cursor.color;
            
            CF_V2 hand_section_extents = content_extents;
            hand_section_extents.y *= 0.4f;
            
            ui_child_layout_begin(hand_section_extents);
            ui_layout_set_title("Hand");
            ui_do_color_wheel(cursor_color);
            ui_child_layout_end();
            
        }
        ui_pop_item_alignment();
        ui_child_layout_end();
    }
    
    ui_layout_end();
    
    // footer
    {
        CF_Aabb footer_aabb = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT * 0.1f), UI_WIDTH, UI_HEIGHT * 0.1f);
        ui_push_layout_background_color(menu_layout_background_color());
        ui_layout_begin("Options");
        ui_pop_layout_background_color();
        ui_layout_set_aabb(footer_aabb);
        ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Bottom) | BIT(UI_Layout_Alignment_Horizontal_Left));
        ui_layout_set_direction(UI_Layout_Direction_Up);
        
        if (game_ui_do_button("Back"))
        {
            go_back = true;
        }
        
        ui_layout_end();
    }
    
    
    if (go_back)
    {
        game_ui_pop_state();
    }
    
}

void game_ui_do_pause()
{
    World* world = &s_app->world;
    
    ui_push_layout_background_color(menu_layout_background_color());
    ui_layout_begin("Pause");
    ui_pop_layout_background_color();
    
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Center) | BIT(UI_Layout_Alignment_Horizontal_Center));
    ui_push_item_alignment(UI_Item_Alignment_Center);
    
    cf_push_font_size(64.0f);
    ui_do_text("Pause");
    cf_pop_font_size();
    
    b32 go_back = false;
    
    if (cf_button_binding_consume_press(s_app->game_ui.do_close_window))
    {
        go_back = true;
    }
    
    if (game_ui_do_button("Continue"))
    {
        go_back = true;
    }
    if (game_ui_do_button("Options"))
    {
        game_ui_push_state(Game_UI_State_Options);
    }
    if (game_ui_do_button("Main Menu"))
    {
        world_set_state(World_State_Overworld);
        game_ui_set_state(Game_UI_State_Main_Menu);
    }
    
    if (go_back)
    {
        game_ui_pop_state();
    }
    
    ui_pop_item_alignment();
    ui_layout_end();
}

void game_ui_do_play()
{
    World* world = &s_app->world;
    Game_UI* game_ui = &s_app->game_ui;
    
    if (cf_array_count(game_ui->pin_tooltips))
    {
        if (cf_button_binding_consume_press(game_ui->do_close_window))
        {
            cf_array_last(game_ui->pin_tooltips).do_close = true;
        }
    }
    
    // handle closing all tooltips
    if (cf_key_ctrl() && cf_key_just_pressed(CF_KEY_SPACE))
    {
        game_ui_close_all_tooltips();
    }
    
    switch (world->state)
    {
        case World_State_Overworld:
        {
            overworld_ui();
            break;
        }
        case World_State_Arena_Placement:
        {
            game_ui_do_arena_placement();
            break;
        }
        case World_State_Arena_In_Progress:
        {
            game_ui_do_arena_in_progress();
            break;
        }
        case World_State_Arena_End:
        {
            game_ui_do_arena_result();
            break;
        }
        case World_State_Shop:
        {
            game_ui_do_shop();
            break;
        }
    }
    
    // inventory button
    {
        ui_layout_begin("Inventory Button");
        ui_layout_set_aabb(cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT * 0.1f), UI_WIDTH * 0.1f, UI_HEIGHT * 0.1f));
        ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Bottom) | BIT(UI_Layout_Alignment_Horizontal_Left));
        ui_layout_set_direction(BIT(UI_Layout_Direction_Up));
        ui_push_item_alignment(UI_Item_Alignment_Left);
        
        CF_Sprite sprite = assets_get_sprite("sprites/notepad.png");
        if (ui_do_sprite_button(sprite))
        {
            audio_play_ui("sounds/ui_button.wav");
            game_ui->show_inventory = !game_ui->show_inventory;
        }
        
        ui_layout_end();
        
        if (game_ui->show_inventory)
        {
            game_ui_do_inventory();
        }
    }
    
    game_ui_do_tooltips();
    
    if (cf_button_binding_consume_press(s_app->game_ui.do_close_window))
    {
        game_ui_push_state(Game_UI_State_Pause);
    }
}

void game_ui_do_debug_stats()
{
    static s32 select_offset_index = 0;
    
    Profiler* profiler = profiler_get();
    
    cf_push_font_size(36.0f);
    ui_push_layout_background_color(menu_layout_background_color());
    UI_Layout* layout = ui_layout_begin("Stats");
    ui_layout_set_title(scratch_fmt("FPS %.2f | %.2f", cf_app_get_framerate(), cf_app_get_smoothed_framerate()));
    ui_pop_layout_background_color();
    cf_pop_font_size();
    
    CF_Aabb aabb = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT * 0.25f), UI_WIDTH * 0.5f, UI_HEIGHT * 0.25f);
    ui_layout_set_aabb(aabb);
    ui_layout_set_scrollable(BIT(UI_Layout_Scroll_Vertical), cf_v2(0, 0));
    ui_layout_do_scrollbar(true);
    
    cf_push_font_size(24.0f);
    
    // graph
    {
        fixed f32* durations = NULL;
        MAKE_SCRATCH_ARRAY(durations, cf_array_count(profiler->frames));
        
        for (s32 index = profiler->read_index; index != profiler->write_index;)
        {
            Profile_Frame* frame = profiler->frames + index;
            cf_array_push(durations, (f32)frame->samples[0].duration);
            index = (index + 1) % cf_array_count(profiler->frames);
        }
        
        ui_push_border_color(cf_color_white());
        if (ui_do_graph_line(durations, cf_array_count(durations), &select_offset_index))
        {
            profiler_pause(true);
        }
        ui_pop_border_color();
    }
    
    // samples
    {
        s32 select_index = (profiler->read_index + select_offset_index) % cf_array_count(profiler->frames);
        if (select_index == profiler->write_index)
        {
            select_index = profiler->write_index - 1 + cf_array_count(profiler->frames);
            select_index = select_index % cf_array_count(profiler->frames);
        }
        
        profiler_ui_do_flame_graph(profiler, select_index);
    }
    
    if (!ui_layout_is_hovering(layout) && profiler_is_paused())
    {
        profiler_pause(false);
        select_offset_index = cf_array_count(profiler->frames) - 2;
    }
    
    ui_layout_end();
    
    cf_pop_font_size();
}

void profiler_ui_sample_tooltip(Profiler* profiler, s32 frame_index, Profile_Sample* sample)
{
    UI_Layout* layout = ui_peek_layout();
    UI_Item* item = ui_layout_peek_item();
    BIT_SET(item->state, UI_Item_State_Can_Hover);
    
    if (ui_item_is_hovered())
    {
        ui_push_layout_background_color(cf_color_black());
        ui_push_layout_border_color(cf_color_white());
        ui_push_layout_border_thickness(1);
        ui_push_layout_corner_radius(5);
        
        game_ui_tooltip_begin(cf_extents(layout->aabb));
        
        fixed f32* durations = NULL;
        MAKE_SCRATCH_ARRAY(durations, cf_array_count(profiler->frames));
        
        s32 read_index = profiler->read_index;
        while (read_index != profiler->write_index)
        {
            Profile_Frame* tooltip_frame = profiler->frames + read_index;
            cf_array_push(durations, 0);
            for (s32 sample_index = 0; sample_index < cf_array_count(tooltip_frame->samples); ++sample_index)
            {
                if (cf_string_equ(tooltip_frame->samples[sample_index].name, sample->name))
                {
                    cf_array_last(durations) = (f32)tooltip_frame->samples[sample_index].duration;
                    break;
                }
            }
            read_index = (read_index + 1) % cf_array_count(profiler->frames);
        }
        
        ui_push_border_thickness(1);
        ui_push_border_color(cf_color_white());
        
        s32 graph_line_index = (frame_index - profiler->read_index + cf_array_count(profiler->frames)) % cf_array_count(profiler->frames);
        ui_do_graph_line(durations, cf_array_count(durations), &graph_line_index);
        
        ui_pop_border_color();
        ui_pop_border_thickness();
        
        s32 frame = profiler->frames[frame_index].frame;
        
        ui_do_text("%s | %d\n%s:%d\n%.2fms\n%" PRIu64 " | %" PRIu64" | %" PRIu64, sample->name, frame, sample->file, sample->line, sample->duration, sample->start, sample->end, sample->end - sample->start);
        game_ui_tooltip_end();
        
        ui_pop_layout_background_color();
        ui_pop_layout_border_color();
        ui_pop_layout_border_thickness();
        ui_pop_layout_corner_radius();
    }
}

void profiler_ui_do_flame_graph(Profiler* profiler, s32 frame_index)
{
    Profile_Frame* frame = profiler->frames + frame_index;
    if (cf_array_count(frame->samples) == 0)
    {
        return;
    }
    
    UI_Layout* layout = ui_peek_layout();
    
    CF_V2 layout_extents = cf_extents(layout->usable_aabb);
    
    ui_push_layout_border_thickness(1.0f);
    ui_push_layout_border_color(cf_color_white());
    
    f32 graph_width = layout_extents.x - layout->item_padding;
    f32 inner_graph_width = graph_width - layout->item_padding * 2;
    
    f32 height = cf_peek_font_size();
    
    // entire flame graph will be inside a child layout to resolve arbitary Y height
    // since we don't iknow how deep a call stack can get
    UI_Layout* child_layout = ui_child_layout_begin(cf_v2(graph_width, height));
    BIT_SET(child_layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
    ui_layout_set_direction(0);
    
    ui_pop_layout_border_thickness();
    ui_pop_layout_border_color();
    
    u64 start = frame->samples[0].start;
    u64 end = frame->samples[0].end;
    f32 range = (f32)(end - start);
    
    for (s32 index = 0; index < cf_array_count(frame->samples); ++index)
    {
        Profile_Sample* sample = frame->samples + index;
        
        s32 depth = 0;
        
        // resolve how deep into a callstack a sample is to offset along y axis
        for (s32 depth_index = 0; depth_index < index; ++depth_index)
        {
            Profile_Sample* depth_sample = frame->samples + depth_index;
            if (sample->start >= depth_sample->start && sample->end <= depth_sample->end)
            {
                depth++;
            }
        }
        
        f32 x0 = (sample->start - start) / range * inner_graph_width;
        f32 x1 = (sample->end - start) / range * inner_graph_width;
        
        // this graph is going downwards with the root being very top
        f32 y0 = -height * (depth + 1);
        f32 y1 = -height * depth;
        
        {
            ui_push_background_color(cf_color_grey());
            ui_push_border_color(cf_color_white());
            
            UI_Item* item = ui_make_item();
            
            CF_V2 min = cf_v2(x0, y0);
            CF_V2 max = cf_v2(x1, y1);
            
            f32 text_width = cf_text_width(sample->name, -1);
            if (text_width < x1 - x0)
            {
                item->text = scratch_fmt("%s", sample->name);
            }
            item->aabb = cf_make_aabb(min, max);
            item->text_aabb = item->aabb;
            item->interactable_aabb = item->aabb;
            
            ui_pop_border_color();
            ui_pop_background_color();
        }
        
        // dump metrics
        profiler_ui_sample_tooltip(profiler, frame_index, sample);
    }
    
    ui_child_layout_end();
    
    ui_do_text("Frame: %d | %.2fms", frame->frame, frame->samples[0].duration);
    
    // simple dump for visibility so you don't need to scrub the entire flame graph
    for (s32 index = 0; index < cf_array_count(frame->samples); ++index)
    {
        Profile_Sample* sample = frame->samples + index;
        ui_do_text("%s | %.2fms", sample->name, sample->duration);
        profiler_ui_sample_tooltip(profiler, frame_index, sample);
    }
}
