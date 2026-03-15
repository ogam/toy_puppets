#ifndef CONDITION_H
#define CONDITION_H

#ifndef MAX_CONDITION_POOL_SIZE
#define MAX_CONDITION_POOL_SIZE (256)
#endif

#ifndef CONDITION_MASK
#define CONDITION_MASK BIT(61)
#endif

typedef s32 Condition_Effect_Type;
enum
{
    Condition_Effect_Type_None,
    Condition_Effect_Type_Grow_Arms,
    Condition_Effect_Type_Enlarge,
    Condition_Effect_Type_Shrink,
    Condition_Effect_Type_Death,
    Condition_Effect_Type_Burn,
    Condition_Effect_Type_Regen,
    Condition_Effect_Type_HP_Up,
    Condition_Effect_Type_Str_Up,
    Condition_Effect_Type_Agi_Up,
    Condition_Effect_Type_HP_Down,
    Condition_Effect_Type_Str_Down,
    Condition_Effect_Type_Agi_Down,
    Condition_Effect_Type_Charmed,
    Condition_Effect_Type_Enrage,
    Condition_Effect_Type_Count
};

typedef u8 Condition_Duration_Type;
enum
{
    Condition_Duration_Type_Finite,
    Condition_Duration_Type_Infinite,
};

//  @note:  should conditions be stackable?
typedef struct Condition_Effect
{
    const char* name;
    const char* description;
    // tick rate of 0 will never tick, it should be used for things that doesn't anything special
    // such as toggle on/off attributes like buffs/debuffs
    f32 tick_rate;
    f32 duration;
    
    Condition_Effect_Type type;
    Condition_Duration_Type duration_type;
    // leaving this as `false` will allow this effect to stomp on all conflicts,
    // `true` will cancel out all conflicts and not add this effect
    b8 is_blocked_after_conflict;
    // leaving this as `false` will allow this effect to stomp on all other combinations
    // effects, `true` will avoid adding the effect. both ways will still do a combination add
    b8 is_blocked_after_combine;
} Condition_Effect;

// combinations can work in 2 ways
// A + B => C
// A + B => AC
// it's setup as other and combined so that this can be a 1 way combination so that
// you can apply Enlarge + Grow_Arms => Grow_Arms + Vigor
// and do the opposite to Grow_Arms + Enlarge => Grow_Arms + Enlarge
// this can make it annoying to map in both effects if you want to cancel both out but
// it allows more customization. if it seems like it doesn't make sense when playing
// then this can be changed to always cancel both out
typedef struct Condition_Combination
{
    Condition_Effect_Type other;
    Condition_Effect_Type combined;
} Condition_Combination;

typedef struct Condition
{
    u64 id;
} Condition;

typedef struct Condition_Internal
{
    b32 is_active;
    
    dyna Condition_Effect* effects;
    CF_ListNode node;
} Condition_Internal;

typedef struct Condition_Pool
{
    dyna Condition_Internal* list;
    Node_Pool pool;
} Condition_Pool;

void init_conditions();
void conditions_reset();

Condition make_condition();
dyna Condition_Effect* get_condition_effects(Condition condition);

s32 condition_update(Condition condition, f32 dt);

void destroy_condition(Condition condition);
b32 has_condition_effect(Condition condition, Condition_Effect_Type type);
b32 can_add_condition_effect(Condition condition, Condition_Effect_Type type);
b32 add_condition_effect(Condition condition, Condition_Effect_Type type);
b32 remove_condition_effect(Condition condition, Condition_Effect_Type type);
fixed Condition_Effect_Type* get_conflict_condition_effects(Condition condition, Condition_Effect_Type type);
b32 can_add_condition_after_conflict(Condition_Effect_Type type);
fixed Condition_Combination* get_condition_combinations(Condition_Effect_Type type);
f32 get_condition_duration(Condition_Effect_Type type);
f32 get_condition_tick_rate(Condition_Effect_Type type);
CF_Sprite get_condition_sprite(Condition_Effect_Type type);
Condition_Effect_Type get_condition_effect_from_mask(u64 mask);

const char* get_condition_name(Condition_Effect_Type type);
const char* get_condition_description(Condition_Effect_Type type);
Condition_Effect get_condition_effect(Condition_Effect_Type type);
CF_Color get_condition_text_color(Condition_Effect_Type type);
s32 get_condition_tick_damage(Condition_Effect_Type type, s32 tick_Count);
s32 get_condition_added_damage(Condition_Effect_Type type);
Condition_Effect_Type get_condition_added_effect(Condition_Effect_Type type);
struct Attributes get_condition_attributes(Condition_Effect_Type type);
struct Body_Proportions get_condition_body_proportions(Condition_Effect_Type type);

#endif //CONDITION_H
