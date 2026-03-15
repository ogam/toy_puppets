#include "game/inventory.h"

void inventory_reset(Inventory* inventory)
{
    if (inventory->spells == NULL)
    {
        cf_array_fit(inventory->spells, Spell_Type_Count);
        cf_array_setlen(inventory->spells, Spell_Type_Count);
    }
    if (inventory->bodies == NULL)
    {
        cf_array_fit(inventory->bodies, Body_Type_Count);
        cf_array_setlen(inventory->bodies, Body_Type_Count);
    }
    if (inventory->items == NULL)
    {
        cf_array_fit(inventory->items, Item_Type_Count);
        cf_array_setlen(inventory->items, Item_Type_Count);
    }
    
    CF_MEMSET(inventory->spells, 0, sizeof(*inventory->spells) * cf_array_count(inventory->spells));
    CF_MEMSET(inventory->bodies, 0, sizeof(*inventory->bodies) * cf_array_count(inventory->bodies));
    CF_MEMSET(inventory->items, 0, sizeof(*inventory->items) * cf_array_count(inventory->items));
    
    inventory->gold = 0;
}

b32 inventory_add_ex(dyna s32* objects, s32 type, s32 count)
{
    b32 added = false;
    if (objects[type] == TOY_INVENTORY_INFINITE_QTY)
    {
        added = true;
    }
    else if (objects[type] < TOY_INVENTORY_MAX_QTY)
    {
        objects[type] = cf_clamp(objects[type] + count, 0, TOY_INVENTORY_MAX_QTY);
        added = true;
    }
    return added;
}

b32 inventory_remove_ex(dyna s32* objects, s32 type, s32 count)
{
    b32 removed = false;
    if (objects[type] >= count)
    {
        if (objects[type] == TOY_INVENTORY_INFINITE_QTY)
        {
            removed = true;
        }
        else
        {
            objects[type] = cf_clamp(objects[type] - count, 0, TOY_INVENTORY_MAX_QTY);
            removed = true;
        }
    }
    
    return removed;
}

b32 inventory_add_body(Inventory* inventory, Body_Type type, s32 count)
{
    b32 added = false;
    if (type < Body_Type_Count)
    {
        added = inventory_add_ex(inventory->bodies, type, count);
    }
    
    return added;
}

b32 inventory_remove_body(Inventory* inventory, Body_Type type, s32 count)
{
    b32 removed = false;
    if (type < Body_Type_Count)
    {
        removed = inventory_remove_ex(inventory->bodies, type, count);
    }
    
    return removed;
}

b32 inventory_has_body(Inventory* inventory, Body_Type type)
{
    return inventory->bodies[type] > 0;
}

b32 inventory_add_spell(Inventory* inventory, Spell_Type type, s32 count)
{
    b32 added = false;
    if (type > Spell_Type_None && type < Spell_Type_Count)
    {
        if (inventory->spells[type] < TOY_INVENTORY_INFINITE_QTY)
        {
            added = inventory_add_ex(inventory->spells, type, count);
        }
    }
    
    return added;
}

b32 inventory_remove_spell(Inventory* inventory, Spell_Type type, s32 count)
{
    b32 removed = false;
    if (type > Spell_Type_None && type < Spell_Type_Count)
    {
        if (inventory->spells[type] >= count)
        {
            removed = inventory_remove_ex(inventory->spells, type, count);
        }
    }
    
    return removed;
}

b32 inventory_has_spell(Inventory* inventory, Spell_Type type)
{
    return inventory->spells[type] > 0;
}

b32 inventory_add_item(Inventory* inventory, Item_Type type, s32 count)
{
    b32 added = false;
    if (type > Item_Type_None && type < Item_Type_Count)
    {
        added = inventory_add_ex(inventory->items, type, count);
    }
    
    return added;
}

b32 inventory_remove_item(Inventory* inventory, Item_Type type, s32 count)
{
    b32 removed = false;
    if (type < Body_Type_Count && 
        inventory->bodies[type] != TOY_INVENTORY_INFINITE_QTY)
    {
        if (inventory->bodies[type] >= count)
        {
            removed = inventory_remove_ex(inventory->items, type, count);
        }
    }
    
    return removed;
}

b32 inventory_has_item(Inventory* inventory, Item_Type type)
{
    return inventory->items[type] > 0;
}


b32 inventory_add_gold(Inventory* inventory, s32 count)
{
    inventory->gold += count;
    return true;
}

b32 inventory_remove_gold(Inventory* inventory, s32 count)
{
    b32 removed = false;
    if (inventory_has_gold(inventory, count))
    {
        inventory->gold -= count;
        removed = true;
    }
    
    return removed;
}

b32 inventory_has_gold(Inventory* inventory, s32 count)
{
    //  @todo:  should there be other things that count as gold?
    return inventory->gold >= count;
}

void inventory_randomize(Inventory* inventory, u64 seed)
{
    CF_Rnd rnd = cf_rnd_seed(seed);
    
    CF_MEMSET(inventory->spells, 0, sizeof(*inventory->spells) * cf_array_count(inventory->spells));
    CF_MEMSET(inventory->bodies, 0, sizeof(*inventory->bodies) * cf_array_count(inventory->bodies));
    CF_MEMSET(inventory->items, 0, sizeof(*inventory->items) * cf_array_count(inventory->items));
    
    inventory->gold = 0;
    
    Body_Type body_types[] = 
    {
        Body_Type_Human,
        Body_Type_Slime,
        Body_Type_Tubeman,
    };
    
    Body_Type spell_types[] = 
    {
        Spell_Type_Enlarge,
        Spell_Type_Shrink,
        Spell_Type_Ignite,
        Spell_Type_Vigor,
    };
    
    s32 body_rolls = 2;
    s32 spell_rolls = 2;
    
    for (s32 index = 0; index < body_rolls; ++index)
    {
        s32 body_type_index = cf_rnd_range_int(&rnd, 0, CF_ARRAY_SIZE(body_types) - 1);
        inventory_add_body(inventory, body_types[body_type_index], 5);
    }
    
    for (s32 index = 0; index < spell_rolls; ++index)
    {
        s32 spell_type_index = cf_rnd_range_int(&rnd, 0, CF_ARRAY_SIZE(spell_types) - 1);
        inventory_add_spell(inventory, spell_types[spell_type_index], 5);
    }
}