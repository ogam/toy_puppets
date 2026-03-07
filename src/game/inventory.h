#ifndef INVENTORY_H
#define INVENTORY_H

typedef struct Inventory
{
    // each index maps directly to Spell_Type or Body_Type
    dyna s32* spells;
    dyna s32* bodies;
    dyna s32* items;
    s32 gold;
} Inventory;

void inventory_reset(Inventory* inventory);
b32 inventory_add_body(Inventory* inventory, Body_Type type, s32 count);
b32 inventory_remove_body(Inventory* inventory, Body_Type type, s32 count);
b32 inventory_has_body(Inventory* inventory, Body_Type type);

b32 inventory_add_spell(Inventory* inventory, Spell_Type type, s32 count);
b32 inventory_remove_spell(Inventory* inventory, Spell_Type type, s32 count);
b32 inventory_has_spell(Inventory* inventory, Spell_Type type);

b32 inventory_add_item(Inventory* inventory, Item_Type type, s32 count);
b32 inventory_remove_item(Inventory* inventory, Item_Type type, s32 count);
b32 inventory_has_item(Inventory* inventory, Item_Type type);

b32 inventory_add_gold(Inventory* inventory, s32 count);
b32 inventory_remove_gold(Inventory* inventory, s32 count);
b32 inventory_has_gold(Inventory* inventory, s32 count);

void inventory_randomize(Inventory* inventory, u64 seed);

#endif //INVENTORY_H
