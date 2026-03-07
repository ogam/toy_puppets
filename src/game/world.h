#ifndef WORLD_H
#define WORLD_H

#pragma warning(push)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)

#define PICO_ECS_MAX_COMPONENTS (32)
#define PICO_ECS_MAX_SYSTEMS (32)

#ifndef ECS_ENTITY_COUNT
#define ECS_ENTITY_COUNT 4096
#endif

#define PICO_ECS_IMPLEMENTATION
#include "pico_ecs.h"

#pragma warning(pop)

#ifndef ECS_GET_COMP
#define ECS_GET_COMP(COMP)  cf_map_get(s_app->world.components, cf_sintern(#COMP))
#endif

#ifndef ECS_GET_SYSTEM
#define ECS_GET_SYSTEM(SYSTEM) cf_map_get(s_app->world.systems, cf_sintern(#SYSTEM))
#endif

#ifndef ECS_REGISTER_COMP
#define ECS_REGISTER_COMP(COMP) \
{ \
ecs_comp_t id = ecs_define_component(s_app->world.ecs, sizeof(COMP), NULL, NULL); \
cf_map_set(s_app->world.components, cf_sintern(#COMP), id); \
}
#endif

#ifndef ECS_REGISTER_COMP_CB
#define ECS_REGISTER_COMP_CB(COMP, CTOR, DTOR) \
{ \
ecs_comp_t id = ecs_define_component(s_app->world.ecs, sizeof(COMP), CTOR, DTOR); \
cf_map_set(s_app->world.components, cf_sintern(#COMP), id); \
}
#endif

#ifndef ECS_REGISTER_SYSTEM
#define ECS_REGISTER_SYSTEM(SYSTEM) \
{ \
ecs_system_t id = ecs_define_system(s_app->world.ecs, (ecs_mask_t){0}, SYSTEM, NULL, NULL, NULL); \
cf_map_set(s_app->world.systems, cf_sintern(#SYSTEM), id); \
}
#endif

#ifndef ECS_REGISTER_SYSTEM_CB
#define ECS_REGISTER_SYSTEM_CB(SYSTEM, ADD_FN, REMOVE_FN) \
{ \
ecs_system_t id = ecs_define_system(s_app->world.ecs, (ecs_mask_t){0}, SYSTEM, ADD_FN, REMOVE_FN, NULL); \
cf_map_set(s_app->world.systems, cf_sintern(#SYSTEM), id); \
}
#endif

#ifndef ECS_REQUIRE_COMP
#define ECS_REQUIRE_COMP(SYSTEM, COMP) ecs_require_component(s_app->world.ecs, ECS_GET_SYSTEM(SYSTEM), ECS_GET_COMP(COMP))
#endif

#ifndef ECS_EXCLUDE_COMP
#define ECS_EXCLUDE_COMP(SYSTEM, COMP) ecs_exclude_component(s_app->world.ecs, ECS_GET_SYSTEM(SYSTEM), ECS_GET_COMP(COMP))
#endif

#ifndef ECS_GET
#define ECS_GET(ENTITY, COMP) ecs_get(s_app->world.ecs, ENTITY, ECS_GET_COMP(COMP))
#endif

#ifndef ECS_ADD
#define ECS_ADD(ENTITY, COMP) ecs_add(s_app->world.ecs, ENTITY, ECS_GET_COMP(COMP), NULL)
#endif

#ifndef ECS_HAS
#define ECS_HAS(ENTITY, COMP) ecs_has(s_app->world.ecs, ENTITY, ECS_GET_COMP(COMP))
#endif

#ifndef ECS_RUN_SYSTEM
#define ECS_RUN_SYSTEM(SYSTEM) ecs_run_system(s_app->world.ecs, ECS_GET_SYSTEM(SYSTEM), 0)
#endif

#ifndef ENTITY_IS_VALID
#define ENTITY_IS_VALID(ENTITY) ( !ECS_IS_INVALID(ENTITY) && ecs_is_ready(s_app->world.ecs, ENTITY) )
#endif

typedef struct C_Puppet
{
    Body body;
    CF_MAP(Body_Proportions) proportions;
    CF_V2 stride;
    f32 spin_speed;
    CF_Color color;
    
    CF_Coroutine action_co;
} C_Puppet;

typedef struct C_Condition
{
    Condition handle;
} C_Condition;

typedef struct C_AI
{
    ecs_entity_t target;
    
    f32 attack_delay;
} C_AI;

//  @note:  if ai needs to be coordinated it might be better to have a system 
//          to handle targetting / movement of groups, for now it's 'good enough'
#if 0
typedef struct AI_Path
{
    CF_V2 start;
    CF_V2 end;
} AI_Path;

typedef struct C_AI_Conductor
{
    CF_MAP(AI_Path) paths;
} C_AI_Conductor;
#endif

typedef u8 C_Human;
typedef u8 C_Tentacle;
typedef u8 C_Slime;

typedef struct C_Team
{
    u64 base;
    u64 current;
    u64 next;
} C_Team;

typedef struct C_Hurt_Box
{
    dyna ecs_entity_t* hits;
    s32 damage;
} C_Hurt_Box;

typedef s32 C_Health;

typedef struct Attributes
{
    s32 health;
    s32 strength;
    s32 agility;
    f32 move_speed;
} Attributes;

typedef struct C_Creature
{
    const char* name;
    //  @todo:  currently all tributes are set based off of Condition_Effect_Type
    //          that is a u8, each key of a CF_MAP takes a u64 so there's still 56
    //          available bits to do any additional encoding such as if the attribute
    //          is from an Item, Perk, Feature, etc so this gives a lot more lea-way
    //          on to how to handle looking up these
    CF_MAP(Attributes) attributes;
} C_Creature;

typedef struct C_Hand_Caster
{
    Spell_Type spell_type;
    CF_V2 position;
    CF_V2 size;
    Cast_Target_Type target_type;
} C_Hand_Caster;

typedef u8 Floating_Text_Type;
enum
{
    Floating_Text_Type_Neutral,
    Floating_Text_Type_Positive,
    Floating_Text_Type_Negative,
};

typedef struct Floating_Text
{
    ecs_entity_t entity;
    const char* text;
    CF_Color color;
    CF_V2 position;
    CF_V2 velocity;
    f32 duration;
    f32 delay;
    f32 time;
    s32 damage;
    b8 is_damage;
} Floating_Text;

typedef struct C_Movement
{
    f32 speed;
    CF_V2 direction;
} C_Movement;

typedef struct Damage_Source
{
    s32 damage;
    s32 count;
    Condition_Effect_Type effect;
    Spell_Type spell;
    Item_Type item;
} Damage_Source;

typedef u8 C_Creature_Normal;
typedef u8 C_Creature_Elite;
typedef u8 C_Creature_Boss;
typedef u8 C_Dead;

typedef s32 Event_Type;
enum
{
    // self condition changes
    Event_Type_Condition_Gained,
    Event_Type_Condition_Loss,
    Event_Type_Condition_Expired,
    Event_Type_Condition_Tick,
    
    // attacker to victim
    Event_Type_Condition_Inflict,
    Event_Type_Condition_Purge,
    Event_Type_Damage,
    Event_Type_Hit,
    Event_Type_Died,
    
    Event_Type_Hand_Caster_Select_Spell,
    Event_Type_Hand_Caster_Cast_Spell,
    Event_Type_Hand_Caster_Cast_Spell_Missed,
    Event_Type_Hand_Caster_Condition_Add,
    Event_Type_Hand_Caster_Condition_Remove,
    Event_Type_Hand_Caster_Damage,
    
    Event_Type_Hand_Slap_Hit,
    
    Event_Type_World_Arena_Start,
};

typedef struct C_Event
{
    Event_Type type;
    union
    {
        struct
        {
            ecs_entity_t attacker;
            ecs_entity_t victim;
            Condition_Effect_Type type;
            s32 tick_count;
        } condition;
        
        struct
        {
            ecs_entity_t attacker;
            ecs_entity_t victim;
            fixed Damage_Source* sources;
        } hit;
        
        struct
        {
            Spell_Type spell_type;
            ecs_entity_t entity;
            s32 damage;
        } hand_caster_spell;
        
        struct
        {
            ecs_entity_t victim;
            CF_Aabb slap_region;
        } slap_hit;
        
        struct
        {
            ecs_entity_t victim;
            fixed Damage_Source* sources;
        } slap_damage;
        
        struct
        {
            // this is not stable!
            // only check the victim if there's another system that keeps track of ecs_entity_t
            ecs_entity_t victim;
            const char* name;
            u64 team;
        } died;
    };
} C_Event;

typedef u8 World_State;
enum
{
    World_State_Overworld,
    World_State_Arena_Placement,
    World_State_Arena_In_Progress,
    World_State_Arena_End,
    World_State_Shop,
};

typedef struct World
{
    ecs_t* ecs;
    CF_MAP(ecs_comp_t) components;
    CF_MAP(ecs_system_t) systems;
    
    Condition_Pool condition_pool;
    
    Inventory inventory;
    
    CF_Aabb walk_bounds;
    CF_Aabb bounds;
    
    CF_MAP(dyna ecs_entity_t*) team_creatures;
    Spatial_Map spatial_map;
    dyna Floating_Text* floating_texts;
    
    Body_Type placement_type;
    CF_V2 placement_center;
    CF_V2 placement_size;
    CF_V2 placement_position;
    
    s32 damage_arena_index;
    CF_Arena damage_arenas[2];
    
    ecs_entity_t hover_entity;
    f32 hover_duration;
    
    CF_Rnd rnd;
    CF_Rnd name_rnd;
    
    f32 dt;
    f32 condition_dt;
    
    CF_Coroutine state_transition_co;
    World_State state;
} World;

void init_world();
void update_world();
void draw_world();

void init_ecs();

void world_push_background_camera();
void world_push_camera();
void world_pop_camera();

void world_set_state(World_State state);
void world_clear();
void world_inventory_clear();

void world_arena_update();

void world_arena_draw();

void world_arena_transition_to_in_progress_co(CF_Coroutine co);

void world_arena_enter_placement(CF_Rnd* rnd, s32 enemy_count, s32 elite_count, s32 boss_count);
void world_arena_do_placement();

// components
void comp_puppet_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr);

void comp_condition_ctor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr, void* args);
void comp_condition_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr);

void comp_team_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr);

void comp_creature_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr);

void comp_hurt_box_ctor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr, void* args);
void comp_hurt_box_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr);

void comp_damage_ctor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr, void* args);
void comp_damage_dtor(ecs_t* ecs, ecs_entity_t entity, void* comp_ptr);

// system updates
ecs_ret_t system_handle_events(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_conditions(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_healths(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_creature_teams(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_puppet_animations(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_prebuild_spatial_grid(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
ecs_ret_t system_update_build_spatial_grid(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_hurt_boxes(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_ai_humans(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
ecs_ret_t system_update_ai_tentacles(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
ecs_ret_t system_update_ai_slimes(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_movements(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_hand_caster(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
// optional
ecs_ret_t system_event_cleanup_hand_caster(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_puppet_picking(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_update_floating_texts(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
ecs_ret_t system_update_cursor(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

// system draws
ecs_ret_t system_draw_puppets(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
ecs_ret_t system_draw_healthbars(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);
ecs_ret_t system_draw_hand_caster(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

ecs_ret_t system_draw_floating_texts(ecs_t* ecs, ecs_entity_t* entities, size_t entity_count, void* udata);

C_Event* make_event(Event_Type type);
// self condition changes
void make_event_condition_gained(ecs_entity_t entity, Condition_Effect_Type effect_type);
void make_event_condition_loss(ecs_entity_t entity, Condition_Effect_Type effect_type);
void make_event_condition_expired(ecs_entity_t entity, Condition_Effect_Type effect_type);
void make_event_condition_tick(ecs_entity_t entity, Condition_Effect_Type effect_type, s32 tick_count);

// attacker to victim condition changes
void make_event_condition_inflict(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type type);
void make_event_ondition_purge(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type type);
void make_event_damage(ecs_entity_t attacker, ecs_entity_t victim, fixed Damage_Source* sources);
void make_event_hit(ecs_entity_t attacker, ecs_entity_t victim);
void make_event_died(ecs_entity_t victim, const char* name, u64 team);

// hand cast events
void make_event_hand_caster_select_spell(Spell_Type spell_type);
void make_event_hand_caster_cast_spell(Spell_Type spell_type);
void make_event_hand_caster_cast_spell_missed(Spell_Type spell_type);
void make_event_hand_caster_condition_add(ecs_entity_t entity, Condition_Effect_Type effect_type);
void make_event_hand_caster_condition_remove(ecs_entity_t entity, Condition_Effect_Type effect_type);
void make_event_hand_caster_damage(ecs_entity_t entity, Spell_Type spell_type, s32 damage);

void make_event_hand_slap_hit(ecs_entity_t victim, CF_Aabb slap_region);
void make_event_hand_slap_damage(ecs_entity_t victim, fixed Damage_Source* sources);

// world events
void make_event_world_arena_start();

ecs_entity_t make_creature();
ecs_entity_t make_human(s32 stat_scaling, f32 body_scaling);
ecs_entity_t make_slime(s32 stat_scaling, f32 body_scaling);
ecs_entity_t make_tentacle(s32 stat_scaling, f32 body_scaling);

void hand_cast_add_condition(ecs_entity_t entity, Condition_Effect_Type type);
void attacker_add_condition(ecs_entity_t attacker, ecs_entity_t victim, Condition_Effect_Type type);

void entity_apply_items(ecs_entity_t entity);

Floating_Text* floating_text_add(ecs_entity_t entity, Floating_Text_Type type, const char* text);
Floating_Text* floating_text_add_fmt(ecs_entity_t entity, Floating_Text_Type type, const char* fmt, ...);
void floating_text_add_damage(ecs_entity_t entity, s32 damage);

// utility
ecs_entity_t find_nearest_creature(CF_V2 position, u64 team_mask);

void teleport(ecs_entity_t entity, CF_V2 position);

CF_V2 aabb_random_point(CF_Aabb aabb, CF_Rnd* rnd);


CF_V2 walk_direction_to_attack(Body* body, Body* target_body);

void set_team_base(ecs_entity_t entity, u64 team);
void set_team(ecs_entity_t entity, u64 team);
void team_add_creature(ecs_entity_t entity, u64 team);
void team_remove_creature(ecs_entity_t entity, u64 team);

b32 is_entity_friendly(ecs_entity_t self, ecs_entity_t other);

b32 array_add_unique(dyna ecs_entity_t* list, ecs_entity_t item);

s32 strength_to_damage(C_Creature* creature);
f32 agility_to_attack_rate(C_Creature* creature);

void draw_ellipse(CF_V2 center, CF_V2 size);
b32 ellipse_contains(CF_V2 center, CF_V2 size, CF_V2 point);

Body_Proportions accumulate_body_proportions(CF_MAP(Body_Proportions) proportions);

Damage_Source* make_damage_source(s32 damage, Condition_Effect_Type effect, Spell_Type spell);
s32 accumulate_damage_sources(dyna Damage_Source* sources);
Attributes accumulate_attributes(CF_MAP(Attributes) map);
Attributes attributes_scale(Attributes attributes, s32 scale);

CF_Color get_team_color(u64 team);
const char* get_color_tagged_name_ex(const char* name, u64 team);
const char* get_color_tagged_name(ecs_entity_t entity);

#endif //WORLD_H
