#ifndef ITEMS_H
#define ITEMS_H

#ifndef ITEM_MASK
#define ITEM_MASK BIT(63)
#endif

//  @todo:  probably having items as a bit mask is a bit too complicated and it would probably been better
//          to keep this same as spells and conditions for the inventory, that way there can be quantities
//          for stacking item effects
//          the bit mask might be better for things like perks
//  @todo:  whenever perks gets implemented move the bit mask stuff to that instead and change this to match
//          spells and conditions

typedef s32 Item_Type;
enum
{
    Item_Type_None,
    Item_Type_Brass_Knuckles,
    Item_Type_Speed_Boots,
    Item_Type_Burning_Hands,
    Item_Type_Heart_Container,
    Item_Type_Count,
};

const char* get_item_name(Item_Type type);
const char* get_item_description(Item_Type type);

CF_Sprite get_item_sprite(Item_Type type);
Condition_Effect_Type get_item_slap_condition_effect(Item_Type type);
s32 get_item_slap_damage(Item_Type type);
struct Attributes get_item_attributes(Item_Type type);
CF_Color get_item_text_color(Item_Type type);
Item_Type get_item_type_from_mask(u64 mask);
s32 get_item_added_damage(Item_Type type);
Condition_Effect_Type get_item_added_effect(Item_Type type);
b32 get_next_item_type_from_mask(u64 mask, Item_Type* in_out_item_type);
u64 get_item_team_mask(Item_Type type);

#endif //ITEMS_H
