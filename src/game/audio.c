#include "game/audio.h"

void init_audio()
{
    Audio* audio = &s_app->audio;
    audio->volume.master = 0.5f;
    audio->volume.music = 1.0f;
    audio->volume.sfx = 1.0f;
    audio->volume.ui = 1.0f;
    
    cf_array_fit(audio->sources, 128);
    cf_array_fit(audio->play_queue, 64);
    
    audio->rnd = cf_rnd_seed(cf_get_tick_frequency());
}

void update_audio()
{
    Audio* audio = &s_app->audio;
    
    f32 mixed_sfx = audio->volume.master * audio->volume.sfx;
    f32 mixed_music = audio->volume.master * audio->volume.music;
    f32 mixed_ui = audio->volume.master * audio->volume.ui;
    
    // remove any sounds that are done
    for (s32 index = cf_array_count(audio->sources) - 1; index >= 0; --index)
    {
        Audio_Source* source = audio->sources + index;
        if (!cf_sound_is_active(source->sound))
        {
            cf_array_del(audio->sources, index);
        }
    }
    
    // play new sounds
    for (s32 index = 0; index < cf_array_count(audio->play_queue); ++index)
    {
        Audio_Play_Command command = audio->play_queue[index];
        CF_Audio clip = assets_get_audio(command.name);
        if (clip.id)
        {
            Audio_Source source = { 0 };
            source.name = command.name;
            source.type = command.type;
            
            CF_SoundParams params = cf_sound_params_defaults();
            params.pitch = command.pitch;
            params.looped = command.looped;
            switch (command.type)
            {
                case Audio_Type_Music:
                {
                    params.volume = mixed_music;
                    break;
                }
                case Audio_Type_Sfx:
                {
                    params.volume = mixed_sfx;
                    break;
                }
                case Audio_Type_UI:
                {
                    params.volume = mixed_ui;
                    break;
                }
            }
            
            source.sound = cf_play_sound(clip, params);
            cf_array_push(audio->sources, source);
        }
    }
    
    // update volumes
    for (s32 index = 0; index < cf_array_count(audio->sources); ++index)
    {
        Audio_Source* source = audio->sources + index;
        switch (source->type)
        {
            case Audio_Type_Music:
            {
                cf_sound_set_volume(source->sound, mixed_music);
                break;
            }
            case Audio_Type_Sfx:
            {
                cf_sound_set_volume(source->sound, mixed_sfx);
                break;
            }
            case Audio_Type_UI:
            {
                cf_sound_set_volume(source->sound, mixed_ui);
                break;
            }
        }
    }
    
    cf_array_clear(audio->play_queue);
}

b32 audio_is_duplicated_sound(const char* name, Audio_Type type)
{
    Audio* audio = &s_app->audio;
    b32 is_duplicated = false;
    
    for (s32 index = 0; index < cf_array_count(audio->play_queue); ++index)
    {
        Audio_Play_Command command = audio->play_queue[index];
        if (command.name == name && command.type == type)
        {
            is_duplicated = true;
            break;
        }
    }
    
    return is_duplicated;
}

void audio_play_music(const char* name)
{
    Audio* audio = &s_app->audio;
    name = cf_sintern(name);
    
    if (!audio_is_duplicated_sound(name, Audio_Type_Music))
    {
        Audio_Play_Command command = { 0 };
        command.pitch = 1.0f;
        command.looped = true;
        command.type = Audio_Type_Music;
        command.name = name;
        cf_array_push(audio->play_queue, command);
    }
}

void audio_play_sfx_pitch(const char* name, f32 pitch)
{
    Audio* audio = &s_app->audio;
    name = cf_sintern(name);
    
    if (!audio_is_duplicated_sound(name, Audio_Type_Sfx))
    {
        Audio_Play_Command command = { 0 };
        command.pitch = 1.0f;
        command.type = Audio_Type_Sfx;
        command.name = name;
        cf_array_push(audio->play_queue, command);
    }
}

void audio_play_sfx(const char* name)
{
    audio_play_sfx_pitch(name, 1.0f);
}

void audio_play_sfx_rand_pitch(const char* name)
{
    f32 pitch = cf_rnd_range_float(&s_app->audio.rnd, 0.8f, 1.0f);
    audio_play_sfx_pitch(name, pitch);
}

void audio_play_ui(const char* name)
{
    Audio* audio = &s_app->audio;
    name = cf_sintern(name);
    
    if (!audio_is_duplicated_sound(name, Audio_Type_UI))
    {
        Audio_Play_Command command = { 0 };
        command.pitch = 1.0f;
        command.type = Audio_Type_UI;
        command.name = name;
        cf_array_push(audio->play_queue, command);
    }
}
