#include "game/condition.h"

void init_conditions()
{
    Condition_Pool* pool = &s_app->world.condition_pool;
    node_pool_init(&pool->pool);
    
    cf_array_fit(pool->list, MAX_CONDITION_POOL_SIZE);
    
    for (s32 index = 0; index < cf_array_capacity(pool->list); ++index)
    {
        Condition_Internal condition = { 0 };
        cf_array_fit(condition.effects, Condition_Effect_Type_Count);
        cf_array_push(pool->list, condition);
        node_pool_add(&pool->pool, &cf_array_last(pool->list).node);
    }
}

void conditions_reset()
{
    Condition_Pool* pool = &s_app->world.condition_pool;
    node_pool_reset(&pool->pool);
}

Condition make_condition()
{
    Condition_Pool* pool = &s_app->world.condition_pool;
    Condition condition = (Condition){ 0 };
    CF_ListNode* node = node_pool_alloc(&pool->pool);
    if (node)
    {
        Condition_Internal* condition_internal = CF_LIST_HOST(Condition_Internal, node, node);
        cf_array_clear(condition_internal->effects);
        condition.id = (u64)(uintptr_t)condition_internal;
    }
    
    return condition;
}

dyna Condition_Effect* get_condition_effects(Condition condition)
{
    dyna Condition_Effect* effects = NULL;
    if (condition.id)
    {
        Condition_Internal* condition_internal = (Condition_Internal*)condition.id;
        effects = condition_internal->effects;
    }
    return effects;
}

void destroy_condition(Condition condition)
{
    if (condition.id)
    {
        Condition_Pool* pool = &s_app->world.condition_pool;
        Condition_Internal* condition_internal = (Condition_Internal*)condition.id;
        node_pool_free(&pool->pool, &condition_internal->node);
    }
}

b32 has_condition_effect(Condition condition, Condition_Effect_Type type)
{
    b32 found = false;
    dyna Condition_Effect* effects = get_condition_effects(condition);
    for (s32 index = 0; index < cf_array_count(effects); ++index)
    {
        if (effects[index].type == type)
        {
            found = true;
            break;
        }
    }
    
    return found;
}

b32 can_add_condition_effect(Condition condition, Condition_Effect_Type type)
{
    return !has_condition_effect(condition, type);;
}

b32 add_condition_effect(Condition condition, Condition_Effect_Type type)
{
    b32 added_successfully = false;
    dyna Condition_Effect* effects = get_condition_effects(condition);
    
    if (can_add_condition_effect(condition, type))
    {
        //  @todo:  some function that has a table of all of the effect types with some default values
        Condition_Effect effect = get_condition_effect(type);
        cf_array_push(effects, effect);
        added_successfully = true;
    }
    else
    {
        // reset duration
        for (s32 index = 0; index < cf_array_count(effects); ++index)
        {
            if (effects[index].type == type)
            {
                effects[index].duration = get_condition_duration(type);
                break;
            }
        }
    }
    
    return added_successfully;
}

b32 remove_condition_effect(Condition condition, Condition_Effect_Type type)
{
    b32 remove_successfully = false;
    dyna Condition_Effect* effects = get_condition_effects(condition);
    for (s32 index = cf_array_count(effects) - 1; index >= 0; --index)
    {
        if (effects[index].type == type)
        {
            cf_array_del(effects, index);
            remove_successfully = true;
        }
    }
    
    return remove_successfully;
}

fixed Condition_Effect_Type* get_conflict_condition_effects(Condition condition, Condition_Effect_Type type)
{
    dyna Condition_Effect* effects = get_condition_effects(condition);
    fixed Condition_Effect_Type* conflict_effects = NULL;
    
    if (cf_array_count(effects))
    {
        MAKE_SCRATCH_ARRAY(conflict_effects, cf_array_count(effects));
        fixed Condition_Effect_Type* search_conflict_effects = NULL;
        MAKE_SCRATCH_ARRAY(search_conflict_effects, Condition_Effect_Type_Count);
        
        switch (type)
        {
            case Condition_Effect_Type_Grow_Arms:
            {
                break;
            }
            case Condition_Effect_Type_Enlarge:
            {
                cf_array_push(search_conflict_effects, Condition_Effect_Type_Shrink);
                break;
            }
            case Condition_Effect_Type_Shrink:
            {
                cf_array_push(search_conflict_effects, Condition_Effect_Type_Enlarge);
                break;
            }
        }
        
        for (s32 index = 0; index < cf_array_count(effects); ++index)
        {
            Condition_Effect_Type current_type = effects[index].type;
            for (s32 search_index = 0; search_index < cf_array_count(search_conflict_effects); ++search_index)
            {
                Condition_Effect_Type search_type = search_conflict_effects[search_index];
                
                if (current_type == search_type)
                {
                    cf_array_push(conflict_effects, search_type);
                    break;
                }
            }
        }
    }
    
    return conflict_effects;
}

b32 can_add_condition_after_conflict(Condition_Effect_Type type)
{
    Condition_Effect effect = get_condition_effect(type);
    return !effect.is_blocked_after_conflict;
}

fixed Condition_Combination* get_condition_combinations(Condition_Effect_Type type)
{
    fixed Condition_Combination* combinations = NULL;
    MAKE_SCRATCH_ARRAY(combinations, 8);
    
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            Condition_Combination combination = { 0 };
            combination.other = Condition_Effect_Type_Enlarge;
            combination.combined = Condition_Effect_Type_HP_Up;
            cf_array_push(combinations, combination);
            break;
        }
    }
    
    return combinations;
}

f32 get_condition_duration(Condition_Effect_Type type)
{
    return 30.0f;
}

f32 get_condition_tick_rate(Condition_Effect_Type type)
{
    f32 tick_rate = 0.0f;
    switch (type)
    {
        case Condition_Effect_Type_Burn:
        {
            tick_rate = 2.0f;
            break;
        }
        case Condition_Effect_Type_Regen:
        {
            tick_rate = 2.0f;
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            tick_rate = get_condition_duration(type);
            break;
        }
    }
    return tick_rate;
}

CF_Sprite get_condition_sprite(Condition_Effect_Type type)
{
    CF_Sprite sprite = assets_get_sprite(ASSETS_DEFAULT);;
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            sprite = assets_get_sprite("sprites/hand.ase");
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            sprite = assets_get_sprite("sprites/pawn_up.ase");
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            sprite = assets_get_sprite("sprites/pawn_down.ase");
            break;
        }
        case Condition_Effect_Type_Death:
        {
            sprite = assets_get_sprite("sprites/skull.ase");
            break;
        }
        case Condition_Effect_Type_Burn:
        {
            sprite = assets_get_sprite("sprites/fire.ase");
            break;
        }
        case Condition_Effect_Type_Regen:
        {
            sprite = assets_get_sprite("sprites/flask_half.ase");
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            sprite = assets_get_sprite("sprites/suit_hearts.ase");
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            sprite = assets_get_sprite("sprites/sword.ase");
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            sprite = assets_get_sprite("sprites/hourglass.ase");
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            sprite = assets_get_sprite("sprites/heart.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_NEG;
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            sprite = assets_get_sprite("sprites/sword.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_NEG;
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            sprite = assets_get_sprite("sprites/hourglass.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_NEG;
            break;
        }
        case Condition_Effect_Type_Charmed:
        {
            sprite = assets_get_sprite("sprites/arrow_rotate.ase");
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            sprite = assets_get_sprite("sprites/sword.ase");
            break;
        }
        default:
        break;
    }
    
    return sprite;
}

Condition_Effect_Type get_condition_effect_from_mask(u64 mask)
{
    return (Condition_Effect_Type)BIT_UNSET_EX(mask, CONDITION_MASK);
}

b32 can_add_condition_after_combination(Condition_Effect_Type type)
{
    Condition_Effect effect = get_condition_effect(type);
    return !effect.is_blocked_after_combine;
}

const char* get_condition_name(Condition_Effect_Type type)
{
    const char* name = NULL;
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            name = cf_sintern("Grow Arms");
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            name = cf_sintern("Enlarge");
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            name = cf_sintern("Shrink");
            break;
        }
        case Condition_Effect_Type_Death:
        {
            name = cf_sintern("Death");
            break;
        }
        case Condition_Effect_Type_Burn:
        {
            name = cf_sintern("Burn");
            break;
        }
        case Condition_Effect_Type_Regen:
        {
            name = cf_sintern("Regen");
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            name = cf_sintern("HP Up");
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            name = cf_sintern("Str Up");
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            name = cf_sintern("Agi Up");
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            name = cf_sintern("HP Down");
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            name = cf_sintern("Str Down");
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            name = cf_sintern("Agi Down");
            break;
        }
        case Condition_Effect_Type_Charmed:
        {
            name = cf_sintern("Charmed");
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            name = cf_sintern("Enraged");
            break;
        }
        default:
        break;
    }
    
    return name;
}

const char* get_condition_description(Condition_Effect_Type type)
{
    const char* description = NULL;
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            description = scratch_fmt("Lengthens arms and increases strength");
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            description = scratch_fmt("Increases size, making the creature strong and slow");
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            description = scratch_fmt("Decreases size, making the creature fast and weak");
            break;
        }
        case Condition_Effect_Type_Death:
        {
            description = scratch_fmt("Dead, creature didn't make it");
            break;
        }
        case Condition_Effect_Type_Burn:
        {
            description = scratch_fmt("Burnt, take %d damage every %.2f seconds", get_condition_tick_damage(type, 1),  get_condition_tick_rate(type));
            break;
        }
        case Condition_Effect_Type_Regen:
        {
            description = scratch_fmt("Regenerates health every %.2f seconds", get_condition_tick_rate(type));
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            description = scratch_fmt("Increases max health, fattens up the creature");
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            description = scratch_fmt("Increases strength, pumps up the creature");
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            description = scratch_fmt("Increases agility, speeds up the creature");
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            description = scratch_fmt("Decreases max health, softens the creature");
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            description = scratch_fmt("Decreases strength, weakening the creature");
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            description = scratch_fmt("Decreases agility, slowing the creature down");
            break;
        }
        case Condition_Effect_Type_Charmed:
        {
            description = scratch_fmt("Charmed enemies with with you");
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            description = scratch_fmt("Angry");
            break;
        }
        default:
        break;
    }
    
    return description;
}

Condition_Effect get_condition_effect(Condition_Effect_Type type)
{
    Condition_Effect effect = { 0 };
    effect.type = type;
    effect.name = get_condition_name(type);
    effect.description = get_condition_description(type);
    effect.duration_type = Condition_Duration_Type_Finite;
    effect.duration = get_condition_duration(type);
    effect.tick_rate = get_condition_tick_rate(type);
    effect.is_blocked_after_conflict = true;
    
    return effect;
}

CF_Color get_condition_text_color(Condition_Effect_Type type)
{
    CF_Color color = cf_color_white();
    
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            color = cf_color_cyan();
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            color = cf_color_cyan();
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            color = cf_color_cyan();
            break;
        }
        case Condition_Effect_Type_Death:
        {
            color = cf_color_red();
            break;
        }
        case Condition_Effect_Type_Burn:
        {
            color = cf_color_red();
            break;
        }
        case Condition_Effect_Type_Regen:
        {
            color = cf_color_green();
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            color = cf_color_green();
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            color = cf_color_cyan();
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            color = cf_color_cyan();
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            color = cf_color_orange();
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            color = cf_color_orange();
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            color = cf_color_orange();
            break;
        }
        case Condition_Effect_Type_Charmed:
        {
            color = cf_color_red();
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            color = cf_color_cyan();
            break;
        }
        default:
        break;
    }
    
    return color;
}

s32 get_condition_tick_damage(Condition_Effect_Type type, s32 tick_count)
{
    s32 damage = 0;
    
    switch (type)
    {
        case Condition_Effect_Type_Burn:
        {
            damage = 1 * tick_count;
            break;
        }
        case Condition_Effect_Type_Regen:
        {
            damage = -1 * tick_count;
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            damage = 1 * tick_count;
            break;
        }
        default:
        break;
    }
    
    return damage;
}

s32 get_condition_added_damage(Condition_Effect_Type type)
{
    s32 damage = 0;
    
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            damage = 1;
            break;
        }
        case Condition_Effect_Type_Burn:
        {
            damage = 1;
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            damage = 1;
            break;
        }
        default:
        break;
    }
    
    return damage;
}

Condition_Effect_Type get_condition_added_effect(Condition_Effect_Type type)
{
    Condition_Effect_Type added_effect = Condition_Effect_Type_None;
    
    switch (type)
    {
        case Condition_Effect_Type_Burn:
        {
            added_effect = Condition_Effect_Type_Burn;
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            added_effect = Condition_Effect_Type_Shrink;
            break;
        }
        default:
        break;
    }
    
    return added_effect;
    
}

Attributes get_condition_attributes(Condition_Effect_Type type)
{
    Attributes attributes = { 0 };
    
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            attributes.strength = 10;
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            attributes.strength = 10;
            attributes.agility = -6;
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            attributes.strength = -6;
            attributes.agility = 10;
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            attributes.health = 5;
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            attributes.strength = 6;
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            attributes.agility = 6;
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            attributes.health -= 5;
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            attributes.strength -= 6;
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            attributes.agility -= 6;
            break;
        }
        case Condition_Effect_Type_Enrage:
        {
            attributes.strength += 4;
            attributes.agility += 4;
            break;
        }
    }
    return attributes;
}

struct Body_Proportions get_condition_body_proportions(Condition_Effect_Type type)
{
    Body_Proportions proportions = { 0 };
    switch (type)
    {
        case Condition_Effect_Type_Grow_Arms:
        {
            proportions.upper_arm_length = 0.5f;
            proportions.lower_arm_length = 0.5f;
            proportions.upper_arm_thickness = 1.0f;
            proportions.lower_arm_thickness = 1.0f;
            proportions.hand_thickness = 1.5f;
            break;
        }
        case Condition_Effect_Type_Enlarge:
        {
            proportions.scale = 0.25f;
            break;
        }
        case Condition_Effect_Type_Shrink:
        {
            proportions.scale = -0.25f;
            break;
        }
        case Condition_Effect_Type_HP_Up:
        {
            proportions.torso_chubbiness = 2.0f;
            break;
        }
        case Condition_Effect_Type_Str_Up:
        {
            proportions.upper_arm_thickness = 0.5f;
            break;
        }
        case Condition_Effect_Type_Agi_Up:
        {
            proportions.upper_leg_thickness = 0.5f;
            break;
        }
        case Condition_Effect_Type_HP_Down:
        {
            proportions.torso_chubbiness = -0.5f;
            break;
        }
        case Condition_Effect_Type_Str_Down:
        {
            proportions.upper_arm_thickness = -0.5f;
            break;
        }
        case Condition_Effect_Type_Agi_Down:
        {
            proportions.upper_leg_thickness = -0.5f;
            break;
        }
    }
    return proportions;
}