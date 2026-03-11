#include "common/profiler.h"

Profiler* s_profiler;

void init_profiler(s32 ring_buffer_size)
{
    s_profiler = cf_alloc(sizeof(*s_profiler));
    CF_MEMSET(s_profiler, 0, sizeof(*s_profiler));
    
    cf_array_fit(s_profiler->frames, ring_buffer_size);
    cf_array_setlen(s_profiler->frames, cf_array_capacity(s_profiler->frames));
    CF_MEMSET(s_profiler->frames, 0, sizeof(*s_profiler->frames) * cf_array_count(s_profiler->frames));
    
    for (s32 index = 0; index < cf_array_count(s_profiler->frames); ++index)
    {
        cf_array_fit(s_profiler->frames[index].samples, 1024);
        cf_array_fit(s_profiler->frames[index].stack, 1024);
    }
    
    s_profiler->inv_freq = 1.0 / cf_get_tick_frequency();
    
    cf_string_fit(s_profiler->buffer, KB(64));
}

void profile_file_stream_begin(const char* file_name)
{
    if (!s_profiler->file)
    {
        //  mounting vfs
        char* path = cf_path_normalize(cf_fs_get_base_directory());
        
        cf_fs_set_write_directory(path);
        
        s_profiler->file = cf_fs_open_file_for_write(file_name);
        
        cf_fs_dismount(path);
        cf_string_free(path);
        
        char buffer[1024];
        size_t length = CF_SNPRINTF(buffer, sizeof(buffer), 
                                    "{\n" \
                                    "\"displayTimeUnit\": \"ms\",\n" \
                                    "\"systemTraceEvents\": \"SystemTraceData\",\n" \
                                    "\"traceEvents\": [\n");
        
        cf_fs_write(s_profiler->file, buffer, length);
    }
}

void profile_file_stream_end()
{
    if (s_profiler->file)
    {
        // write any remainder
        if (cf_string_count(s_profiler->buffer) > 0)
        {
            cf_fs_write(s_profiler->file, s_profiler->buffer, cf_string_count(s_profiler->buffer));
            cf_string_clear(s_profiler->buffer);
        }
        
        size_t position = cf_fs_tell(s_profiler->file);
        // remove last comma
        cf_fs_seek(s_profiler->file, position - 3);
        cf_fs_write(s_profiler->file, "]\n}", 3);
        
        cf_fs_close(s_profiler->file);
        s_profiler->file = NULL;
    }
}

void profile_file_write_sample(Profile_Sample* sample)
{
    if (s_profiler->file)
    {
        char buffer[1024];
        
        f64 start = sample->start * s_profiler->inv_freq * 1000.0;
        f64 end = sample->end * s_profiler->inv_freq * 1000.0;
        CF_ThreadId tid = cf_thread_id();
        
        size_t length = CF_SNPRINTF(buffer, sizeof(buffer), 
                                    "{ \"name\": \"%s\", \"cat\": \"PERF\", \"ph\": \"B\", \"pid\": 0, \"tid\": %" PRIu64 ", \"ts\": %.0f },\n" \
                                    "{ \"name\": \"%s\", \"cat\": \"PERF\", \"ph\": \"E\", \"pid\": 0, \"tid\": %" PRIu64 ", \"ts\": %.0f },\n", 
                                    sample->name, tid, start, sample->name, tid, end);
        
        // only write whenever buffer is filled up to avoid hitting disk every sample
        if (cf_string_count(s_profiler->buffer) + length > cf_string_cap(s_profiler->buffer))
        {
            cf_fs_write(s_profiler->file, s_profiler->buffer, cf_string_count(s_profiler->buffer) - 1);
            cf_string_clear(s_profiler->buffer);
        }
        
        cf_string_append(s_profiler->buffer, buffer);
    }
}

void profile_begin(const char* name, const char* file, s32 line)
{
    Profile_Frame* frame = s_profiler->frames + s_profiler->write_index;
    
    s32 index = cf_array_count(frame->samples);
    
    Profile_Sample sample = { 0 };
    sample.name = name;
    sample.file = file;
    sample.line = line;
    sample.start = cf_get_ticks();
    cf_array_push(frame->samples, sample);
    cf_array_push(frame->stack, index);
}

void profile_end()
{
    Profile_Frame* frame = s_profiler->frames + s_profiler->write_index;
    
    CF_ASSERT(cf_array_count(frame->stack) > 0);
    
    s32 index = cf_array_pop(frame->stack);
    
    Profile_Sample* sample = frame->samples + index;
    
    sample->end = cf_get_ticks();
    // microseconds
    sample->duration = (sample->end - sample->start) * s_profiler->inv_freq * 1000.0;
    
    if (s_profiler->file)
    {
        profile_file_write_sample(sample);
    }
    
    if (cf_array_count(frame->stack) == 0)
    {
        if (!s_profiler->is_paused)
        {
            s_profiler->write_index = (s_profiler->write_index + 1) % cf_array_count(s_profiler->frames);
            if (s_profiler->write_index == s_profiler->read_index)
            {
                s_profiler->read_index = (s_profiler->read_index + 1) % cf_array_count(s_profiler->frames);
            }
        }
        
        Profile_Frame* new_frame = s_profiler->frames + s_profiler->write_index;
        new_frame->frame = ++s_profiler->frame;
        cf_array_clear(new_frame->samples);
        cf_array_clear(new_frame->stack);
    }
}

void profiler_pause(b32 true_to_pause)
{
    s_profiler->is_paused = true_to_pause;
}

b32 profiler_is_paused()
{
    return s_profiler->is_paused;
}

Profiler* profiler_get()
{
    return s_profiler;
}