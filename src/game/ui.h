#ifndef UI_H
#define UI_H

#ifndef UI_DEFAULT_FONT_SIZE
#define UI_DEFAULT_FONT_SIZE (48.0f)
#endif

#ifndef UI_WIDTH
#define UI_WIDTH GAME_WIDTH
#endif

#ifndef UI_HEIGHT
#define UI_HEIGHT GAME_HEIGHT
#endif

#ifndef UI_LAYOUT_CAPACITY
#define UI_LAYOUT_CAPACITY 256
#endif

#ifndef UI_LAYOUT_ITEM_MIN_CAPACITY
#define UI_LAYOUT_ITEM_MIN_CAPACITY 64
#endif

// all the states, direction, alignments etc here are all bits
typedef u32 UI_Item_State;
enum
{
    UI_Item_State_Visible = 0,
    UI_Item_State_Interactable = 1,
    UI_Item_State_Skip_Auto_Tiling = 2,
    UI_Item_State_Ignore_Scissor = 3,
    UI_Item_State_Can_Hover = 4,
    UI_Item_State_Scrollbar = 5,
    UI_Item_State_Same_Line = 6,
    
    // leave remaining 16 bits for user
};

// only horizontal alignments
typedef u8 UI_Item_Alignment;
enum
{
    UI_Item_Alignment_Left,
    UI_Item_Alignment_Center,
    UI_Item_Alignment_Right
};

typedef struct UI_Layout UI_Layout;
typedef struct UI_Item UI_Item;

typedef void (*ui_item_draw_fn)(UI_Layout* layout, UI_Item* item);

typedef struct UI_Item
{
    const char* text;
    f32 font_size;
    CF_Color text_color;
    CF_Color text_shadow_color;
    CF_Color background_color;
    CF_Color border_color;
    
    f32 border_thickness;
    f32 corner_radius;
    b32 word_wrap;
    
    CF_Aabb aabb;
    CF_Aabb text_aabb;
    CF_Aabb interactable_aabb;
    
    u64 hash;
    
    // when a layout has a child layout, this is a backwards reference for debugging
    UI_Layout* layout;
    
    ui_item_draw_fn custom_draw;
    void* custom_data;
    u64 custom_size;
    
    UI_Item_State state;
    UI_Item_Alignment alignment;
} UI_Item;

//  @note:  currently doing double left or double right won't work correctly, not sure if it's a case
//          that's really needed for now
//          example UI_Layout_Direction_Left and UI_Layout_Alignment_Left

// direction for items to be placed next
typedef u8 UI_Layout_Direction;
enum
{
    UI_Layout_Direction_None,
    UI_Layout_Direction_Up,
    UI_Layout_Direction_Down,
    UI_Layout_Direction_Left,
    UI_Layout_Direction_Right,
};

// layout alignment / origin
typedef u8 UI_Layout_Alignment;
enum
{
    UI_Layout_Alignment_Vertical_Top,
    UI_Layout_Alignment_Vertical_Center,
    UI_Layout_Alignment_Vertical_Bottom,
    UI_Layout_Alignment_Horizontal_Left,
    UI_Layout_Alignment_Horizontal_Center,
    UI_Layout_Alignment_Horizontal_Right,
};

typedef u8 UI_Layout_Scroll;
enum
{
    UI_Layout_Scroll_Vertical,
    UI_Layout_Scroll_Horizontal,
};

typedef u8 UI_Layout_State;
enum
{
    UI_Layout_State_Tooltip,
    UI_Layout_State_Fit_To_Item_Aabb_X,
    UI_Layout_State_Fit_To_Item_Aabb_Y,
    UI_Layout_State_Close_Button,
};

typedef struct UI_Layout
{
    const char* name;
    const char* title;
    dyna UI_Item* items;
    UI_Layout* parent;
    dyna UI_Layout** children;
    
    CF_Aabb aabb;
    // area that items can be placed, also probably not the best name for how this is used but oh well
    CF_Aabb usable_aabb;
    CF_Aabb item_aabb;
    
    u64 item_hash;
    
    CF_Color background_color;
    CF_Color border_color;
    CF_Color title_color;
    f32 border_thickness;
    f32 corner_radius;
    f32 title_font_size;
    f32 item_padding;
    
    //  @todo:  background and 9 slice
    
    // (0, 0) is top left, (1, 1) is bottom right
    CF_V2 scroll;
    // 0 to 1 for both x and y axis, this is scaling outwards from center outwards
    // items won't be visible while scaling out
    CF_V2 expand_animation_t;
    
    UI_Layout_Direction direction;
    UI_Layout_Direction grid_direction;
    UI_Layout_Alignment alignment;
    UI_Layout_Scroll scroll_direction;
    
    UI_Layout_State state;
    
    CF_ListNode node;
} UI_Layout;

typedef union UI_Layout_Var
{
    b32 b32_value;
    s32 s32_value;
    f32 f32_value;
    f64 f64_value;
    u64 u64_value;
    CF_V2 v2_value;
    CF_Aabb aabb;
    const char* string;
} UI_Layout_Var;

typedef struct UI_Style
{
    // applies to UI_Item
    dyna CF_Color* text_colors;
    dyna CF_Color* text_shadow_colors;
    dyna CF_Color* background_colors;
    dyna CF_Color* border_colors;
    // yes these are typos for the macro sssss
    dyna f32* border_thicknesss;
    dyna f32* corner_radiuss;
    dyna b32* word_wraps;
    
    // UI_Item interactables
    dyna CF_Color* idle_text_colors;
    dyna CF_Color* idle_text_shadow_colors;
    dyna CF_Color* idle_background_colors;
    dyna CF_Color* idle_border_colors;
    
    dyna CF_Color* hover_text_colors;
    dyna CF_Color* hover_text_shadow_colors;
    dyna CF_Color* hover_background_colors;
    dyna CF_Color* hover_border_colors;
    
    dyna CF_Color* pressed_text_colors;
    dyna CF_Color* pressed_text_shadow_colors;
    dyna CF_Color* pressed_background_colors;
    dyna CF_Color* pressed_border_colors;
    
    dyna b32* is_disableds;
    dyna CF_Color* disabled_colors;
    
    // applies to UI_Layout
    dyna CF_Color* layout_background_colors;
    dyna CF_Color* layout_border_colors;
    dyna CF_Color* layout_title_colors;
    dyna f32* layout_border_thicknesss;
    dyna f32* layout_corner_radiuss;
    dyna f32* layout_item_paddings;
    
    dyna UI_Item_Alignment* item_alignments;
    
} UI_Style;

typedef struct UI_Input
{
    CF_V2 mouse_position;
    CF_ButtonBinding select;
    CF_ButtonBinding cancel;
    CF_ButtonBinding menu;
    CF_ButtonBinding drag;
    f32 mouse_wheel;
} UI_Input;

typedef struct UI_Text_Effect_Callbacks
{
    CF_TextEffectFn* draw;
    CF_TextEffectFn* hover;
} UI_Text_Effect_Callbacks;

typedef struct UI
{
    UI_Style style;
    UI_Input input;
    
    dyna UI_Layout* layouts;
    Node_Pool layout_pool;
    
    dyna UI_Layout** layout_stack;
    CF_MAP(UI_Item*) item_map;
    
    CF_MAP(CF_MAP(UI_Layout_Var)) layout_var_tables;
    CF_MAP(UI_Text_Effect_Callbacks) text_effect_map;
    
    const char* hover_layout_name;
    CF_V2 hover_layout_position;
    const char* down_layout_name;
    CF_V2 down_layout_position;
    CF_V2 down_layout_offset;
    
    CF_V2 down_root_layout_position;
    CF_V2 down_root_layout_offset;
    
    // offsets here are based off of aabb.min
    CF_V2 hover_position;
    CF_V2 hover_offset;
    CF_V2 hover_normalize_position;
    CF_V2 down_position;
    CF_V2 down_offset;
    CF_V2 down_normalize_position;
    
    u64 debug_hover_hash;
    u64 hover_hash;
    u64 down_hash;
    u64 release_hash;
    
    struct
    {
        CF_Mesh mesh;
        
    } draw;
} UI;

void init_ui();
void draw_ui();

void ui_begin();
void ui_end();

void ui_update_input();
void ui_style_reset();

void ui_push_camera();
void ui_pop_camera();

//  @todo:  should there be a ui_layout_child_begin() that in-herits some properties from the parent layout?
//          such as aabb?
UI_Layout* ui_layout_begin(const char* name);
void ui_layout_end();
UI_Layout* ui_child_layout_begin(CF_V2 size);
void ui_child_layout_end();

UI_Layout* ui_peek_layout();
void ui_layout_set_title(const char* title);
void ui_layout_set_aabb(CF_Aabb aabb);
void ui_layout_expand_usable_aabb(CF_V2 value);
void ui_layout_set_alignment(UI_Layout_Alignment alignment);
void ui_layout_set_direction(UI_Layout_Direction direction);
void ui_layout_set_grid_direction(UI_Layout_Direction direction);
void ui_layout_set_scrollable(UI_Layout_Scroll scroll_direction, CF_V2 start_position);
void ui_layout_do_scrollbar(b32 auto_hide);
void ui_layout_set_expand_animation_t(CF_V2 expand_t);
UI_Item* ui_layout_peek_item();
CF_V2 ui_layout_get_anchor(UI_Layout* layout);
b32 ui_layout_do_close_button();

b32 ui_layout_is_hovering(UI_Layout* layout);
b32 ui_layout_is_down(UI_Layout* layout);

b32 ui_layout_is_hovering_scrollable(UI_Layout* layout);
b32 ui_layout_is_down_scrollable(UI_Layout* layout);

b32 ui_layout_is_tooltip(UI_Layout* layout);
b32 ui_is_any_layout_hovered();
CF_V2 ui_layout_get_scroll_offset(UI_Layout* layout);

CF_V2 ui_layout_item_get_anchor(UI_Layout* layout, UI_Item* item);

UI_Layout* ui_tooltip_begin(CF_V2 size);
void ui_tooltip_end();
UI_Layout* ui_tooltip_vfmt(const char* fmt, va_list args);
UI_Layout* ui_tooltip(const char* fmt, ...);

void ui_build_layout(UI_Layout* layout);
void ui_build_layouts();
void ui_animate_layouts();
void ui_handle_input();
void ui_consume_release();
void ui_reset_input_hashes();

void ui_move_item(UI_Item* item, CF_V2 offset);
void ui_move_items(UI_Item* begin, UI_Item* end, CF_V2 offset);
// items that are considered for on hover is anything that's interactable or can_hover
b32 ui_item_is_hovered();
u64 ui_layout_get_hash(UI_Layout* layout);
UI_Item* ui_make_item_ex(UI_Layout* layout);
UI_Item* ui_make_item();
UI_Item* ui_do_item_vfmt(const char* fmt, va_list args);
UI_Item* ui_get_item(u64 hash);

void ui_do_text(const char* fmt, ...);
b32 ui_handle_button(UI_Item* item);
b32 ui_do_button_vfmt(const char* fmt, va_list args);
b32 ui_do_button(const char* fmt, ...);
b32 ui_do_checkbox(b32* value);
b32 ui_do_checkbox_bit8(b8* mask, s32 index);
b32 ui_do_checkbox_bit16(b16* mask, s32 index);
b32 ui_do_checkbox_bit32(b32* mask, s32 index);
b32 ui_do_checkbox_bit64(b64* mask, s32 index);
b32 ui_handle_scrollbar(UI_Item* item, f32* value, f32 min, f32 max, f32 rate);
b32 ui_do_scrollbar_ex(f32* value, f32 min, f32 max, f32 rate, f32 scale, b32 is_horizontal);
// horizontal slider
b32 ui_do_slider(f32* value, f32 min, f32 max, f32 rate);
void ui_do_sprite_ex(CF_Sprite sprite, CF_V2 size);
void ui_do_sprite(CF_Sprite sprite);
b32 ui_do_sprite_button(CF_Sprite sprite);
b32 ui_do_tabs(const char** names, s32 count, s32* current);
// hsv color wheel, in color is expected to be in RGBA format
b32 ui_do_color_wheel(CF_Color* color);

// @same_line
// only handles horizontal same line, so if you have a layout placing items to left or right
// then this won't work correctly
void ui_do_same_line();

// misc

void ui_register_text_effect(const char* name, CF_TextEffectFn* draw_callback, CF_TextEffectFn* hover_callback);

#endif //UI_H
