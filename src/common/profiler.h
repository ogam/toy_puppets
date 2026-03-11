#ifndef PROFILER_H
#define PROFILER_H

typedef struct Profile_Sample
{
    const char* name;
    const char* file;
    s32 line;
    
    f64 duration;
    u64 start;
    u64 end;
} Profile_Sample;

typedef struct Profile_Frame
{
    s32 frame;
    dyna Profile_Sample* samples;
    dyna s32* stack;
} Profile_Frame;

typedef struct Profiler
{
    s32 frame;
    dyna Profile_Frame* frames;
    
    s32 write_index;
    s32 read_index;
    b32 is_paused;
    f64 inv_freq;
    
    CF_File* file;
    dyna char* buffer;
} Profiler;

extern Profiler* s_profiler;

#ifndef PROFILE_BEGIN_STR
#define PROFILE_BEGIN_STR(NAME) profile_begin(NAME, __FILE__, __LINE__)
#endif

#ifndef PROFILE_BEGIN
#define PROFILE_BEGIN() PROFILE_BEGIN_STR(__func__)
#endif

#ifndef PROFILE_END
#define PROFILE_END() profile_end()
#endif

#ifndef PROFILE_SECTION
#define PROFILE_SECTION(NAME) for(int ___x = (PROFILE_BEGIN_STR(NAME), 0); ___x < 1; PROFILE_END(), ++___x)
#endif

#ifndef PROFILE_FUNC
#define PROFILE_FUNC(FUNC) { PROFILE_SECTION(#FUNC) { FUNC(); } }
#endif

void init_profiler(s32 ring_buffer_size);

// dups out as json trace events so something like google
void profile_file_stream_begin(const char* file_name);
void profile_file_stream_end();
void profile_file_write_sample(Profile_Sample* sample);

void profile_begin(const char* name, const char* file, s32 line);
void profile_end();
void profiler_pause(b32 true_to_pause);
b32 profiler_is_paused();
Profiler* profiler_get();

#endif //PROFILER_H
