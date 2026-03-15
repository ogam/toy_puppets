#include "game/names.h"

const char* generate_human_name()
{
    const char* names[] =
    {
        "Grug",
        "Doof",
        "Wonder",
        "Stashe",
        "Flimsy",
        "Bouncy",
        "Clumbsy",
    };
    
    const char* name = names[cf_rnd_range_int(&s_app->world.name_rnd, 0, CF_ARRAY_SIZE(names) - 1)];
    
    return cf_sintern(name);
}

const char* generate_slime_name()
{
    const char* names[] =
    {
        "Squish",
        "Squash",
        "Slop",
        "Flop",
        "Flip",
        "Slip",
        "Flat",
        "Squeeze",
    };
    
    const char* name = names[cf_rnd_range_int(&s_app->world.name_rnd, 0, CF_ARRAY_SIZE(names) - 1)];
    return cf_sintern(name);
}

const char* generate_tentacle_name()
{
    const char* prefixes[] =
    {
        "Long",
        "Short",
        "Snap",
        "Stink"
    };
    
    const char* suffixes[] =
    {
        "tip",
        "top",
        "whack",
        "stink",
    };
    
    const char* prefix = prefixes[cf_rnd_range_int(&s_app->world.name_rnd, 0, CF_ARRAY_SIZE(prefixes) - 1)];
    const char* suffix = suffixes[cf_rnd_range_int(&s_app->world.name_rnd, 0, CF_ARRAY_SIZE(suffixes) - 1)];
    const char* name = scratch_fmt("%s%s", prefix, suffix);;
    return cf_sintern(name);
}

const char* generate_tubeman_name()
{
    const char* names[] =
    {
        "Tuby",
        "Bendy",
        "Grabby",
        "Wiggle",
        "Waggle",
        "Shifty",
    };
    
    const char* name = names[cf_rnd_range_int(&s_app->world.name_rnd, 0, CF_ARRAY_SIZE(names) - 1)];
    
    return cf_sintern(name);
}