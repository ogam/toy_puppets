#ifndef OVERWORLD_H
#define OVERWORLD_H

typedef u8 Overworld_Room_Type;
//  @todo:  get rid of the prefix here
#define Overworld_Room_Type(PREFIX, ENUM) \
ENUM(PREFIX, Normal) \
ENUM(PREFIX, Elite) \
ENUM(PREFIX, Shop) \
ENUM(PREFIX, Boss)

MAKE_ENUM(Overworld_Room_Type);

typedef struct Overworld_Room
{
    // position here is a ratio
    CF_V2 position;
    fixed s32* next_rooms;
    u64 seed;
    s32 depth;
    s32 lane;
    b32 is_connected;
    s32 gold;
    Overworld_Room_Type type;
} Overworld_Room;

typedef struct Overworld_Shop
{
    // same as inventory, each entry is a count for Body_Type or Spell_Type
    dyna s32* spells;
    dyna s32* bodies;
    dyna s32* items;
} Overworld_Shop;

typedef struct Overworld
{
    fixed Overworld_Room* rooms;
    fixed s32* path;
    
    s32 level;
    Overworld_Shop shop;
    
    CF_Arena arena;
    CF_Rnd rnd;
} Overworld;

void init_overworld();
void update_overworld();
void draw_overworld();

void overworld_generate();
void overworld_clear();

void overworld_generate_shop(s32 depth, u64 seed);
b32 overworld_buy_body(Overworld_Shop* shop, Body_Type type);
b32 overworld_buy_spell(Overworld_Shop* shop, Spell_Type type);
b32 overworld_buy_item(Overworld_Shop* shop, Item_Type type);

s32 overworld_shop_get_body_cost(Body_Type type);
s32 overworld_shop_get_spell_cost(Spell_Type type);
s32 overworld_shop_get_item_cost(Item_Type type);

CF_Sprite overworld_get_room_sprite(Overworld_Room_Type type);

void overworld_ui();

Overworld_Room* overworld_get_current_room();
void overworld_current_room_add_gold(s32 gold);
void overworld_current_room_enemy_died(b32 is_normal, b32 is_elite, b32 is_boss);

b32 overworld_can_advance();
b32 overworld_is_level_finished();

#endif //OVERWORLD_H
