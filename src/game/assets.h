#ifndef ASSETS_H
#define ASSETS_H

#ifndef ASSETS_DEFAULT
#define ASSETS_DEFAULT "__default"
#endif

#ifndef ASSETS_BLEND_LAYER_ALL
#define ASSETS_BLEND_LAYER_ALL 0
#endif

#ifndef ASSETS_BLEND_LAYER_DEFAULT
#define ASSETS_BLEND_LAYER_DEFAULT 1
#endif

#ifndef ASSETS_BLEND_LAYER_NEG
#define ASSETS_BLEND_LAYER_NEG 2
#endif

typedef struct Assets
{
    CF_MAP(CF_Sprite) sprites;
    CF_MAP(CF_Audio) audios;
} Assets;

char* mount_get_directory_path();
void mount_root_read_directory();
void mount_root_write_directory();
void dismount_root_directory();
void mount_data_read_directory();
void mount_data_write_directory();
void dismount_data_directory();

void assets_load_all();
CF_Sprite assets_get_sprite(const char* name);
CF_Audio assets_get_audio(const char* name);

#endif //ASSETS_H
