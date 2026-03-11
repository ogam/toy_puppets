#include "game/ui.h"

//  @bug:  fix hovering over items when items are right aligned with horizontal right align layout
//         and layout direction left
//         items will bounce back and forth
//  @bug:  fix scrollbar knob dragging

#define UI_STYLE_FUNCS(TYPE, NAME) \
void ui_push_##NAME(TYPE value) \
{ \
UI_Style* style = &s_app->ui.style; \
cf_array_push(style->NAME##s, value); \
} \
TYPE ui_peek_##NAME() \
{ \
UI_Style* style = &s_app->ui.style; \
return cf_array_last(style->NAME##s); \
} \
TYPE ui_pop_##NAME() \
{ \
UI_Style* style = &s_app->ui.style; \
TYPE value = cf_array_last(style->NAME##s); \
if (cf_array_count(style->NAME##s) > 1) \
{ \
cf_array_pop(style->NAME##s); \
} \
return value; \
}

UI_STYLE_FUNCS(CF_Color, text_color);
UI_STYLE_FUNCS(CF_Color, text_shadow_color);
UI_STYLE_FUNCS(CF_Color, background_color);
UI_STYLE_FUNCS(CF_Color, border_color);
UI_STYLE_FUNCS(UI_Item_Alignment, item_alignment);
UI_STYLE_FUNCS(f32, border_thickness);
UI_STYLE_FUNCS(f32, corner_radius);
UI_STYLE_FUNCS(b32, word_wrap);

UI_STYLE_FUNCS(CF_Color, idle_text_color);
UI_STYLE_FUNCS(CF_Color, idle_text_shadow_color);
UI_STYLE_FUNCS(CF_Color, idle_background_color);
UI_STYLE_FUNCS(CF_Color, idle_border_color);

UI_STYLE_FUNCS(CF_Color, hover_text_color);
UI_STYLE_FUNCS(CF_Color, hover_text_shadow_color);
UI_STYLE_FUNCS(CF_Color, hover_background_color);
UI_STYLE_FUNCS(CF_Color, hover_border_color);

UI_STYLE_FUNCS(CF_Color, pressed_text_color);
UI_STYLE_FUNCS(CF_Color, pressed_text_shadow_color);
UI_STYLE_FUNCS(CF_Color, pressed_background_color);
UI_STYLE_FUNCS(CF_Color, pressed_border_color);

UI_STYLE_FUNCS(b32, is_disabled);
UI_STYLE_FUNCS(CF_Color, disabled_color);

UI_STYLE_FUNCS(CF_Color, layout_background_color);
UI_STYLE_FUNCS(CF_Color, layout_border_color);
UI_STYLE_FUNCS(CF_Color, layout_title_color);
UI_STYLE_FUNCS(f32, layout_border_thickness);
UI_STYLE_FUNCS(f32, layout_corner_radius);
UI_STYLE_FUNCS(f32, layout_item_padding);

bool ui_text_fx_keyword(CF_TextEffect* fx)
{
    UI* ui = &s_app->ui;
    
    if (cf_map_has(ui->text_effect_map, fx->effect_name))
    {
        UI_Text_Effect_Callbacks callbacks = cf_map_get(ui->text_effect_map, fx->effect_name);
        callbacks.draw(fx);
    }
    
    return true;
}

void ui_text_markup_info(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
    UI* ui = &s_app->ui;
    CF_V2 mouse_position = ui->input.mouse_position;
    
    if (cf_map_has(ui->text_effect_map, fx->effect_name))
    {
        UI_Text_Effect_Callbacks callbacks = cf_map_get(ui->text_effect_map, fx->effect_name);
        
        for (s32 index = 0; index < info.bounds_count; ++index)
        {
            if (cf_contains_point(info.bounds[index], mouse_position))
            {
                callbacks.hover((CF_TextEffect*)fx);
                break;
            }
        }
    }
}

void init_ui()
{
    UI* ui = &s_app->ui;
    UI_Input* input = &ui->input;
    
    node_pool_init(&ui->layout_pool);
    
    cf_array_fit(ui->layouts, UI_LAYOUT_CAPACITY);
    for (s32 index = 0; index < UI_LAYOUT_CAPACITY; ++index)
    {
        UI_Layout layout = { 0 };
        cf_array_fit(layout.items, UI_LAYOUT_ITEM_MIN_CAPACITY);
        cf_array_push(ui->layouts, layout);
        
        node_pool_add(&ui->layout_pool, &cf_array_last(ui->layouts).node);
    }
    
    cf_array_fit(ui->layout_stack, 8);
    
    input->select = cf_make_button_binding(0, 0.1f);
    input->cancel = cf_make_button_binding(0, 0.1f);
    input->menu = cf_make_button_binding(0, 0.1f);
    input->drag = cf_make_button_binding(0, 0.1f);
    
    cf_button_binding_add_mouse_button(input->select, CF_MOUSE_BUTTON_LEFT);
    cf_button_binding_add_key(input->select, CF_KEY_ESCAPE);
    cf_button_binding_add_key(input->select, CF_KEY_ESCAPE);
    cf_button_binding_add_mouse_button(input->drag, CF_MOUSE_BUTTON_LEFT);
}

void ui_draw_layout(UI_Layout* layout)
{
    CF_Aabb screen_scissor = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT), UI_WIDTH, UI_HEIGHT);
    
    CF_Aabb scissor = layout->aabb;
    scissor = cf_expand_aabb_f(scissor, layout->border_thickness + layout->corner_radius);
    // make sure current child layout is clipped within parent layouts
    {
        UI_Layout* next = layout->parent;
        while (next)
        {
            scissor = cf_clamp_aabb(scissor, next->usable_aabb);
            next = next->parent;
        }
    }
    
    push_scissor(scissor);
    
    if (layout->background_color.a > 0)
    {
        cf_draw_push_color(layout->background_color);
        cf_draw_box_fill(layout->aabb, layout->corner_radius);
        cf_draw_pop_color();
    }
    
    if (layout->border_color.a > 0)
    {
        cf_draw_push_color(layout->border_color);
        cf_draw_box(layout->aabb, layout->border_thickness, layout->corner_radius);
        cf_draw_pop_color();
    }
    
    // title bar
    {
        CF_Aabb title_bar_aabb = layout->aabb;
        title_bar_aabb.min.y = layout->usable_aabb.max.y;
        if (layout->aabb.max.y > layout->usable_aabb.max.y)
        {
            cf_draw_push_color(layout->title_color);
            cf_draw_box_fill(title_bar_aabb, 0);
            cf_draw_pop_color();
        }
        
        if (layout->title)
        {
            cf_push_font_size(layout->title_font_size);
            CF_V2 position = cf_top(title_bar_aabb);
            CF_V2 text_size = cf_text_size(layout->title, -1);
            position.x -= text_size.x * 0.5f;
            cf_draw_text(layout->title, position, -1);
            cf_pop_font_size();
        }
    }
    
    // resolve inner scissor
    scissor = layout->usable_aabb;
    {
        UI_Layout* next = layout->parent;
        while (next)
        {
            scissor = cf_clamp_aabb(scissor, next->usable_aabb);
            next = next->parent;
        }
    }
    
    for (s32 index = 0; index < cf_array_count(layout->items); ++index)
    {
        UI_Item* item = layout->items + index;
        
        if (!BIT_IS_SET(item->state, UI_Item_State_Visible))
        {
            continue;
        }
        
        if (BIT_IS_SET(item->state, UI_Item_State_Ignore_Scissor))
        {
            push_scissor(screen_scissor);
        }
        
        if (item->custom_draw)
        {
            item->custom_draw(layout, item);
        }
        else
        {
            // draw normal boxes and text
            if (item->background_color.a > 0)
            {
                cf_draw_push_color(item->background_color);
                cf_draw_box_fill(item->aabb, item->corner_radius);
                cf_draw_pop_color();
            }
            
            if (item->border_color.a > 0)
            {
                cf_draw_push_color(item->border_color);
                cf_draw_box(item->aabb, item->border_thickness, item->corner_radius);
                cf_draw_pop_color();
            }
            
            if (item->text)
            {
                if (item->word_wrap)
                {
                    cf_push_text_wrap_width(cf_extents(layout->usable_aabb).x);
                }
                
                CF_V2 top_left = cf_top_left(item->text_aabb);
                CF_V2 shadow_top_left = cf_add(top_left, cf_v2(1, -1));
                
                cf_push_font_size(item->font_size);
                
                cf_draw_push_color(item->text_shadow_color);
                cf_draw_text(cf_text_without_markups(item->text), shadow_top_left, -1);
                cf_draw_pop_color();
                
                cf_draw_push_color(item->text_color);
                cf_draw_text(item->text, top_left, -1);
                cf_draw_pop_color();
                
                cf_pop_font_size();
                
                if (item->word_wrap)
                {
                    cf_pop_text_wrap_width();
                }
            }
        }
        
        if (BIT_IS_SET(item->state, UI_Item_State_Ignore_Scissor))
        {
            pop_scissor();
        }
    }
    
    pop_scissor();
}

void draw_ui()
{
    UI* ui = &s_app->ui;
    UI_Input* input = &ui->input;
    ui_push_camera();
    
    FOREACH_LIST(node, &ui->layout_pool.active_list)
    {
        UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
        
        if (!ui_layout_is_tooltip(layout))
        {
            ui_draw_layout(layout);
        }
    }
    
    FOREACH_LIST(node, &ui->layout_pool.active_list)
    {
        UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
        
        if (ui_layout_is_tooltip(layout))
        {
            ui_draw_layout(layout);
        }
    }
    
    ui_pop_camera();
}

void ui_begin()
{
    UI* ui = &s_app->ui;
    ui_update_input();
    node_pool_reset(&ui->layout_pool);
    cf_array_clear(ui->layout_stack);
    cf_map_clear(ui->item_map);
    ui_style_reset();
    
    cf_push_font_size(UI_DEFAULT_FONT_SIZE);
    
    {
        fixed const char** remove_layouts = NULL;
        MAKE_SCRATCH_ARRAY(remove_layouts, cf_map_size(ui->layout_var_tables));
        const char** keys = (const char**)cf_map_keys(ui->layout_var_tables);
        for (s32 index = 0; index < cf_map_size(ui->layout_var_tables); ++index)
        {
            CF_MAP(UI_Layout_Var) var_table = ui->layout_var_tables[index];
            UI_Layout_Var var = cf_map_get(var_table, cf_sintern("time"));
            if (CF_SECONDS - var.f64_value > 2.0f)
            {
                cf_array_push(remove_layouts, keys[index]);
            }
        }
        
        for (s32 index = 0; index < cf_array_count(remove_layouts); ++index)
        {
            cf_map_del(ui->layout_var_tables, remove_layouts[index]);
        }
    }
}

void ui_end()
{
    if (s_app->ui.debug)
    {
        ui_debug_view();
    }
    
    cf_pop_font_size();
    ui_build_layouts();
    ui_animate_layouts();
    ui_handle_input();
}

void ui_update_input()
{
    UI_Input* input = &s_app->ui.input;
    
    ui_push_camera();
    CF_V2 screen_position = cf_v2(cf_mouse_x(), cf_mouse_y());
    CF_V2 mouse_position = cf_screen_to_world(screen_position);
    ui_pop_camera();
    
    input->mouse_wheel = cf_mouse_wheel_motion();
    input->mouse_position = mouse_position;
}

void ui_style_reset()
{
    UI_Style* style = &s_app->ui.style;
    cf_array_clear(style->text_colors);
    cf_array_clear(style->text_shadow_colors);
    cf_array_clear(style->background_colors);
    cf_array_clear(style->border_colors);
    cf_array_clear(style->item_alignments);
    cf_array_clear(style->border_thicknesss);
    cf_array_clear(style->corner_radiuss);
    cf_array_clear(style->word_wraps);
    cf_array_clear(style->idle_text_colors);
    cf_array_clear(style->idle_text_shadow_colors);
    cf_array_clear(style->idle_background_colors);
    cf_array_clear(style->idle_border_colors);
    cf_array_clear(style->hover_text_colors);
    cf_array_clear(style->hover_text_shadow_colors);
    cf_array_clear(style->hover_background_colors);
    cf_array_clear(style->hover_border_colors);
    cf_array_clear(style->pressed_text_colors);
    cf_array_clear(style->pressed_text_shadow_colors);
    cf_array_clear(style->pressed_background_colors);
    cf_array_clear(style->pressed_border_colors);
    cf_array_clear(style->is_disableds);
    cf_array_clear(style->disabled_colors);
    cf_array_clear(style->layout_background_colors);
    cf_array_clear(style->layout_border_colors);
    cf_array_clear(style->layout_title_colors);
    cf_array_clear(style->layout_border_thicknesss);
    cf_array_clear(style->layout_corner_radiuss);
    
    
    cf_array_fit(style->text_colors, 8);
    cf_array_fit(style->text_shadow_colors, 8);
    cf_array_fit(style->background_colors, 8);
    cf_array_fit(style->border_colors, 8);
    cf_array_fit(style->item_alignments, 8);
    cf_array_fit(style->border_thicknesss, 8);
    cf_array_fit(style->corner_radiuss, 8);
    cf_array_fit(style->word_wraps, 8);
    cf_array_fit(style->idle_text_colors, 8);
    cf_array_fit(style->idle_text_shadow_colors, 8);
    cf_array_fit(style->idle_background_colors, 8);
    cf_array_fit(style->idle_border_colors, 8);
    cf_array_fit(style->hover_text_colors, 8);
    cf_array_fit(style->hover_text_shadow_colors, 8);
    cf_array_fit(style->hover_background_colors, 8);
    cf_array_fit(style->hover_border_colors, 8);
    cf_array_fit(style->pressed_text_colors, 8);
    cf_array_fit(style->pressed_text_shadow_colors, 8);
    cf_array_fit(style->pressed_background_colors, 8);
    cf_array_fit(style->pressed_border_colors, 8);
    cf_array_fit(style->is_disableds, 8);
    cf_array_fit(style->disabled_colors, 8);
    cf_array_fit(style->layout_background_colors, 8);
    cf_array_fit(style->layout_border_colors, 8);
    cf_array_fit(style->layout_title_colors, 8);
    cf_array_fit(style->layout_border_thicknesss, 8);
    cf_array_fit(style->layout_corner_radiuss, 8);
    
    ui_push_text_color(cf_color_white());
    ui_push_text_shadow_color(cf_color_black());
    ui_push_background_color(cf_color_clear());
    ui_push_border_color(cf_color_clear());
    ui_push_item_alignment(UI_Item_Alignment_Left);
    ui_push_border_thickness(1.0f);
    ui_push_corner_radius(0.0f);
    ui_push_word_wrap(false);
    
    ui_push_idle_text_color(cf_color_white());
    ui_push_idle_text_shadow_color(cf_color_black());
    ui_push_idle_background_color(cf_color_grey());
    ui_push_idle_border_color(cf_color_clear());
    
    ui_push_hover_text_color(cf_color_white());
    ui_push_hover_text_shadow_color(cf_color_black());
    ui_push_hover_background_color(cf_color_orange());
    ui_push_hover_border_color(cf_color_clear());
    
    ui_push_pressed_text_color(cf_color_black());
    ui_push_pressed_text_shadow_color(cf_color_white());
    ui_push_pressed_background_color(cf_color_yellow());
    ui_push_pressed_border_color(cf_color_clear());
    
    ui_push_is_disabled(false);
    ui_push_disabled_color(cf_color_grey());
    
    ui_push_layout_background_color(cf_color_clear());
    ui_push_layout_border_color(cf_color_clear());
    ui_push_layout_border_color(cf_color_clear());
    ui_push_layout_border_thickness(1.0f);
    ui_push_layout_corner_radius(0.0f);
}

void ui_push_camera()
{
    cf_draw_push();
    cf_draw_translate(UI_WIDTH * -0.5f, UI_HEIGHT * -0.5f);
}

void ui_pop_camera()
{
    cf_draw_pop();
}

// layout var
b32 ui_layout_has_var_ex(UI_Layout* layout, const char* name)
{
    UI* ui = &s_app->ui;
    b32 has_var = false;
    
    if (layout && cf_map_has(ui->layout_var_tables, layout->name))
    {
        CF_MAP(UI_Layout_Var) var_table = cf_map_get(ui->layout_var_tables, layout->name);
        has_var = cf_map_has(var_table, cf_sintern(name));
    }
    
    return has_var;
}

UI_Layout_Var ui_layout_get_var_ex(UI_Layout* layout, const char* name)
{
    UI* ui = &s_app->ui;
    UI_Layout_Var var = { 0 };
    
    if (layout && cf_map_has(ui->layout_var_tables, layout->name))
    {
        CF_MAP(UI_Layout_Var) var_table = cf_map_get(ui->layout_var_tables, layout->name);
        if (cf_map_has(var_table, cf_sintern(name)))
        {
            var = cf_map_get(var_table, cf_sintern(name));
        }
    }
    
    return var;
}

void ui_layout_set_var_ex(UI_Layout* layout, const char* name, UI_Layout_Var var)
{
    UI* ui = &s_app->ui;
    
    if (layout && cf_map_has(ui->layout_var_tables, layout->name))
    {
        CF_MAP(UI_Layout_Var) var_table = cf_map_get(ui->layout_var_tables, layout->name);
        cf_map_set(var_table, cf_sintern(name), var);
    }
}

b32 ui_layout_has_var(const char* name)
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_peek_layout();
    return ui_layout_has_var_ex(layout, name);
}

UI_Layout_Var ui_layout_get_var(const char* name)
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_peek_layout();
    return ui_layout_get_var_ex(layout, name);
}

void ui_layout_set_var(const char* name, UI_Layout_Var var)
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_peek_layout();
    ui_layout_set_var_ex(layout, name, var);
}

// layout
UI_Layout* ui_layout_begin(const char* name)
{
    UI* ui = &s_app->ui;
    CF_ListNode* node = node_pool_alloc(&ui->layout_pool);
    UI_Layout* layout = NULL;
    if (node)
    {
        layout = CF_LIST_HOST(UI_Layout, node, node);
        cf_array_clear(layout->items);
        cf_array_push(ui->layout_stack, layout);
        layout->parent = NULL;
        
        layout->name = cf_sintern(name);
        layout->title = NULL;
        layout->direction = UI_Layout_Direction_Down;
        layout->grid_direction = UI_Layout_Direction_None;
        BIT_ASSIGN(layout->alignment, UI_Layout_Alignment_Vertical_Top);
        BIT_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Left);
        layout->scroll_direction = 0;
        layout->state = 0;
        layout->item_padding = 10.0f;
        layout->child_count = 0;
        
        layout->aabb = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT), UI_WIDTH, UI_HEIGHT);
        layout->usable_aabb = layout->aabb;
        
        layout->background_color = ui_peek_layout_background_color();
        layout->border_color = ui_peek_layout_border_color();
        layout->title_color = ui_peek_layout_title_color();
        layout->border_thickness = ui_peek_layout_border_thickness();
        layout->corner_radius = ui_peek_layout_corner_radius();
        
        u64 layout_index = layout - ui->layouts;
        layout->item_hash = cf_fnv1a(name, (s32)CF_STRLEN(name));
        
        layout->expand_animation_t = cf_v2(1.0f, 1.0f);
        layout->title_font_size = cf_peek_font_size();
        
        layout->scroll = cf_v2(0, 0);
        
        if (ui_layout_has_var_ex(layout, "scroll"))
        {
            UI_Layout_Var scroll_var = ui_layout_get_var_ex(layout, "scroll");
            layout->scroll = scroll_var.v2_value;
        }
    }
    return layout;
}

void ui_layout_end()
{
    UI* ui = &s_app->ui;
    UI_Input* input = &ui->input;
    
    UI_Layout* layout = ui_peek_layout();
    cf_array_pop(ui->layout_stack);
    
    if (layout)
    {
        if (!cf_map_has(ui->layout_var_tables, layout->name))
        {
            CF_MAP(UI_Layout_Var) new_table = NULL;
            {
                UI_Layout_Var var = { 0 };
                var.string = layout->name;
                cf_map_set(new_table, cf_sintern("name"), var);
            }
            cf_map_set(ui->layout_var_tables, layout->name, new_table);
        }
        
        {
            UI_Layout_Var var = { 0 };
            var.f64_value = CF_SECONDS;
            ui_layout_set_var_ex(layout, cf_sintern("time"), var);
        }
        
        {
            UI_Layout_Var var = { 0 };
            var.b32_value = layout->scroll_direction != 0;
            ui_layout_set_var_ex(layout, cf_sintern("is_scrollable"), var);
        }
        
        // handle mouse scroll dragging and caching
        if (layout->scroll_direction)
        {
            // handle mouse dragging
            {
                b32 is_hovering = ui_layout_is_down_scrollable(layout);
                
                if (is_hovering &&
                    ui->down_hash == 0 &&
                    cf_button_binding_down(input->drag))
                {
                    CF_V2 scroll_extents = cf_v2(0, 0);
                    CF_V2 mouse_motion = cf_v2(0, 0);
                    CF_Aabb item_aabb = layout->item_aabb;
                    CF_Aabb usable_aabb = layout->usable_aabb;
                    
                    // layout grid can change the item_aabb size so use previous aabb size
                    if (ui_layout_has_var_ex(layout, "item_aabb"))
                    {
                        item_aabb = ui_layout_get_var_ex(layout, "item_aabb").aabb;
                        usable_aabb = ui_layout_get_var_ex(layout, "usable_aabb").aabb;
                    }
                    
                    CF_V2 item_extents = cf_extents(item_aabb);
                    CF_V2 extents = cf_extents(usable_aabb);
                    
                    // only do dragging if item_aabb is larger than usable_aabb
                    if (BIT_IS_SET(layout->scroll_direction, UI_Layout_Scroll_Vertical))
                    {
                        if (item_extents.y > extents.y)
                        {
                            mouse_motion.y = cf_mouse_motion_y();
                            scroll_extents.y = item_extents.y - extents.y;
                        }
                    }
                    if (BIT_IS_SET(layout->scroll_direction, UI_Layout_Scroll_Horizontal))
                    {
                        if (item_extents.x > extents.x)
                        {
                            mouse_motion.x = -cf_mouse_motion_x();
                            scroll_extents.x = item_extents.x - extents.x;
                        }
                    }
                    
                    mouse_motion = cf_mul_v2(mouse_motion, cf_safe_invert(scroll_extents));
                    layout->scroll = cf_add(layout->scroll, mouse_motion);
                    layout->scroll = cf_clamp01(layout->scroll);
                }
            }
            
            {
                UI_Layout_Var scroll_var = { 0 };
                scroll_var.v2_value = layout->scroll;
                ui_layout_set_var_ex(layout, "scroll", scroll_var);
            }
        }
    }
}

UI_Layout* ui_child_layout_begin(CF_V2 size)
{
    UI* ui = &s_app->ui;
    UI_Layout* parent = ui_peek_layout();
    CF_ASSERT(parent);
    
    UI_Item* item = ui_make_item();
    item->aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), size.x, size.y);
    item->text_aabb = item->aabb;
    item->interactable_aabb = item->aabb;
    
    UI_Layout* layout = ui_layout_begin(scratch_fmt("%s_child_%d", parent->name, parent->child_count++));
    ui_layout_set_aabb(item->aabb);
    item->layout = layout;
    layout->parent = parent;
    
    return layout;
}

void ui_child_layout_end()
{
    ui_layout_end();
}

UI_Layout* ui_peek_layout()
{
    UI* ui = &s_app->ui;
    if (cf_array_count(ui->layout_stack) > 0)
    {
        return cf_array_last(ui->layout_stack);
    }
    return NULL;
}

void ui_layout_set_title(const char* title)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->title = scratch_fmt(title);
    }
}

void ui_layout_set_aabb(CF_Aabb aabb)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->aabb = aabb;
        layout->usable_aabb = aabb;
    }
}

void ui_layout_expand_usable_aabb(CF_V2 value)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->usable_aabb = cf_expand_aabb(layout->usable_aabb, value);
    }
}

void ui_layout_set_alignment(UI_Layout_Alignment alignment)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->alignment = alignment;
    }
}

void ui_layout_set_direction(UI_Layout_Direction direction)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->direction = direction;
    }
}

void ui_layout_set_grid_direction(UI_Layout_Direction direction)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->grid_direction = direction;
    }
}

void ui_layout_set_scrollable(UI_Layout_Scroll scroll_direction, CF_V2 start_scroll)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->scroll_direction = scroll_direction;
        if (!ui_layout_has_var_ex(layout, "scroll"))
        {
            layout->scroll = start_scroll;
        }
    }
}

void ui_layout_do_scrollbar(b32 auto_hide)
{
    UI* ui = &s_app->ui;
    
    UI_Layout* layout = ui_peek_layout();
    if (!layout->scroll_direction)
    {
        return;
    }
    
    b32 try_horizontal_scrollbar = BIT_IS_SET(layout->scroll_direction, UI_Layout_Scroll_Horizontal);
    b32 try_vertical_scrollbar = BIT_IS_SET(layout->scroll_direction, UI_Layout_Scroll_Vertical);
    CF_V2 previous_layout_extents = cf_v2(0, 0);
    CF_V2 previous_layout_item_extents = cf_v2(0, 0);
    
    b32 is_hovering = ui_layout_is_hovering_scrollable(layout);
    
    {
        UI_Layout_Var var_aabb = ui_layout_get_var("usable_aabb");
        UI_Layout_Var var_item_aabb = ui_layout_get_var("item_aabb");
        
        previous_layout_extents = cf_extents(var_aabb.aabb);
        previous_layout_item_extents = cf_extents(var_item_aabb.aabb);
        
        if (auto_hide)
        {
            try_horizontal_scrollbar = try_horizontal_scrollbar && previous_layout_item_extents.x > previous_layout_extents.x;
            try_vertical_scrollbar = try_vertical_scrollbar && previous_layout_item_extents.y > previous_layout_extents.y;
        }
    }
    
    if (try_horizontal_scrollbar)
    {
        ui_do_scrollbar_ex(&layout->scroll.x, 0.0f, 1.0f, 0.0f, 1.0f, true);
        UI_Item *item = ui_layout_peek_item();
        BIT_SET(item->state, UI_Item_State_Skip_Auto_Tiling);
        BIT_SET(item->state, UI_Item_State_Ignore_Scissor);
        
        CF_V2 item_extents = cf_extents(item->aabb);
        CF_V2 offset = cf_v2(0, item_extents.y);
        
        ui_move_item(item, offset);
        
        layout->usable_aabb.min.y += cf_height(item->aabb);
    }
    if (try_vertical_scrollbar)
    {
        if (is_hovering || ui_layout_is_tooltip(layout))
        {
            layout->scroll.y = cf_clamp01(layout->scroll.y + ui->input.mouse_wheel * -0.05f);
        }
        
        ui_do_scrollbar_ex(&layout->scroll.y, 0.0f, 1.0f, 0.0f, 1.0f, false);
        UI_Item *item = ui_layout_peek_item();
        BIT_SET(item->state, UI_Item_State_Skip_Auto_Tiling);
        BIT_SET(item->state, UI_Item_State_Ignore_Scissor);
        
        CF_V2 slider_extents = cf_extents(item->aabb);
        CF_V2 item_extents = cf_extents(item->aabb);
        CF_V2 offset = cf_v2(0.0f, item_extents.y);
        
        if (ui_layout_has_var_ex(layout, "aabb"))
        {
            UI_Layout_Var var = ui_layout_get_var_ex(layout, "aabb");
            offset.x = cf_width(var.aabb) - item_extents.x;
        }
        
        ui_move_item(item, offset);
        
        layout->usable_aabb.max.x -= slider_extents.x;
    }
}

void ui_layout_set_expand_animation_t(CF_V2 expand_t)
{
    UI_Layout* layout = ui_peek_layout();
    if (layout)
    {
        layout->expand_animation_t = cf_clamp01(expand_t);
    }
}

UI_Item* ui_layout_peek_item()
{
    UI_Layout* layout = ui_peek_layout();
    UI_Item* item = NULL;
    if (layout)
    {
        item = &cf_array_last(layout->items);
    }
    
    return item;
}

CF_V2 ui_layout_get_anchor(UI_Layout* layout)
{
    CF_V2 anchor = layout->aabb.min;
    if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Top))
    {
        anchor.y = layout->aabb.max.y;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Center))
    {
        anchor.y = cf_center(layout->aabb).y;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Bottom))
    {
        anchor.y = layout->aabb.min.y;
    }
    
    if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Left))
    {
        anchor.x = layout->aabb.min.x;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Center))
    {
        anchor.x = cf_center(layout->aabb).x;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Right))
    {
        anchor.x = layout->aabb.max.x;
    }
    
    return anchor;
}

void ui_draw_close_button(UI_Layout* layout, UI_Item* item)
{
    // draw normal boxes and text
    if (item->background_color.a > 0)
    {
        cf_draw_push_color(item->background_color);
        cf_draw_box_fill(item->aabb, item->corner_radius);
        cf_draw_pop_color();
    }
    
    if (item->border_color.a > 0)
    {
        cf_draw_push_color(item->border_color);
        cf_draw_box(item->aabb, item->border_thickness, item->corner_radius);
        cf_draw_pop_color();
    }
    
    {
        CF_V2 offset = cf_mul_v2_f(cf_extents(item->aabb), 0.05f);
        CF_V2 p0 = cf_add(cf_top_left(item->aabb), cf_v2(offset.x, -offset.y));
        CF_V2 p1 = cf_add(cf_bottom_left(item->aabb), cf_v2(offset.x, offset.y));
        CF_V2 p2 = cf_add(cf_bottom_right(item->aabb), cf_v2(-offset.x, offset.y));
        CF_V2 p3 = cf_add(cf_top_right(item->aabb), cf_v2(-offset.x, -offset.y));
        
        cf_draw_push_color(item->text_color);
        cf_draw_line(p0, p2, 1.0f);
        cf_draw_line(p1, p3, 1.0f);
        cf_draw_pop_color();
    }
}

b32 ui_layout_do_close_button()
{
    UI_Layout* layout = ui_peek_layout();
    b32 is_closed = false;
    if (layout)
    {
        if (ui_layout_has_var("is_close"))
        {
            UI_Layout_Var var = ui_layout_get_var("is_close");
            is_closed = var.b32_value;
        }
    }
    BIT_SET(layout->state, UI_Layout_State_Close_Button);
    
    return is_closed;
}

b32 ui_layout_tree_is_match(UI_Layout* layout, const char* name)
{
    UI* ui = &s_app->ui;
    b32 is_match = false;
    UI_Layout* next = layout;
    
    if (name && cf_string_find(name, layout->name) == name)
    {
        is_match = true;
    }
    
    return is_match;
}

b32 ui_layout_is_hovering(UI_Layout* layout)
{
    return ui_layout_tree_is_match(layout, s_app->ui.hover_layout_name);
}

b32 ui_layout_is_down(UI_Layout* layout)
{
    return ui_layout_tree_is_match(layout, s_app->ui.down_layout_name);
}

b32 ui_layout_is_scrollable_match_name(UI_Layout* layout, const char* name)
{
    UI* ui = &s_app->ui;
    b32 is_match = false;
    
    if (layout->scroll_direction && name)
    {
        if (layout->name == name)
        {
            is_match = true;
        }
        else if (cf_string_find(name, layout->name) == name)
        {
            if (cf_map_has(ui->layout_var_tables, name))
            {
                // searching by name isn't probably the best way to do this since the parent + child
                // could both have hoverables
                CF_MAP(UI_Layout_Var) var_table = cf_map_get(ui->layout_var_tables, name);
                if (cf_map_has(var_table, cf_sintern("is_scrollable")))
                {
                    UI_Layout_Var var = cf_map_get(var_table, cf_sintern("is_scrollable"));
                    if (!var.b32_value)
                    {
                        is_match = true;
                    }
                }
            }
        }
    }
    
    return is_match;
}

b32 ui_layout_is_hovering_scrollable(UI_Layout* layout)
{
    return ui_layout_is_scrollable_match_name(layout, s_app->ui.hover_layout_name);
}

b32 ui_layout_is_down_scrollable(UI_Layout* layout)
{
    return ui_layout_is_scrollable_match_name(layout, s_app->ui.down_layout_name);
}

b32 ui_layout_is_tooltip(UI_Layout* layout)
{
    UI* ui = &s_app->ui;
    b32 is_tooltip = false;
    UI_Layout* next = layout;
    while (next)
    {
        if (BIT_IS_SET(next->state, UI_Layout_State_Tooltip))
        {
            is_tooltip = true;
            break;
        }
        next = next->parent;
    }
    
    return is_tooltip;
}

b32 ui_is_any_layout_hovered()
{
    return s_app->ui.hover_layout_name != NULL;
}

CF_V2 ui_layout_get_scroll_offset(UI_Layout* layout)
{
    CF_V2 scroll = layout->scroll;
    CF_V2 scroll_extents = cf_v2(0, 0);
    CF_V2 scroll_offset = cf_v2(0, 0);
    CF_V2 extents = cf_extents(layout->usable_aabb);
    CF_V2 item_extents = cf_extents(layout->item_aabb);
    
    if (BIT_IS_SET(layout->scroll_direction, UI_Layout_Scroll_Vertical))
    {
        if (item_extents.y > extents.y)
        {
            scroll_extents.y = layout->usable_aabb.min.y - layout->item_aabb.min.y;
            if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Bottom))
            {
                scroll.y = 1.0f - scroll.y;
                scroll_extents.y = layout->usable_aabb.max.y - layout->item_aabb.max.y;
                scroll_offset.y = (extents.y - item_extents.y) * (1.0f - layout->scroll.y);
            }
        }
    }
    if (BIT_IS_SET(layout->scroll_direction, UI_Layout_Scroll_Horizontal))
    {
        if (item_extents.x > extents.x)
        {
            scroll_extents.x = layout->usable_aabb.max.x - layout->item_aabb.max.x;
            if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Right))
            {
                scroll_extents.x = layout->usable_aabb.min.x - layout->item_aabb.min.x;
                scroll_offset.x = (item_extents.x - extents.x) * layout->scroll.x;
            }
        }
    }
    
    if (cf_len_sq(scroll_extents))
    {
        scroll_offset = cf_mul_v2(scroll_extents, scroll);
    }
    
    return scroll_offset;
}

CF_V2 ui_layout_item_get_anchor(UI_Layout* layout, UI_Item* item)
{
    CF_V2 anchor = layout->aabb.min;
    if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Top))
    {
        anchor.y = item->aabb.max.y;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Center))
    {
        anchor.y = cf_center(item->aabb).y;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Bottom))
    {
        anchor.y = item->aabb.min.y;
    }
    
    if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Left))
    {
        anchor.x = item->aabb.min.x;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Center))
    {
        anchor.x = cf_center(item->aabb).x;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Right))
    {
        anchor.x = item->aabb.max.x;
    }
    
    return anchor;
}

UI_Layout* ui_tooltip_begin(CF_V2 size)
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_layout_begin("##ui_tooltip##");
    
    CF_V2 position = ui->input.mouse_position;
    f32 width = size.x <= 0 ? UI_WIDTH : size.x;
    f32 height = size.y <= 0 ? UI_HEIGHT : size.y;
    CF_Aabb aabb = cf_make_aabb_from_top_left(position, width, height);
    
    ui_layout_set_aabb(aabb);
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Top) | BIT(UI_Layout_Alignment_Horizontal_Left));
    ui_layout_set_direction(UI_Layout_Direction_Down);
    ui_push_item_alignment(UI_Item_Alignment_Left);
    
    BIT_SET(layout->state, UI_Layout_State_Tooltip);
    BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_X);
    BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
    
    return layout;
}

void ui_tooltip_end()
{
    ui_pop_item_alignment();
    ui_layout_end();
}

UI_Layout* ui_tooltip_vfmt(const char* fmt, va_list args)
{
    // allow previous item to trigger on hover
    {
        UI_Item* item = ui_layout_peek_item();
        if (item)
        {
            BIT_SET(item->state, UI_Item_State_Can_Hover);
        }
    }
    
    UI_Layout* layout = NULL;
    if (ui_item_is_hovered())
    {
        layout = ui_tooltip_begin(cf_v2(0, 0));
        ui_do_item_vfmt(fmt, args);
        ui_tooltip_end();
    }
    
    return layout;
}

UI_Layout* ui_tooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    UI_Layout* layout = ui_tooltip_vfmt(fmt, args);
    va_end(args);
    
    return layout;
}

inline CF_V2 ui_layout_get_next_visible_extents(UI_Layout* layout, UI_Item* item)
{
    CF_V2 extents = cf_v2(0, 0);
    UI_Item* next_visible_item = item + 1;
    while (next_visible_item <= &cf_array_last(layout->items))
    {
        if (BIT_IS_SET(next_visible_item->state, UI_Item_State_Visible))
        {
            extents = cf_extents(next_visible_item->aabb);
            break;
        }
        next_visible_item++;
    }
    return extents;
}

void ui_build_layout(UI_Layout* layout)
{
    //  @todo:  move this to be part of each layout so it can be modified
    f32 item_padding = layout->item_padding;
    
    CF_V2 cursor = cf_v2(0, 0);
    
    CF_V2 reset_cursor = cf_v2(0, 0);
    b32 has_title_bar = layout->title != NULL || BIT_IS_SET(layout->state, UI_Layout_State_Close_Button);
    f32 title_bar_height = 0.0f;
    CF_V2 title_text_size = cf_v2(0, 0);
    
    if (layout->title)
    {
        cf_push_font_size(layout->title_font_size);
        title_text_size = cf_text_size(layout->title, -1);
        title_bar_height = title_text_size.y + item_padding;
        cf_pop_font_size();
    }
    
    if (has_title_bar)
    {
        layout->usable_aabb.max.y -= title_bar_height;
    }
    
    // set cursor to anchor
    if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Top))
    {
        cursor.y = cf_top(layout->usable_aabb).y - item_padding;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Center))
    {
        cursor.y = cf_center(layout->usable_aabb).y;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Bottom))
    {
        cursor.y = cf_bottom(layout->usable_aabb).y + item_padding;
    }
    
    if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Left))
    {
        cursor.x = cf_left(layout->usable_aabb).x + item_padding;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Center))
    {
        cursor.x = cf_center(layout->usable_aabb).x;
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Horizontal_Right))
    {
        cursor.x = cf_right(layout->usable_aabb).x - item_padding;
    }
    
    reset_cursor = cursor;
    CF_V2 grid_item_min = cf_v2(F32_MAX, F32_MAX);
    CF_V2 grid_item_max = cf_v2(-F32_MAX, -F32_MAX);
    UI_Item* grid_item_begin = NULL;
    CF_Aabb prev_grid_aabb = { 0 };
    
    b32 shift_item_aabb_up = BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Bottom) && 
        layout->direction == UI_Layout_Direction_Down;
    b32 shift_item_aabb_down = BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Top) && 
        layout->direction == UI_Layout_Direction_Up;
    
    b32 do_same_line = false;
    UI_Item* prev_tiled_item = NULL;
    
    // builds out item_aabb and moves each item to auto layout position
    // builds out each section as a row/column, once each section reaches
    // end of the usable x/y direction then shifts to next row/column
    for (s32 index = 0; index < cf_array_count(layout->items); ++index)
    {
        UI_Item* item = layout->items + index;
        
        if (BIT_IS_SET(item->state, UI_Item_State_Same_Line) && prev_tiled_item)
        {
            do_same_line = true;
        }
        
        if (!BIT_IS_SET(item->state, UI_Item_State_Visible))
        {
            continue;
        }
        
        if (BIT_IS_SET(item->state, UI_Item_State_Skip_Auto_Tiling))
        {
            continue;
        }
        
        CF_V2 item_cursor = cursor;
        
        //  @todo:  currently only handles horizontal same line
        // @same_line
        if (do_same_line)
        {
            CF_V2 same_line_direction = cf_v2(1.0f, 0.0f);
            CF_V2 position = cf_top_right(prev_tiled_item->aabb);
            if (layout->grid_direction == UI_Layout_Direction_Left)
            {
                same_line_direction.x = -1.0f;
                position = cf_top_left(prev_tiled_item->aabb);
            }
            
            item_cursor = cf_add(position, cf_mul_v2_f(same_line_direction, item_padding));
        }
        else
        {
            // only stash beginning of a row/column if not doing same line items
            if (grid_item_begin == NULL)
            {
                grid_item_begin = item;
            }
        }
        
        CF_V2 extents = cf_extents(item->aabb);
        if (item->layout)
        {
            // if child layout has any Fit_To_Item_Aabb* then account for the item->aabb size change
            if (ui_layout_has_var_ex(item->layout, "aabb"))
            {
                UI_Layout_Var var = ui_layout_get_var_ex(item->layout, "aabb");
                
                // all layouts are AABB from top left (0,0) to (width, -height)
                if (BIT_IS_SET(item->layout->state, UI_Layout_State_Fit_To_Item_Aabb_X))
                {
                    extents.x = var.aabb.max.x - var.aabb.min.x + item->layout->item_padding;
                    item->aabb.min.x = -extents.x;
                }
                if (BIT_IS_SET(item->layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y))
                {
                    extents.y = var.aabb.max.y - var.aabb.min.y + item->layout->item_padding;
                    item->aabb.min.y = -extents.y;
                }
            }
        }
        
        switch (item->alignment)
        {
            case UI_Item_Alignment_Center:
            {
                item_cursor.x -= extents.x * 0.5f;
                break;
            }
            case UI_Item_Alignment_Right:
            {
                item_cursor.x -= extents.x; 
                break;
            }
        }
        
        if (layout->direction == UI_Layout_Direction_Up)
        {
            item_cursor.y += extents.y;
        }
        
        ui_move_item(item, item_cursor);
        
        prev_tiled_item = item;
        // used to build column/row aabb to determine when to shift to next column/row
        grid_item_min = cf_min(grid_item_min, item->aabb.min);
        grid_item_max = cf_max(grid_item_max, item->aabb.max);
        
        if (do_same_line)
        {
            do_same_line = false;
            // don't do grid alignment stuff while same same line
            continue;
        }
        
        b32 shift_cursor = false;
        
        // move cursor for next item
        CF_V2 next_visible_extents = ui_layout_get_next_visible_extents(layout, item);
        switch (layout->direction)
        {
            // up and down will default to move right whenever 
            case UI_Layout_Direction_Up:
            {
                cursor.y = cf_top(item->aabb).y + item_padding;
                
                // @grid
                // handle grid positioning, make sure there's enough space for next item before shifting over
                if (layout->grid_direction != UI_Layout_Direction_None && 
                    cursor.y > cf_top(layout->usable_aabb).y - next_visible_extents.y)
                {
                    cursor.y = reset_cursor.y;
                    shift_cursor = true;
                }
                break;
            }
            case UI_Layout_Direction_Down:
            {
                cursor.y = cf_bottom(item->aabb).y - item_padding;
                
                // @grid
                if (layout->grid_direction != UI_Layout_Direction_None && 
                    cursor.y < cf_bottom(layout->usable_aabb).y + next_visible_extents.y)
                {
                    cursor.y = reset_cursor.y;
                    shift_cursor = true;
                }
                break;
            }
            case UI_Layout_Direction_Left:
            {
                cursor.x = cf_left(item->aabb).x - item_padding;
                if (item->alignment == UI_Item_Alignment_Center)
                {
                    cursor.x -= extents.x * 0.5f;
                }
                
                // @grid
                if (layout->grid_direction != UI_Layout_Direction_None && 
                    cursor.x < cf_left(layout->usable_aabb).x + next_visible_extents.x)
                {
                    cursor.x = reset_cursor.x;
                    shift_cursor = true;
                }
                break;
            }
            case UI_Layout_Direction_Right:
            {
                cursor.x = cf_right(item->aabb).x + item_padding;
                if (item->alignment == UI_Item_Alignment_Center)
                {
                    cursor.x += extents.x * 0.5f;
                }
                
                // @grid
                if (layout->grid_direction != UI_Layout_Direction_None && 
                    cursor.x > cf_right(layout->usable_aabb).x - next_visible_extents.x)
                {
                    cursor.x = reset_cursor.x;
                    shift_cursor = true;
                }
                break;
            }
        }
        
        // @grid
        // next cursor is outside of bounds and needs to move 
        if (shift_cursor)
        {
            switch (layout->grid_direction)
            {
                // up and down will default to move right whenever 
                case UI_Layout_Direction_Up:
                {
                    cursor.y = grid_item_max.y + item_padding;
                    break;
                }
                case UI_Layout_Direction_Down:
                {
                    cursor.y = grid_item_min.y - item_padding;
                    break;
                }
                case UI_Layout_Direction_Left:
                {
                    cursor.x = grid_item_min.x - item_padding;
                    break;
                }
                case UI_Layout_Direction_Right:
                {
                    cursor.x = grid_item_max.x + item_padding;
                    break;
                }
            }
            
            CF_Aabb grid_aabb = cf_make_aabb(grid_item_min, grid_item_max);
            
            // check if item alignments are causing any overlaps with current column or row of items
            // if so, adjust all of the items by the distance between aabbs
            if (cf_overlaps(grid_aabb, prev_grid_aabb))
            {
                CF_V2 grid_offset = cf_v2(0, 0);
                if (layout->grid_direction == UI_Layout_Direction_Up)
                {
                    grid_offset.y += cf_top(prev_grid_aabb).y - cf_bottom(grid_aabb).y;
                }
                else if (layout->grid_direction == UI_Layout_Direction_Down)
                {
                    grid_offset.y += cf_top(grid_aabb).y - cf_bottom(prev_grid_aabb).y;
                }
                else if (layout->grid_direction == UI_Layout_Direction_Left)
                {
                    grid_offset.x -= cf_right(grid_aabb).x - cf_left(prev_grid_aabb).x;
                }
                else if (layout->grid_direction == UI_Layout_Direction_Right)
                {
                    grid_offset.x += cf_right(prev_grid_aabb).x - cf_left(grid_aabb).x;
                }
                
                grid_item_min = cf_v2(F32_MAX, F32_MAX);
                grid_item_max = cf_v2(-F32_MAX, -F32_MAX);
                
                UI_Item* grid_item = grid_item_begin;
                while (grid_item <= item)
                {
                    if (BIT_IS_SET(grid_item->state, UI_Item_State_Visible) && !BIT_IS_SET(grid_item->state, UI_Item_State_Skip_Auto_Tiling))
                    {
                        ui_move_item(grid_item, grid_offset);
                        
                        grid_item_min = cf_min(grid_item_min, grid_item->aabb.min);
                        grid_item_max = cf_max(grid_item_max, grid_item->aabb.max);
                    }
                    
                    grid_item++;
                }
                
                // make sure to adjust cusror otherwise this will drift over some amount of columns / rows
                cursor = cf_add(cursor, grid_offset);
            }
            
            prev_grid_aabb = cf_make_aabb(grid_item_min, grid_item_max);
            
            grid_item_min = cf_v2(F32_MAX, F32_MAX);
            grid_item_max = cf_v2(-F32_MAX, -F32_MAX);
            grid_item_begin = NULL;
        }
    }
    
    // @grid
    // check if last column or row of items overlaps with previous to adjust positions
    {
        CF_Aabb grid_aabb = cf_make_aabb(grid_item_min, grid_item_max);
        if (cf_overlaps(grid_aabb, prev_grid_aabb))
        {
            CF_V2 grid_offset = cf_v2(0, 0);
            if (layout->grid_direction == UI_Layout_Direction_Up)
            {
                grid_offset.y += cf_top(prev_grid_aabb).y - cf_bottom(grid_aabb).y;
            }
            else if (layout->grid_direction == UI_Layout_Direction_Down)
            {
                grid_offset.y += cf_top(grid_aabb).y - cf_bottom(prev_grid_aabb).y;
            }
            else if (layout->grid_direction == UI_Layout_Direction_Left)
            {
                grid_offset.x -= cf_right(grid_aabb).x - cf_left(prev_grid_aabb).x;
            }
            else if (layout->grid_direction == UI_Layout_Direction_Right)
            {
                grid_offset.x += cf_right(prev_grid_aabb).x - cf_left(grid_aabb).x;
            }
            
            ui_move_items(grid_item_begin, &cf_array_last(layout->items), grid_offset);
        }
    }
    
    // @grid
    // calculate item bounds for scrolling
    {
        CF_V2 item_min = cf_v2(F32_MAX, F32_MAX);
        CF_V2 item_max = cf_v2(-F32_MAX, -F32_MAX);
        for (s32 index = 0; index < cf_array_count(layout->items); ++index)
        {
            UI_Item* item = layout->items + index;
            if (!BIT_IS_SET(item->state, UI_Item_State_Visible))
            {
                continue;
            }
            
            if (BIT_IS_SET(item->state, UI_Item_State_Skip_Auto_Tiling))
            {
                continue;
            }
            
            item_min = cf_min(item_min, item->aabb.min);
            item_max = cf_max(item_max, item->aabb.max);
        }
        layout->item_aabb = cf_make_aabb(item_min, item_max);
    }
    
    // if alignment is BOTTOM and direction is DOWN then shift these up to fit layout view
    if (shift_item_aabb_up)
    {
        CF_V2 offset = cf_v2(0, cf_extents(layout->item_aabb).y);
        layout->item_aabb = move_aabb(layout->item_aabb, offset);
        ui_move_items(layout->items, &cf_array_last(layout->items), offset);
    }
    else if (shift_item_aabb_down)
    {
        // if alignment is TOP and direction is UP then shift these down to fit layout view
        CF_V2 offset = cf_v2(0, -cf_extents(layout->item_aabb).y);
        layout->item_aabb = move_aabb(layout->item_aabb, offset);
        ui_move_items(layout->items, &cf_array_last(layout->items), offset);
    }
    else if (BIT_IS_SET(layout->alignment, UI_Layout_Alignment_Vertical_Center))
    {
        // adjust center alignment otherwise it's slightly further down
        CF_V2 usable_center = cf_center(layout->usable_aabb);
        CF_V2 item_center = cf_center(layout->item_aabb);
        CF_V2 offset = cf_v2(0, usable_center.y - item_center.y);
        layout->item_aabb = move_aabb(layout->item_aabb, offset);
        ui_move_items(layout->items, &cf_array_last(layout->items), offset);
    }
    
    if (BIT_IS_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_X))
    {
        // try to keep this at least as wide as a title
        CF_V2 item_extents = cf_extents(layout->item_aabb);
        if (item_extents.x < title_text_size.x)
        {
            layout->item_aabb.max.x += title_text_size.x - item_extents.x;
        }
        
        layout->aabb.max.x = layout->item_aabb.max.x;
        layout->aabb.min.x = layout->item_aabb.min.x;
        layout->usable_aabb.max.x = layout->item_aabb.max.x;
        layout->usable_aabb.min.x = layout->item_aabb.min.x;
    }
    
    if (BIT_IS_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y))
    {
        // try to keep this at least as wide as a title
        layout->aabb.max.y = layout->item_aabb.max.y;
        layout->aabb.min.y = layout->item_aabb.min.y;
        layout->usable_aabb.max.y = layout->item_aabb.max.y;
        layout->usable_aabb.min.y = layout->item_aabb.min.y;
        
        if (has_title_bar)
        {
            layout->aabb.max.y += title_bar_height;
        }
    }
    
    // handles tooltip positioning to try to keep the layout within screen bounds
    if (BIT_IS_SET(layout->state, UI_Layout_State_Tooltip))
    {
        // fit item aabb to aabb and usable aabb
        CF_V2 offset = cf_v2(0.0f, cf_height(layout->item_aabb) + title_bar_height + item_padding);
        
        layout->item_aabb = move_aabb(layout->item_aabb, offset);
        layout->aabb = move_aabb(layout->aabb, offset);
        layout->usable_aabb = layout->item_aabb;
        
        ui_move_items(layout->items, &cf_array_last(layout->items), offset);
        
        // keep tooltip within screen bounds
        CF_Aabb inner_bounds = cf_make_aabb_from_top_left(cf_v2(0, UI_HEIGHT), UI_WIDTH, UI_HEIGHT);
        inner_bounds.max = cf_sub(inner_bounds.max, cf_extents(layout->aabb));
        CF_V2 position = cf_clamp_aabb_v2(inner_bounds, layout->aabb.min);
        offset = cf_sub(position, layout->aabb.min);
        
        if (cf_len_sq(offset))
        {
            layout->aabb = move_aabb(layout->aabb, offset);
            layout->usable_aabb = move_aabb(layout->usable_aabb, offset);
            layout->item_aabb = move_aabb(layout->item_aabb, offset);
            ui_move_items(layout->items, &cf_array_last(layout->items), offset);
        }
    }
    
    // handle scroll view
    if (layout->scroll_direction)
    {
        CF_V2 scroll_offset = ui_layout_get_scroll_offset(layout);
        
        if (cf_len_sq(scroll_offset))
        {
            ui_move_items(layout->items, &cf_array_last(layout->items), scroll_offset);
        }
    }
    
    // handle layout buttons
    {
        CF_Aabb title_bar_aabb = layout->aabb;
        title_bar_aabb.min.y = layout->usable_aabb.max.y;
        
        f32 border_thickness = ui_peek_border_thickness();
        f32 corner_radius = ui_peek_corner_radius();
        CF_V2 button_size = cf_v2(title_bar_height * 0.5f, title_bar_height * 0.5f);
        CF_V2 button_position = cf_top_right(title_bar_aabb);
        button_position.x -= button_size.x;
        
        if (BIT_IS_SET(layout->state, UI_Layout_State_Close_Button))
        {
            UI_Item* close_button = ui_make_item_ex(layout);
            BIT_SET(close_button->state, UI_Item_State_Ignore_Scissor);
            close_button->aabb = cf_make_aabb_from_top_left(button_position, button_size.x, button_size.y);
            close_button->text_aabb = close_button->aabb;
            close_button->interactable_aabb = close_button->aabb;
            close_button->custom_draw = ui_draw_close_button;
            
            if (ui_handle_button(close_button))
            {
                UI_Layout_Var var = { 0 };
                var.b32_value = true;
                ui_layout_set_var_ex(layout, "is_close", var);
            }
            
            button_position.x = close_button->aabb.min.x - item_padding;
        }
    }
    
    // update child layout position
    for (s32 index = 0; index < cf_array_count(layout->items); ++index)
    {
        UI_Item* item = layout->items + index;
        if (item->layout)
        {
            UI_Layout* child_layout = item->layout;
            child_layout->aabb = move_aabb(child_layout->aabb, ui_layout_item_get_anchor(layout, item));
            child_layout->usable_aabb = child_layout->aabb;
            // if parent is animating then hide the child layout
            if (cf_len_sq(layout->expand_animation_t) < 2.0f)
            {
                child_layout->expand_animation_t = cf_v2(0, 0);
            }
        }
    }
    
    // fix scroll bar positions
    for (s32 index = 0; index < cf_array_count(layout->items); ++index)
    {
        UI_Item* item = layout->items + index;
        if (BIT_IS_SET(item->state, UI_Item_State_Scrollbar))
        {
            ui_move_item(item, layout->aabb.min);
        }
    }
    
    // cache aabb sizes for anything that needs it
    {
        // aabb
        {
            UI_Layout_Var var = { 0 };
            var.aabb = layout->aabb;
            ui_layout_set_var_ex(layout, "aabb", var);
        }
        // item_aabb
        {
            UI_Layout_Var var = { 0 };
            var.aabb = layout->item_aabb;
            ui_layout_set_var_ex(layout, "item_aabb", var);
        }
        // usable_aabb
        {
            UI_Layout_Var var = { 0 };
            var.aabb = layout->usable_aabb;
            ui_layout_set_var_ex(layout, "usable_aabb", var);
        }
    }
}

void ui_build_layouts()
{
    UI* ui = &s_app->ui;
    
    s32 count = 0;
    s32 old_count = 0;
    FOREACH_LIST(node, &ui->layout_pool.active_list)
    {
        UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
        ui_build_layout(layout);
        ++old_count;
    }
    
    // handle text effect on hover
    if (ui->hover_layout_name)
    {
        // check for any on hover for any keywords
        FOREACH_LIST(node, &ui->layout_pool.active_list)
        {
            UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
            if (ui_layout_is_hovering(layout))
            {
                for (s32 index = 0; index < cf_array_count(layout->items); ++index)
                {
                    UI_Item* item = layout->items + index;
                    if (item->text)
                    {
                        if (cf_contains_aabb(layout->usable_aabb, item->aabb))
                        {
                            cf_push_font_size(item->font_size);
                            cf_text_get_markup_info(&ui_text_markup_info, item->text, cf_top_left(item->text_aabb), -1);
                            cf_pop_font_size();
                        }
                    }
                }
            }
            
            ++count;
        }
        
        // a on hover happens, build the new on hovers
        if (count > old_count)
        {
            FOREACH_LIST(node, &ui->layout_pool.active_list)
            {
                UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
                --count;
                if (count <= 0)
                {
                    ui_build_layout(layout);
                }
            }
        }
    }
}

void ui_animate_layouts()
{
    UI* ui = &s_app->ui;
    
    FOREACH_LIST(node, &ui->layout_pool.active_list)
    {
        UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
        
        CF_V2 t = layout->expand_animation_t;
        if (t.x == 1.0f && t.y == 1.0f)
        {
            continue;
        }
        
        CF_V2 half_extents = cf_half_extents(layout->aabb);
        CF_V2 center = cf_center(layout->aabb);
        layout->aabb = cf_make_aabb_center_half_extents(center, cf_mul(half_extents, t));
        
        for (s32 index = 0; index < cf_array_count(layout->items); ++index)
        {
            UI_Item* item = layout->items + index;
            BIT_UNSET(item->state, UI_Item_State_Visible);
            BIT_UNSET(item->state, UI_Item_State_Interactable);
            BIT_UNSET(item->state, UI_Item_State_Can_Hover);
        }
    }
}

//  @note:  currently only handles mouse input
void ui_handle_input()
{
    UI* ui = &s_app->ui;
    UI_Input* input = &ui->input;
    
    ui->hover_layout_name = NULL;
    ui->hover_hash = 0;
    
    CF_V2 mouse_position = input->mouse_position;
    b32 just_pressed = false;
    CF_V2 layout_mouse_offset = cf_v2(0, 0);
    const char* item_hover_layout_name = NULL;
    UI_Layout* hover_layout = NULL;
    
    FOREACH_LIST(node, &ui->layout_pool.active_list)
    {
        UI_Layout* layout = CF_LIST_HOST(UI_Layout, node, node);
        if (!cf_contains_point(layout->aabb, mouse_position) || BIT_IS_SET(layout->state, UI_Layout_State_Tooltip))
        {
            continue;
        }
        
        for (s32 index = 0; index < cf_array_count(layout->items); ++index)
        {
            UI_Item* item = layout->items + index;
            
            if (!BIT_IS_SET(item->state, UI_Item_State_Ignore_Scissor))
            {
                if (!cf_contains_point(layout->usable_aabb, mouse_position))
                {
                    continue;
                }
            }
            
            if (cf_contains_point(item->interactable_aabb, mouse_position))
            {
                ui->debug_hover_hash = item->hash;
                
                if (BIT_IS_SET(item->state, UI_Item_State_Interactable) || 
                    BIT_IS_SET(item->state, UI_Item_State_Can_Hover))
                {
                    ui->hover_hash = item->hash;
                    item_hover_layout_name = layout->name;
                }
            }
        }
        
        ui->hover_layout_name = layout->name;
        ui->hover_layout_position = mouse_position;
        layout_mouse_offset = cf_sub(mouse_position, ui_layout_get_anchor(layout));
        hover_layout = layout;
    }
    
    // currently hovering over a layout that's ontop of hovered interactable item, ignore item
    if (item_hover_layout_name != ui->hover_layout_name)
    {
        ui->hover_hash = 0;
    }
    
    b32 select = cf_binding_consume_press(input->select);
    
    if (ui->hover_hash)
    {
        UI_Item* item = cf_map_get(ui->item_map, ui->hover_hash);
        if (BIT_IS_SET(item->state, UI_Item_State_Interactable))
        {
            if (select)
            {
                ui->down_hash = ui->hover_hash;
                just_pressed = true;
            }
            
            if (cf_binding_released(input->select))
            {
                if (ui->down_hash == ui->hover_hash)
                {
                    ui->release_hash = ui->hover_hash;
                }
            }
        }
    }
    
    if (ui->hover_layout_name && ui->down_hash == 0)
    {
        if (select)
        {
            ui->down_layout_name = ui->hover_layout_name;
            ui->down_layout_position = ui->hover_layout_position;
            ui->down_layout_offset = layout_mouse_offset;
            
            UI_Layout* root = hover_layout;
            while (root->parent)
            {
                root = root->parent;
            }
            
            ui->down_root_layout_position = ui->hover_layout_position;
            ui->down_root_layout_offset = cf_sub(mouse_position, ui_layout_get_anchor(root));
        }
    }
    
    if (cf_binding_released(input->select))
    {
        ui->down_hash = 0;
        ui->down_layout_name = NULL;
    }
    
    if (ui->hover_hash)
    {
        UI_Item* item = cf_map_get(ui->item_map, ui->hover_hash);
        CF_V2 extents = cf_extents(item->interactable_aabb);
        CF_V2 normalize_position = cf_v2(0, 0);
        normalize_position.x = (mouse_position.x - item->interactable_aabb.min.x) / extents.x;
        normalize_position.y = (mouse_position.y - item->interactable_aabb.min.y) / extents.y;
        
        ui->hover_normalize_position = normalize_position;
        ui->hover_position = mouse_position;
        ui->hover_offset = cf_sub(mouse_position, item->interactable_aabb.min);
    }
    
    if (ui->down_hash)
    {
        if (cf_map_has(ui->item_map, ui->down_hash))
        {
            UI_Item* item = cf_map_get(ui->item_map, ui->down_hash);
            CF_V2 extents = cf_extents(item->interactable_aabb);
            CF_V2 normalize_position = cf_v2(0, 0);
            normalize_position.x = (mouse_position.x - item->interactable_aabb.min.x) / extents.x;
            normalize_position.y = (mouse_position.y - item->interactable_aabb.min.y) / extents.y;
            
            ui->down_normalize_position = normalize_position;
            ui->down_position = mouse_position;
            ui->down_offset = cf_sub(mouse_position, item->interactable_aabb.min);
        }
        else
        {
            ui->down_hash = 0;
        }
    }
}

void ui_consume_release()
{
    s_app->ui.release_hash = 0;
}

void ui_reset_input_hashes()
{
    UI* ui = &s_app->ui;
    ui->hover_hash = 0;
    ui->down_hash = 0;
    ui->release_hash = 0;
}

void ui_move_item(UI_Item* item, CF_V2 offset)
{
    item->aabb = move_aabb(item->aabb, offset);
    item->text_aabb = move_aabb(item->text_aabb, offset);
    item->interactable_aabb = move_aabb(item->interactable_aabb, offset);
}

void ui_move_items(UI_Item* begin, UI_Item* end, CF_V2 offset)
{
    UI_Item* item = begin;
    while (item <= end)
    {
        if (BIT_IS_SET(item->state, UI_Item_State_Visible) && !BIT_IS_SET(item->state, UI_Item_State_Skip_Auto_Tiling))
        {
            ui_move_item(item, offset);
        }
        
        item++;
    }
}

b32 ui_item_is_hovered()
{
    UI_Item* item = ui_layout_peek_item();
    return item && item->hash == s_app->ui.hover_hash;
}

// crappy hash but enough
u64 ui_layout_get_hash(UI_Layout* layout)
{
    u64 hash = layout->item_hash;
    u64 item_count = cf_array_count(layout->items) + 1;
    layout->item_hash ^= cf_fnv1a(&item_count, (s32)sizeof(item_count));
    return hash;
}

UI_Item* ui_make_item_ex(UI_Layout* layout)
{
    UI* ui = &s_app->ui;
    UI_Item* item = NULL;
    if (layout)
    {
        UI_Item new_item = { 0 };
        BIT_SET(new_item.state, UI_Item_State_Visible);
        
        new_item.font_size = cf_peek_font_size();
        new_item.alignment = ui_peek_item_alignment();
        new_item.text_color = ui_peek_text_color();
        new_item.text_shadow_color = ui_peek_text_shadow_color();
        new_item.background_color = ui_peek_background_color();
        new_item.border_color = ui_peek_border_color();
        new_item.border_thickness = ui_peek_border_thickness();
        new_item.corner_radius = ui_peek_corner_radius();
        new_item.word_wrap = ui_peek_word_wrap();
        
        cf_array_push(layout->items, new_item);
        item = &cf_array_last(layout->items);
        {
            item->hash = ui_layout_get_hash(layout);
            cf_map_set(ui->item_map, item->hash, item);
        }
    }
    
    return item;
}

UI_Item* ui_make_item()
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_peek_layout();
    return ui_make_item_ex(layout);
}

UI_Item* ui_do_item_vfmt(const char* fmt, va_list args)
{
    const char* text = scratch_vfmt(fmt, args);
    
    UI_Item* item = ui_make_item();
    item->text = text;
    
    if (item->word_wrap)
    {
        UI_Layout* layout = ui_peek_layout();
        cf_push_text_wrap_width(cf_extents(layout->usable_aabb).x);
    }
    
    CF_V2 text_size = cf_text_size(text, -1);
    
    item->aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), text_size.x, text_size.y);
    item->text_aabb = item->aabb;
    item->interactable_aabb = item->aabb;
    
    if (item->word_wrap)
    {
        cf_pop_text_wrap_width();
    }
    
    return item;
}

void ui_do_text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ui_do_item_vfmt(fmt, args);
    va_end(args);
}

void ui_handle_interactable_colors(UI_Item* item)
{
    UI* ui = &s_app->ui;
    
    item->background_color = ui_peek_idle_background_color();
    item->border_color = ui_peek_idle_border_color();
    item->text_color = ui_peek_idle_text_color();
    item->text_shadow_color = ui_peek_idle_text_shadow_color();
    
    if (item->hash == ui->hover_hash)
    {
        item->background_color = ui_peek_hover_background_color();
        item->border_color = ui_peek_hover_border_color();
        item->text_color = ui_peek_hover_text_color();
        item->text_shadow_color = ui_peek_hover_text_shadow_color();
    }
    
    if (item->hash == ui->down_hash)
    {
        item->background_color = ui_peek_pressed_background_color();
        item->border_color = ui_peek_pressed_border_color();
        item->text_color = ui_peek_pressed_text_color();
        item->text_shadow_color = ui_peek_pressed_text_shadow_color();
    }
    
    if (!BIT_IS_SET(item->state, UI_Item_State_Interactable))
    {
        item->background_color = ui_peek_disabled_color();
    }
}

b32 ui_handle_button(UI_Item* item)
{
    if (!ui_peek_is_disabled())
    {
        BIT_SET(item->state, UI_Item_State_Interactable);
    }
    ui_handle_interactable_colors(item);
    
    b32 clicked = false;
    
    if (item->hash == s_app->ui.release_hash)
    {
        ui_consume_release();
        clicked = true;
    }
    
    return clicked;
}

b32 ui_do_button_vfmt(const char* fmt, va_list args)
{
    UI_Item* item = ui_do_item_vfmt(fmt, args);
    return ui_handle_button(item);
}

b32 ui_do_button(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    b32 clicked = ui_do_button_vfmt(fmt, args);
    va_end(args);
    
    return clicked;
}

void ui_draw_checkbox(UI_Layout* layout, UI_Item* item)
{
    CF_Aabb aabb = item->aabb;
    CF_Color border_color = item->border_color;
    CF_Color background_color = item->background_color;
    if (border_color.a == 0.0f)
    {
        border_color = cf_color_white();
    }
    if (background_color.a == 0.0f)
    {
        background_color = cf_color_white();
    }
    
    cf_draw_push_color(border_color);
    cf_draw_box(aabb, item->border_thickness, item->corner_radius);
    cf_draw_pop_color();
    
    b32* value = (b32*)item->custom_data;
    
    CF_V2 offset = cf_half_extents(item->aabb);
    offset = cf_mul_v2_f(offset, *value ? -0.25f : -0.9f);
    aabb = cf_expand_aabb(aabb, offset);
    
    cf_draw_push_color(background_color);
    cf_draw_box_fill(aabb, item->corner_radius);
    cf_draw_pop_color();
}

b32 ui_do_checkbox(b32* value)
{
    UI_Item* item = ui_make_item();
    
    item->aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), item->font_size, item->font_size);
    item->text_aabb = item->aabb;
    item->interactable_aabb = item->aabb;
    
    item->custom_draw = ui_draw_checkbox;
    item->custom_data = scratch_copy(value, sizeof(*value));
    item->custom_size = sizeof(*value);
    
    b32 clicked = ui_handle_button(item);
    
    if (clicked)
    {
        *value = !*value;
    }
    
    return clicked;
}

b32 ui_do_checkbox_bit8(b8* mask, s32 index)
{
    b64 mask_64 = (b64)mask;
    b32 changed = ui_do_checkbox_bit64(&mask_64, index);
    BIT_UNSET(*mask, index);
    if (BIT_IS_SET(mask_64, index))
    {
        BIT_SET(*mask, index);
    }
    return changed;
}

b32 ui_do_checkbox_bit16(b16* mask, s32 index)
{
    b64 mask_64 = (b64)mask;
    b32 changed = ui_do_checkbox_bit64(&mask_64, index);
    BIT_UNSET(*mask, index);
    if (BIT_IS_SET(mask_64, index))
    {
        BIT_SET(*mask, index);
    }
    return changed;
}

b32 ui_do_checkbox_bit32(b32* mask, s32 index)
{
    b64 mask_64 = (b64)mask;
    b32 changed = ui_do_checkbox_bit64(&mask_64, index);
    BIT_UNSET(*mask, index);
    if (BIT_IS_SET(mask_64, index))
    {
        BIT_SET(*mask, index);
    }
    return changed;
}

b32 ui_do_checkbox_bit64(b64* mask, s32 index)
{
    b32 flag = BIT_IS_SET(*mask, index);
    s32 changed = ui_do_checkbox(&flag);
    BIT_UNSET(*mask, index);
    if (flag)
    {
        BIT_SET(*mask, index);
    }
    return changed;
}

void ui_draw_slider(UI_Layout* layout, UI_Item* item)
{
    CF_Color background_color = item->background_color;
    CF_Color bar_color = background_color;
    
    background_color.a *= 0.25f;
    
    if (background_color.a > 0)
    {
        cf_draw_push_color(background_color);
        cf_draw_box_fill(item->aabb, item->corner_radius);
        cf_draw_pop_color();
    }
    
    if (item->border_color.a > 0)
    {
        cf_draw_push_color(item->border_color);
        cf_draw_box(item->aabb, item->border_thickness, item->corner_radius);
        cf_draw_pop_color();
    }
    
    {
        cf_draw_push_color(bar_color);
        cf_draw_box_fill(item->text_aabb, item->corner_radius);
        cf_draw_pop_color();
    }
}

b32 ui_handle_scrollbar(UI_Item* item, f32* value, f32 min, f32 max, f32 rate)
{
    UI* ui = &s_app->ui;
    
    ui_handle_interactable_colors(item);
    b32 changed = false;
    
    CF_V2 extents = cf_extents(item->aabb);
    b32 is_horizontal = extents.x > extents.y;
    CF_V2 knob_extents = cf_extents(item->text_aabb);
    
    if (item->hash == ui->down_hash)
    {
        f32 range = max - min;
        f32 ticks = cf_floor(range / cf_max(rate, 0.001f));
        
        f32 value01 = 0.0f;
        
        if (is_horizontal)
        {
            value01 = cf_clamp01(ui->down_normalize_position.x);
        }
        else
        {
            value01 = 1.0f - cf_clamp01(ui->down_normalize_position.y);
        }
        
        f32 next_value = cf_remap01(cf_floor(value01 * ticks) / ticks, min, max);
        changed = cf_abs(*value - next_value) > 1e-7f;
        *value = next_value;
    }
    
    if (item->hash == ui->release_hash)
    {
        ui_consume_release();
    }
    
    CF_V2 offset = cf_v2(0, 0);
    if (is_horizontal)
    {
        offset.x = cf_remap(*value, min, max, 0.0f, 1.0f) * cf_width(item->interactable_aabb) - knob_extents.x * 0.5f;
        offset.x = cf_clamp(offset.x, item->interactable_aabb.min.x, item->interactable_aabb.max.x - knob_extents.x);
    }
    else
    {
        offset.y = cf_remap(*value, min, max, 0.0f, -1.0f) * cf_height(item->interactable_aabb) + knob_extents.x * 0.5f;
        offset.y = cf_clamp(offset.y, item->interactable_aabb.min.y + knob_extents.y, item->interactable_aabb.max.y);
    }
    item->text_aabb = move_aabb(item->text_aabb, offset);
    
    return changed;
}

b32 ui_do_scrollbar_ex(f32* value, f32 min, f32 max, f32 rate, f32 scale, b32 is_horizontal)
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_peek_layout();
    
    UI_Item* item = ui_make_item();
    f32 knob_size = item->font_size;
    
    CF_V2 position = cf_v2(0, 0);
    CF_V2 aabb_extents = cf_v2(knob_size, knob_size);
    if (is_horizontal)
    {
        aabb_extents.x = cf_extents(layout->aabb).x * scale;
    }
    else
    {
        aabb_extents.y = cf_extents(layout->aabb).y * scale;
    }
    
    item->aabb = cf_make_aabb_from_top_left(position, aabb_extents.x, aabb_extents.y);
    item->text_aabb = cf_make_aabb_from_top_left(position, knob_size, knob_size);
    item->interactable_aabb = item->aabb;
    
    if (!ui_peek_is_disabled())
    {
        BIT_SET(item->state, UI_Item_State_Interactable);
    }
    BIT_SET(item->state, UI_Item_State_Scrollbar);
    item->custom_draw = ui_draw_slider;
    
    return ui_handle_scrollbar(item, value, min, max, rate);
}

void ui_draw_text_stretchy_slider(UI_Layout* layout, UI_Item* item)
{
    ui_draw_slider(layout, item);
    
    if (item->text)
    {
        if (item->word_wrap)
        {
            cf_push_text_wrap_width(cf_extents(layout->usable_aabb).x);
        }
        
        CF_V2 top_left = cf_top_left(item->text_aabb);
        CF_V2 shadow_top_left = cf_add(top_left, cf_v2(1, -1));
        
        cf_push_font_size(item->font_size);
        
        cf_draw_push_color(item->text_shadow_color);
        cf_draw_text(cf_text_without_markups(item->text), shadow_top_left, -1);
        cf_draw_pop_color();
        
        cf_draw_push_color(item->text_color);
        cf_draw_text(item->text, top_left, -1);
        cf_draw_pop_color();
        
        cf_pop_font_size();
        
        if (item->word_wrap)
        {
            cf_pop_text_wrap_width();
        }
    }
}

b32 ui_do_slider(f32* value, f32 min, f32 max, f32 rate)
{
    UI* ui = &s_app->ui;
    UI_Layout* layout = ui_peek_layout();
    
    UI_Item* item = ui_make_item();
    CF_V2 extents = cf_v2(cf_width(layout->usable_aabb) * 0.5f, item->font_size);
    
    item->aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), extents.x, extents.y);
    item->text_aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), 0, extents.y);
    item->interactable_aabb = item->aabb;
    
    if (!ui_peek_is_disabled())
    {
        BIT_SET(item->state, UI_Item_State_Interactable);
    }
    ui_handle_interactable_colors(item);
    item->custom_draw = ui_draw_text_stretchy_slider;
    
    b32 changed = false;
    if (item->hash == ui->down_hash)
    {
        f32 value01 = cf_clamp01(ui->down_normalize_position.x);
        f32 range = max - min;
        f32 ticks = cf_floor(range / cf_max(rate, 0.001f));
        
        f32 next_value = cf_remap01(cf_floor(value01 * ticks) / ticks, min, max);
        changed = cf_abs(*value - next_value) > 1e-7f;
        *value = next_value;
    }
    
    if (item->hash == ui->release_hash)
    {
        ui_consume_release();
    }
    
    item->text_aabb.max.x = cf_remap(*value, min, max, 0.0f, 1.0f) * extents.x;
    
    item->text = scratch_fmt("%.2f", *value);
    
    return changed;
}

// no default draw for sprite, this is to 
void ui_draw_sprite(UI_Layout* layout, UI_Item* item)
{
    if (item->background_color.a > 0)
    {
        cf_draw_push_color(item->background_color);
        cf_draw_box_fill(item->aabb, item->corner_radius);
        cf_draw_pop_color();
    }
    
    if (item->border_color.a > 0)
    {
        cf_draw_push_color(item->border_color);
        cf_draw_box(item->aabb, item->border_thickness, item->corner_radius);
        cf_draw_pop_color();
    }
    
    CF_Sprite* sprite = (CF_Sprite*)item->custom_data;
    sprite->transform.p = cf_center(item->aabb);
    cf_draw_sprite(sprite);
}

void ui_do_sprite_ex(CF_Sprite sprite, CF_V2 size)
{
    UI_Item* item = ui_make_item();
    
    item->aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), size.x, size.y);
    item->text_aabb = item->aabb;
    item->interactable_aabb = item->aabb;
    
    CF_Sprite* item_sprite = scratch_alloc(sizeof(sprite));
    *item_sprite = sprite;
    CF_V2 scale = cf_v2(size.x / sprite.w, size.y / sprite.h);
    item_sprite->scale = scale;
    
    item->custom_draw = ui_draw_sprite;
    item->custom_data = item_sprite;
    item->custom_size = sizeof(sprite);
}

void ui_do_sprite(CF_Sprite sprite)
{
    ui_do_sprite_ex(sprite, cf_v2((f32)sprite.w, (f32)sprite.h));
}

b32 ui_do_sprite_button(CF_Sprite sprite)
{
    ui_do_sprite(sprite);
    UI_Item* item = ui_layout_peek_item();
    
    return ui_handle_button(item);
}

b32 ui_do_tabs(const char** names, s32 count, s32* current)
{
    UI_Layout* layout = ui_peek_layout();
    CF_V2 extents = cf_extents(layout->aabb);
    extents.y = cf_peek_font_size() + layout->item_padding;
    
    b32 prev = *current;
    
    ui_child_layout_begin(extents);
    for (s32 index = 0; index < count; ++index)
    {
        if (prev == index)
        {
            ui_push_idle_background_color(ui_peek_hover_background_color());
        }
        
        if (ui_do_button(names[index]))
        {
            *current = index;
        }
        
        if (prev == index)
        {
            ui_pop_idle_background_color();
        }
        
        if (index + 1 < count)
        {
            ui_do_same_line();
        }
    }
    ui_child_layout_end();
    
    return prev != *current;
}

void ui_draw_color_wheel(UI_Layout* layout, UI_Item* item)
{
    if (item->background_color.a > 0)
    {
        cf_draw_push_color(item->background_color);
        cf_draw_box_fill(item->aabb, item->corner_radius);
        cf_draw_pop_color();
    }
    
    if (item->border_color.a > 0)
    {
        cf_draw_push_color(item->border_color);
        cf_draw_box(item->aabb, item->border_thickness, item->corner_radius);
        cf_draw_pop_color();
    }
    
    CF_V2 center = cf_center(item->aabb);
    CF_V2 extents = cf_extents(item->aabb);
    f32 radius = cf_min(extents.x, extents.y) * 0.5f;
    
    f32 picking_radius = radius * 0.1f;
    CF_Color color = *(CF_Color*)item->custom_data;
    CF_Color hsv = cf_rgb_to_hsv(color);
    
    f32 angle = hsv.r * CF_TAU;
    CF_V2 picking = cf_v2(radius * hsv.g * cf_cos(angle), radius * hsv.g * cf_sin(angle));
    picking = cf_add(picking, center);
    
    // draw radial color wheel
    {
        CF_V2 t1 = center;
        CF_Color c1 = cf_make_color_rgb_f(0.0f, 0.0f, hsv.b);
        c1 = cf_hsv_to_rgb(c1);
        
        // disable SDF so tri_colors gets used
        cf_draw_push_shape_aa(0);
        for (angle = 0.0f; angle < 360.0f; angle += 5.0f)
        {
            f32 a0 = angle * CF_PI / 180.0f;
            f32 a2 = (angle + 5.0f) * CF_PI / 180.0f;
            
            CF_V2 t0 = cf_from_angle(a0);
            CF_V2 t2 = cf_from_angle(a2);
            
            t0 = cf_mul_v2_f(t0, radius);
            t2 = cf_mul_v2_f(t2, radius);
            
            t0 = cf_add(t0, center);
            t2 = cf_add(t2, center);
            
            CF_Color c0 = cf_make_color_rgb_f(a0 / CF_TAU, 1.0f, 1.0f);
            CF_Color c2 = cf_make_color_rgb_f(a2 / CF_TAU, 1.0f, 1.0f);
            c0.b = hsv.b;
            c2.b = hsv.b;
            
            c0 = cf_hsv_to_rgb(c0);
            c2 = cf_hsv_to_rgb(c2);
            
            cf_draw_push_tri_colors(c0, c1, c2);
            cf_draw_tri_fill(t0, t1, t2, 0.0f);
            cf_draw_pop_tri_colors();
        }
        cf_draw_pop_shape_aa();
    }
    
    // draw current picking point
    {
        cf_draw_push_color(cf_color_black());
        cf_draw_circle2(picking, picking_radius, 1.0f);
        cf_draw_pop_color();
    }
}

b32 ui_do_color_wheel(CF_Color* color)
{
    UI* ui = &s_app->ui;
    
    CF_Color prev = *color;
    UI_Layout* layout = ui_child_layout_begin(cf_v2(250.0f, 100.0f));
    BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
    
    CF_Color hsv = cf_rgb_to_hsv(*color);
    u32 prev_hsv_value = cf_color_to_pixel(hsv).val;
    {
        ui_push_background_color(cf_color_grey());
        UI_Item* item = ui_make_item();
        BIT_SET(item->state, UI_Item_State_Interactable);
        item->aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), 200.0f, 200.0f);
        item->text_aabb = item->aabb;
        item->interactable_aabb = item->aabb;
        
        item->custom_draw = ui_draw_color_wheel;
        item->custom_data = color;
        item->custom_size = sizeof(*color);
        
        if (item->hash == ui->down_hash)
        {
            CF_V2 position = cf_clamp01(ui->down_normalize_position);
            position.x = cf_remap01(position.x, -1.0f, 1.0f);
            position.y = cf_remap01(position.y, -1.0f, 1.0f);
            
            f32 angle = cf_atan2(position.y, position.x);
            if (position.y < 0)
            {
                angle += CF_TAU;
            }
            
            hsv.r = angle / CF_TAU;
            hsv.g = cf_clamp01(cf_len(position));
        }
        
        ui_pop_background_color();
    }
    ui_child_layout_end();
    
    ui_do_same_line();
    layout = ui_child_layout_begin(cf_v2(250.0f, 100.0f));
    BIT_SET(layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
    {
        cf_push_font_size(24.0f);
        f32 hue = hsv.r * 360.0f;
        
        ui_do_text("H");
        ui_do_same_line();
        if (ui_do_slider(&hue, 0.0f, 360.0f, 1.0f))
        {
            hsv.r = hue / 360.0f;
        }
        
        ui_do_text("S");
        ui_do_same_line();
        ui_do_slider(&hsv.g, 0.0f, 1.0f, 0.01f);
        
        ui_do_text("V");
        ui_do_same_line();
        ui_do_slider(&hsv.b, 0.0f, 1.0f, 0.01f);
        cf_pop_font_size();
    }
    ui_child_layout_end();
    
    if (prev_hsv_value != cf_color_to_pixel(hsv).val)
    {
        f32 a = color->a;
        *color = cf_hsv_to_rgb(hsv);
        color->a = a;
    }
    
    return cf_color_to_pixel(prev).val != cf_color_to_pixel(*color).val;
}

typedef struct UI_Graph_Line_Data
{
    f32* values;
    s32 count;
    s32 select_index;
    f32 min;
    f32 max;
} UI_Graph_Line_Data;

void ui_draw_graph_line(UI_Layout* layout, UI_Item* item)
{
    CF_Aabb aabb = item->interactable_aabb;
    
    if (item->background_color.a > 0)
    {
        cf_draw_push_color(item->background_color);
        cf_draw_box_fill(aabb, item->corner_radius);
        cf_draw_pop_color();
    }
    
    if (item->border_color.a > 0)
    {
        cf_draw_push_color(item->border_color);
        cf_draw_box(aabb, item->border_thickness, item->corner_radius);
        cf_draw_pop_color();
    }
    
    UI_Graph_Line_Data* data = (UI_Graph_Line_Data*)item->custom_data;
    
    if (data->count < 1)
    {
        return;
    }
    
    CF_Color line_color = item->border_color;
    line_color.a = 1.0f;
    
    f32* values = data->values;
    f32 min = data->min;
    f32 max = data->max;
    f32 range = data->max - data->min;
    s32 walk_count = cf_max(data->count - 1, 1);
    
    // draw lines
    cf_draw_push_color(line_color);
    for (s32 index = 0; index < walk_count; ++index)
    {
        CF_V2 p0 = cf_v2(0, 0);
        CF_V2 p1 = cf_v2(0, 0);
        
        p0.x = cf_remap01((f32)index / walk_count, aabb.min.x, aabb.max.x);
        p0.y = cf_remap(values[index], data->min, data->max, aabb.min.y, aabb.max.y);
        
        p1.x = cf_remap01((f32)(index + 1) / walk_count, aabb.min.x, aabb.max.x);
        p1.y = cf_remap(values[index + 1], min, max, aabb.min.y, aabb.max.y);
        
        cf_draw_line(p0, p1, 1.0f);
    }
    cf_draw_pop_color();
    
    // draw current selected
    {
        CF_V2 p = cf_v2(0, 0);
        p.x = (f32)data->select_index / walk_count;
        p.y = cf_remap(values[data->select_index], data->min, data->max, 0.0f, 1.0f);
        
        p.x = cf_remap01(p.x, aabb.min.x, aabb.max.x);
        p.y = cf_remap01(p.y, aabb.min.y, aabb.max.y);
        
        cf_draw_push_color(line_color);
        cf_draw_circle_fill2(p, 2.0f);
        cf_draw_pop_color();
    }
    
    f32 font_size = item->font_size;
    char buffer[256];
    
    // max
    {
        CF_SNPRINTF(buffer, sizeof(buffer), "%.2f", max);
        
        CF_V2 text_position = cf_top_left(aabb);
        text_position.y += cf_text_height(buffer, -1);
        CF_V2 shadow_position = cf_add(text_position, cf_v2(1.0f, -1.0f));
        
        cf_push_font_size(font_size);
        
        cf_draw_push_color(item->text_shadow_color);
        cf_draw_text(buffer, shadow_position, -1);
        cf_draw_pop_color();
        
        cf_draw_push_color(item->text_color);
        cf_draw_text(buffer, text_position, -1);
        cf_draw_pop_color();
        
        cf_pop_font_size();
    }
    
    // min
    {
        CF_SNPRINTF(buffer, sizeof(buffer), "%.2f", min);
        
        CF_V2 text_position = cf_bottom_left(aabb);
        CF_V2 shadow_position = cf_add(text_position, cf_v2(1.0f, -1.0f));
        
        cf_push_font_size(font_size);
        
        cf_draw_push_color(item->text_shadow_color);
        cf_draw_text(buffer, shadow_position, -1);
        cf_draw_pop_color();
        
        cf_draw_push_color(item->text_color);
        cf_draw_text(buffer, text_position, -1);
        cf_draw_pop_color();
        
        cf_pop_font_size();
    }
}

b32 ui_do_graph_line(f32* values, s32 count, s32* select_index)
{
    UI* ui = &s_app->ui;
    
    f32 graph_height = cf_peek_font_size() * 2;
    f32 graph_width = cf_peek_font_size() * 8;
    
    f32 min = F32_MAX;
    f32 max = -F32_MAX;
    
    for (s32 index = 0; index < count; ++index)
    {
        min = cf_min(min, values[index]);
        max = cf_max(max, values[index]);
    }
    
    f32 range = max - min;
    max += range * 0.5f;
    min -= range * 0.5f;
    
    CF_Aabb aabb = cf_make_aabb_from_top_left(cf_v2(0, 0), graph_width, graph_height);
    UI_Item* item = ui_make_item();
    BIT_SET(item->state, UI_Item_State_Interactable);
    
    item->aabb = aabb;
    item->aabb.max.y += cf_peek_font_size();
    item->aabb.min.y -= cf_peek_font_size();
    
    item->text_aabb = aabb;
    item->interactable_aabb = aabb;
    item->font_size = cf_peek_font_size() * 0.5f;
    
    s32 peek_select_index = cf_max(count - 1, 0);
    if (select_index)
    {
        peek_select_index = *select_index;
    }
    
    s32 prev_index = peek_select_index;
    // input handling
    if (item->hash == ui->down_hash)
    {
        f32 x = cf_clamp01(ui->down_normalize_position.x);
        peek_select_index = (s32)(x * (count - 1));
        
        if (select_index)
        {
            *select_index = peek_select_index;
        }
    }
    
    UI_Graph_Line_Data* data = scratch_alloc(sizeof(UI_Graph_Line_Data));
    data->values = values;
    data->count = count;
    data->select_index = peek_select_index;
    data->min = min;
    data->max = max;
    
    item->custom_draw = ui_draw_graph_line;
    item->custom_data = data;
    item->custom_size = sizeof(*data);
    
    return prev_index != peek_select_index;
}

void ui_do_same_line()
{
    UI_Item* item = ui_make_item();
    BIT_UNSET(item->state, UI_Item_State_Visible);
    BIT_SET(item->state, UI_Item_State_Same_Line);
}

// misc

void ui_register_text_effect(const char* name, CF_TextEffectFn* draw_callback, CF_TextEffectFn* hover_callback)
{
    UI* ui = &s_app->ui;
    UI_Text_Effect_Callbacks callbacks = { 0 };
    callbacks.draw = draw_callback;
    callbacks.hover = hover_callback;
    
    name = cf_sintern(name);
    cf_text_effect_register(name, ui_text_fx_keyword);
    cf_map_set(ui->text_effect_map, name, callbacks);
}

void ui_debug_view()
{
    UI* ui = &s_app->ui;
    
    cf_push_font_size(24.0f);
    CF_Aabb layout_aabb = cf_make_aabb_from_top_left(cf_v2(UI_WIDTH * 0.75f, UI_HEIGHT * 0.25f), UI_WIDTH * 0.25f, UI_HEIGHT * 0.25f);
    UI_Layout* debug_layout = ui_layout_begin("Debug");
    ui_layout_set_title("Debug");
    ui_layout_set_aabb(layout_aabb);
    
    ui_layout_set_alignment(BIT(UI_Layout_Alignment_Vertical_Bottom) | BIT(UI_Layout_Alignment_Horizontal_Right));
    ui_layout_set_direction(UI_Layout_Direction_Down);
    ui_push_item_alignment(UI_Item_Alignment_Right);
    
    BIT_SET(debug_layout->state, UI_Layout_State_Fit_To_Item_Aabb_X);
    BIT_SET(debug_layout->state, UI_Layout_State_Fit_To_Item_Aabb_Y);
    
    UI_Layout* layout = NULL;
    
    for (s32 index = 0; index < cf_array_count(ui->layouts); ++index)
    {
        if (ui->layouts[index].name == ui->hover_layout_name)
        {
            layout = ui->layouts + index;
            break;
        }
    }
    
    if (layout)
    {
        layout->border_color = cf_color_magenta();
        layout->border_thickness = 2.0f;
        
        UI_Layout_Var var_aabb = { 0 };
        UI_Layout_Var var_item_aabb = { 0 };
        UI_Layout_Var var_usable_aabb = { 0 };
        var_aabb.aabb = layout->aabb;
        var_item_aabb.aabb = layout->item_aabb;
        var_usable_aabb.aabb = layout->usable_aabb;
        CF_V2 scroll = layout->scroll;
        
        if (ui_layout_has_var_ex(layout, "aabb"))
        {
            var_aabb = ui_layout_get_var_ex(layout, "aabb");
            var_item_aabb = ui_layout_get_var_ex(layout, "item_aabb");
            var_usable_aabb = ui_layout_get_var_ex(layout, "usable_aabb");
        }
        if (ui_layout_has_var_ex(layout, "scroll"))
        {
            UI_Layout_Var var = ui_layout_get_var_ex(layout, "scroll");
            scroll = var.v2_value;
        }
        
        // aabbaabbaabb
        ui_do_text("%s", layout->name);
        ui_do_text("min: %.2f, %.2f", var_aabb.aabb.min.x, var_aabb.aabb.min.y);
        ui_do_text("max: %.2f, %.2f", var_aabb.aabb.max.x, var_aabb.aabb.max.y);
        ui_do_text("item min: %.2f, %.2f", var_item_aabb.aabb.min.x, var_item_aabb.aabb.min.y);
        ui_do_text("item max: %.2f, %.2f", var_item_aabb.aabb.max.x, var_item_aabb.aabb.max.y);
        ui_do_text("usable min: %.2f, %.2f", var_usable_aabb.aabb.min.x, var_usable_aabb.aabb.min.y);
        ui_do_text("usable max: %.2f, %.2f", var_usable_aabb.aabb.max.x, var_usable_aabb.aabb.max.y);
        ui_do_text("scroll: %.2f, %.2f", scroll.x, scroll.y);
        ui_do_text("item: %X", ui->debug_hover_hash);
    }
    else
    {
        ui_do_text("None");
    }
    
    ui_pop_item_alignment();
    ui_layout_end();
    cf_pop_font_size();
}