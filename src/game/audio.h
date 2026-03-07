#ifndef AUDIO_H
#define AUDIO_H

typedef u8 Audio_Type;
enum
{
    Audio_Type_Music,
    Audio_Type_Sfx,
    Audio_Type_UI,
};

typedef struct Audio_Source
{
    const char* name;
    CF_Sound sound;
    Audio_Type type;
} Audio_Source;

typedef struct Audio_Play_Command
{
    const char* name;
    f32 pitch;
    b32 looped;
    Audio_Type type;
} Audio_Play_Command;

typedef struct Audio
{
    struct
    {
        f32 master;
        f32 music;
        f32 sfx;
        f32 ui;
    } volume;
    
    dyna Audio_Source* sources;
    dyna Audio_Play_Command* play_queue;
    
    CF_Rnd rnd;
} Audio;

void init_audio();
void update_audio();

b32 audio_is_duplicated_sound(const char* name, Audio_Type type);

void audio_play_music(const char* name);
void audio_play_sfx(const char* name);
void audio_play_sfx_rand_pitch(const char* name);

void audio_play_ui(const char* name);

#endif //AUDIO_H
