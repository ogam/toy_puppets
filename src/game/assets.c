#include "game/assets.h"

char* mount_get_directory_path()
{
    //  mounting vfs
    char* path = cf_path_normalize(cf_fs_get_base_directory());
    char* dir = cf_path_directory_of(path);
    s32 directory_depth = 0;
    //  running from debugger
    if (cf_string_iequ(dir, "/build"))
    {
        directory_depth = 2;
    }
    //  running from debug build
    else if (cf_string_equ(dir, "/Debug") || cf_string_equ(dir, "/Release"))
    {
        directory_depth = 2;
    }
    cf_string_free(dir);
    path = cf_path_pop_n(path, directory_depth);
    
    return path;
}

void mount_root_read_directory()
{
    char* path = mount_get_directory_path();
    cf_fs_mount(path, "/", false);
    cf_string_free(path);
}

void mount_root_write_directory()
{
    char* path = mount_get_directory_path();
    cf_fs_set_write_directory(path);
    cf_string_free(path);
}

void dismount_root_directory()
{
    char* path = mount_get_directory_path();
    cf_fs_dismount(path);
    cf_string_free(path);
}

void mount_data_read_directory()
{
    char* path = mount_get_directory_path();
    cf_string_append(path, "/data");
    CF_Result result = cf_fs_mount(path, "/", false);
    if (result.code != CF_RESULT_SUCCESS)
    {
        printf("failed to mount: %s\n", result.details);
    }
    cf_string_free(path);
}

void mount_data_write_directory()
{
    char* path = mount_get_directory_path();
    cf_string_append(path, "/data");
    cf_fs_set_write_directory(path);
    cf_string_free(path);
}

void dismount_data_directory()
{
    char* path = mount_get_directory_path();
    cf_string_append(path, "/data");
    cf_fs_dismount(path);
    cf_string_free(path);
}

void assets_load_all()
{
    Assets* assets = &s_app->assets;
    
    mount_data_read_directory();
    {
        CF_Sprite default_sprite = cf_make_demo_sprite();
        cf_map_set(assets->sprites, cf_sintern(ASSETS_DEFAULT), default_sprite);
    }
    
    char buffer[1024];
    
    // sprites
    {
        const char** files = cf_fs_enumerate_directory("sprites");
        for (const char** file = files; *file; ++file)
        {
            
            if (cf_path_ext_equ(*file, ".ase") ||
                cf_path_ext_equ(*file, ".aseprite"))
            {
                CF_SNPRINTF(buffer, sizeof(buffer), "sprites/%s", *file);
                CF_Sprite sprite = cf_make_sprite(buffer);
                
                cf_sprite_add_blend(buffer, (const char*[]){ "Layer" }, ASSETS_BLEND_LAYER_DEFAULT);
                cf_sprite_add_blend(buffer, (const char*[]){ "Layer", "tag_negative" }, ASSETS_BLEND_LAYER_NEG);
                
                cf_map_set(assets->sprites, cf_sintern(buffer), sprite);
                printf("Loaded %s\n", buffer);
            }
            else if (cf_path_ext_equ(*file, ".png"))
            {
                CF_SNPRINTF(buffer, sizeof(buffer), "sprites/%s", *file);
                CF_Result result = { 0 };
                CF_Sprite sprite = cf_make_easy_sprite_from_png(buffer, &result);
                if (result.code == CF_RESULT_SUCCESS)
                {
                    cf_map_set(assets->sprites, cf_sintern(buffer), sprite);
                    printf("Loaded %s\n", buffer);
                }
                else
                {
                    printf("Failed to load %s\n", buffer);
                    printf("\t%s\n", result.details);
                }
            }
        }
        cf_fs_free_enumerated_directory(files);
    }
    // sounds
    {
        const char** files = cf_fs_enumerate_directory("sounds");
        for (const char** file = files; *file; ++file)
        {
            if (cf_path_ext_equ(*file, ".ogg"))
            {
                CF_SNPRINTF(buffer, sizeof(buffer), "sounds/%s", *file);
                CF_Audio audio = cf_audio_load_ogg(buffer);
                cf_map_set(assets->audios, cf_sintern(buffer), audio);
                printf("Loaded %s\n", buffer);
            }
            else if (cf_path_ext_equ(*file, ".wav"))
            {
                CF_SNPRINTF(buffer, sizeof(buffer), "sounds/%s", *file);
                CF_Audio audio = cf_audio_load_wav(buffer);
                cf_map_set(assets->audios, cf_sintern(buffer), audio);
                printf("Loaded %s\n", buffer);
            }
        }
        cf_fs_free_enumerated_directory(files);
    }
}

CF_Sprite assets_get_sprite(const char* name)
{
    Assets* assets = &s_app->assets;
    name = cf_sintern(name);
    CF_Sprite sprite = { 0 };
    if (cf_map_has(assets->sprites, name))
    {
        sprite = cf_map_get(assets->sprites, name);
        sprite.blend_index = ASSETS_BLEND_LAYER_DEFAULT;
    }
    else
    {
        printf("Failed to find %s\n", name);
        sprite = cf_map_get(assets->sprites, cf_sintern(ASSETS_DEFAULT));
    }
    return sprite;
}

CF_Audio assets_get_audio(const char* name)
{
    Assets* assets = &s_app->assets;
    name = cf_sintern(name);
    CF_Audio audio = { 0 };
    
    if (cf_map_has(assets->audios, name))
    {
        audio = cf_map_get(assets->audios, name);
    }
    else
    {
        printf("Failed to find %s\n", name);
    }
    
    return audio;
}