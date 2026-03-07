#ifndef SPELLS_H
#define SPELLS_H

#ifndef SPELL_MASK
#define SPELL_MASK BIT(62)
#endif

typedef u8 Cast_Target_Type;
//  @todo:  get rid of the prefix here
#define Cast_Target_Type(PREFIX, ENUM) \
ENUM(PREFIX, All) \
ENUM(PREFIX, Enemies) \
ENUM(PREFIX, Allies)

MAKE_ENUM(Cast_Target_Type);

typedef s32 Spell_Type;
enum
{
    Spell_Type_None,
    Spell_Type_Big_Hand,
    Spell_Type_Enlarge,
    Spell_Type_Shrink,
    Spell_Type_Death,
    Spell_Type_Ignite,
    Spell_Type_Regen,
    Spell_Type_Vigor,
    Spell_Type_Charm,
    Spell_Type_Count,
};

typedef struct Spell_Data
{
    CF_V2 size;
    
    fixed Condition_Effect_Type* effects;
    s32 damage;
    
    Spell_Type type;
    Cast_Target_Type target_type;
} Spell_Data;

const char* get_spell_name(Spell_Type spell_type);
const char* get_spell_description(Spell_Type spell_type);
Spell_Data get_spell_data(Spell_Type spell_type);
CF_Color get_spell_text_color(Spell_Type spell_type);

CF_Sprite get_spell_sprite(Spell_Type spell_type);
const char* get_spell_sound_name(Spell_Type spell_type);

#endif //SPELLS_H
