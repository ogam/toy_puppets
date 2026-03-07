#include "game/items.h"

const char* get_item_name(Item_Type type)
{
    const char* name = NULL;
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            name = cf_sintern("Brass Knuckles");
            break;
        }
        case Item_Type_Speed_Boots:
        {
            name = cf_sintern("Speed Boots");
            break;
        }
        case Item_Type_Burning_Hands:
        {
            name = cf_sintern("Burning Hands");
            break;
        }
        case Item_Type_Heart_Container:
        {
            name = cf_sintern("Heart Container");
            break;
        }
    }
    
    return name;
}

const char* get_item_description(Item_Type type)
{
    const char* description = NULL;
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            Condition_Effect_Type effect = get_item_slap_condition_effect(type);
            const char* effect_name = get_condition_name(effect);
            description = scratch_fmt("Slaps hits harder and applies <condition type=%d>%s</condition> to creatures", effect, effect_name);
            break;
        }
        case Item_Type_Speed_Boots:
        {
            description = scratch_fmt("Speeds up creatures");
            break;
        }
        case Item_Type_Burning_Hands:
        {
            Condition_Effect_Type effect = get_item_slap_condition_effect(type);
            const char* effect_name = get_condition_name(effect);
            description = scratch_fmt("Slaps applies <condition type=%d>%s</condition> to creatures", effect, effect_name);
            break;
        }
        case Item_Type_Heart_Container:
        {
            description = scratch_fmt("Adds %d to creatures max HP", get_item_attributes(type).health);
            break;
        }
    }
    
    return description;
}

CF_Sprite get_item_sprite(Item_Type type)
{
    CF_Sprite sprite = assets_get_sprite(ASSETS_DEFAULT);
    
    switch (type)
    {
        case Item_Type_Heart_Container:
        {
            sprite = assets_get_sprite("sprites/suit_hearts.ase");
            break;
        }
        default:
        {
            sprite = assets_get_sprite("sprites/puzzle.ase");
            break;
        }
    }
    
    return sprite;
}

Condition_Effect_Type get_item_slap_condition_effect(Item_Type type)
{
    Condition_Effect_Type effect = Condition_Effect_Type_None;
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            effect = Condition_Effect_Type_Grow_Arms;
            break;
        }
        case Item_Type_Burning_Hands:
        {
            effect = Condition_Effect_Type_Burn;
            break;
        }
    }
    
    return effect;
}

s32 get_item_slap_damage(Item_Type type)
{
    s32 damage = 0;
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            damage = 1;
            break;
        }
    }
    
    return damage;
}

Attributes get_item_attributes(Item_Type type)
{
    Attributes attributes = { 0 };
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            attributes.strength = 10;
            break;
        }
        case Item_Type_Speed_Boots:
        {
            attributes.move_speed = 10.0f;
            break;
        }
        case Item_Type_Heart_Container:
        {
            attributes.health = 10;
            break;
        }
    }
    
    return attributes;
}

CF_Color get_item_text_color(Item_Type type)
{
    CF_Color color = cf_color_white();
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            color = cf_color_red();
            break;
        }
        case Item_Type_Speed_Boots:
        {
            color = cf_color_cyan();
            break;
        }
        case Item_Type_Burning_Hands:
        {
            color = cf_color_red();
            break;
        }
        case Item_Type_Heart_Container:
        {
            color = cf_color_cyan();
            break;
        }
    }
    
    return color;
}

Item_Type get_item_type_from_mask(u64 mask)
{
    return (Item_Type)BIT_UNSET_EX(mask, ITEM_MASK);
}

s32 get_item_added_damage(Item_Type type)
{
    s32 damage = 0;
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            damage = 1;
            break;
        }
    }
    
    return damage;
}

Condition_Effect_Type get_item_added_effect(Item_Type type)
{
    Condition_Effect_Type effect = 0;
    
    switch (type)
    {
        case Item_Type_Burning_Hands:
        {
            effect = Condition_Effect_Type_Burn;
            break;
        }
    }
    
    return effect;
}

b32 get_next_item_type_from_mask(u64 mask, Item_Type* in_out_item_type)
{
    // always shift by at least 1 since the first bit should never be set (Item_Type_None)
    mask = mask >> (*in_out_item_type);
    Item_Type offset = 0;
    
    while (mask && !BIT_IS_SET(mask, 0))
    {
        mask >>= 1;
        offset++;
    }
    
    *in_out_item_type = offset + *in_out_item_type;
    return mask != 0;
}

u64 get_item_team_mask(Item_Type type)
{
    u64 team_mask = 0;
    
    switch (type)
    {
        case Item_Type_Brass_Knuckles:
        {
            BIT_SET(team_mask, TOY_PLAYER_INDEX);
            break;
        }
        case Item_Type_Speed_Boots:
        {
            BIT_SET(team_mask, TOY_PLAYER_INDEX);
            break;
        }
        case Item_Type_Burning_Hands:
        {
            BIT_SET(team_mask, TOY_PLAYER_INDEX);
            break;
        }
        case Item_Type_Heart_Container:
        {
            BIT_SET(team_mask, TOY_PLAYER_INDEX);
            break;
        }
    }
    
    return team_mask;
}