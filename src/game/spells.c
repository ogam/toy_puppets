#include "game/spells.h"

const char* get_spell_name(Spell_Type spell_type)
{
    const char* name = NULL;
    
    switch (spell_type)
    {
        case Spell_Type_Big_Hand:
        {
            name = cf_sintern("Big Hand");
            break;
        }
        case Spell_Type_Enlarge:
        {
            name = cf_sintern("Enlarge");
            break;
        }
        case Spell_Type_Shrink:
        {
            name = cf_sintern("Shrink");
            break;
        }
        case Spell_Type_Death:
        {
            name = cf_sintern("Death");
            break;
        }
        case Spell_Type_Ignite:
        {
            name = cf_sintern("Ignite");
            break;
        }
        case Spell_Type_Regen:
        {
            name = cf_sintern("Regen");
            break;
        }
        case Spell_Type_Vigor:
        {
            name = cf_sintern("Vigor");
            break;
        }
        case Spell_Type_Charm:
        {
            name= cf_sintern("Charm");
            break;
        }
    }
    
    return name;
}

const char* get_spell_description(Spell_Type spell_type)
{
    const char* description = NULL;
    
    switch (spell_type)
    {
        case Spell_Type_Big_Hand:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Grow_Arms;
            description = scratch_fmt("Applies <condition type=%d>%s</condition> and pumps up creatures", effect_type, get_condition_name(effect_type));
            break;
        }
        case Spell_Type_Enlarge:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Enlarge;
            description = scratch_fmt("Applies <condition type=%d>%s</condition>  making creatures bulky", effect_type, get_condition_name(effect_type));
            break;
        }
        case Spell_Type_Shrink:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Shrink;
            description = scratch_fmt("Applies <condition type=%d>%s</condition>, making creatures nible", effect_type, get_condition_name(effect_type));
            break;
        }
        case Spell_Type_Death:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Death;
            description = scratch_fmt("Causes <condition type=%d>%s</condition>", effect_type, get_condition_name(effect_type));
            break;
        }
        case Spell_Type_Ignite:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Burn;
            description = scratch_fmt("Causes <condition type=%d>%s</condition> to creatures", effect_type, get_condition_name(effect_type));
            break;
        }
        case Spell_Type_Regen:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Regen;
            description = scratch_fmt("Applies <condition type=%d>%s</condition> to creatures", effect_type, get_condition_name(effect_type));
            break;
        }
        case Spell_Type_Vigor:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_HP_Up;
            Attributes attributes = get_condition_attributes(effect_type);
            description = scratch_fmt("Applies <condition type=%d>%s</condition> to creatures and rasies HP by %d", effect_type, get_condition_name(effect_type), attributes.health);
            break;
        }
        case Spell_Type_Charm:
        {
            Condition_Effect_Type effect_type = Condition_Effect_Type_Charmed;
            description = scratch_fmt("Creatures are <condition type=%d>%s</condition> and fights with you", effect_type, get_condition_name(effect_type));
            break;
        }
    }
    
    return description;
}

Spell_Data get_spell_data(Spell_Type spell_type)
{
    Spell_Data spell = { 0 };
    spell.type = spell_type;
    MAKE_SCRATCH_ARRAY(spell.effects, 8);
    
    switch (spell_type)
    {
        case Spell_Type_Big_Hand:
        {
            spell.size = cf_v2(100.0f, 60.0f);
            spell.target_type = Cast_Target_Type_Allies;
            cf_array_push(spell.effects, Condition_Effect_Type_Grow_Arms);
            break;
        }
        case Spell_Type_Enlarge:
        {
            spell.size = cf_v2(250.0f, 100.0f);
            spell.target_type = Cast_Target_Type_All;
            cf_array_push(spell.effects, Condition_Effect_Type_Enlarge);
            break;
        }
        case Spell_Type_Shrink:
        {
            spell.size = cf_v2(250.0f, 100.0f);
            spell.target_type = Cast_Target_Type_All;
            cf_array_push(spell.effects, Condition_Effect_Type_Shrink);
            break;
        }
        case Spell_Type_Death:
        {
            spell.size = cf_v2(50.0f, 30.0f);
            spell.target_type = Cast_Target_Type_All;
            cf_array_push(spell.effects, Condition_Effect_Type_Death);
            break;
        }
        case Spell_Type_Ignite:
        {
            spell.size = cf_v2(150.0f, 60.0f);
            spell.target_type = Cast_Target_Type_All;
            cf_array_push(spell.effects, Condition_Effect_Type_Burn);
            break;
        }
        case Spell_Type_Regen:
        {
            spell.size = cf_v2(150.0f, 60.0f);
            spell.target_type = Cast_Target_Type_All;
            cf_array_push(spell.effects, Condition_Effect_Type_Regen);
            break;
        }
        case Spell_Type_Vigor:
        {
            spell.size = cf_v2(100.0f, 40.0f);
            spell.target_type = Cast_Target_Type_All;
            cf_array_push(spell.effects, Condition_Effect_Type_HP_Up);
            break;
        }
        case Spell_Type_Charm:
        {
            spell.size = cf_v2(100.0f, 40.0f);
            spell.target_type = Cast_Target_Type_Enemies;
            cf_array_push(spell.effects, Condition_Effect_Type_Charmed);
            break;
        }
    }
    
    return spell;
}

CF_Color get_spell_text_color(Spell_Type spell_type)
{
    CF_Color color = cf_color_white();
    
    switch (spell_type)
    {
        case Spell_Type_Big_Hand:
        {
            color = cf_color_green();
            break;
        }
        case Spell_Type_Enlarge:
        {
            color = cf_color_green();
            break;
        }
        case Spell_Type_Shrink:
        {
            color = cf_color_green();
            break;
        }
        case Spell_Type_Death:
        {
            color = cf_color_red();
            break;
        }
        case Spell_Type_Ignite:
        {
            color = cf_color_red();
            break;
        }
        case Spell_Type_Regen:
        {
            color = cf_color_green();
            break;
        }
        case Spell_Type_Vigor:
        {
            color = cf_color_green();
            break;
        }
        case Spell_Type_Charm:
        {
            color = cf_color_red();
            break;
        }
    }
    
    return color;
}

CF_Sprite get_spell_sprite(Spell_Type spell_type)
{
    CF_Sprite sprite = assets_get_sprite(ASSETS_DEFAULT);
    
    switch (spell_type)
    {
        case Spell_Type_Big_Hand:
        {
            sprite = assets_get_sprite("sprites/hand.ase");
            break;
        }
        case Spell_Type_Enlarge:
        {
            sprite = assets_get_sprite("sprites/pawn_up.ase");
            break;
        }
        case Spell_Type_Shrink:
        {
            sprite = assets_get_sprite("sprites/pawn_down.ase");
            break;
        }
        case Spell_Type_Death:
        {
            sprite = assets_get_sprite("sprites/skull.ase");
            break;
        }
        case Spell_Type_Ignite:
        {
            sprite = assets_get_sprite("sprites/fire.ase");
            break;
        }
        case Spell_Type_Regen:
        {
            sprite = assets_get_sprite("sprites/flask_half.ase");
            break;
        }
        case Spell_Type_Vigor:
        {
            sprite = assets_get_sprite("sprites/suit_hearts.ase");
            break;
        }
        case Spell_Type_Charm:
        {
            sprite = assets_get_sprite("sprites/arrow_rotate.ase");
            break;
        }
    }
    
    return sprite;
}

const char* get_spell_sound_name(Spell_Type spell_type)
{
    const char* name = ASSETS_DEFAULT;
    
    switch (spell_type)
    {
        case Spell_Type_Big_Hand:
        {
            name = "sounds/spell_big_hand.wav";
            break;
        }
        case Spell_Type_Enlarge:
        {
            name = "sounds/spell_enlarge.wav";
            break;
        }
        case Spell_Type_Shrink:
        {
            name = "sounds/spell_shrink.wav";
            break;
        }
        case Spell_Type_Death:
        {
            name = "sounds/spell_death.wav";
            break;
        }
        case Spell_Type_Ignite:
        {
            name = "sounds/spell_ignite.wav";
            break;
        }
        case Spell_Type_Regen:
        {
            name = "sounds/spell_regen.wav";
            break;
        }
        case Spell_Type_Vigor:
        {
            name = "sounds/spell_vigor.wav";
            break;
        }
        case Spell_Type_Charm:
        {
            name = "sounds/spell_vigor.wav";
            break;
        }
    }
    
    return name;
}