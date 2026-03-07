#include "common/strings.h"

fixed char* scratch_string(s32 length)
{
    fixed char* text = NULL;
    cf_string_static(text, scratch_alloc(length), length);;
    return text;
}

const char* scratch_vfmt(const char* fmt, va_list args)
{
    u64 length = vsnprintf(NULL, 0, fmt, args) + 1;
    char* text = (char*)scratch_alloc(length);
    vsnprintf(text, length, fmt, args);
    return text;
}

const char* scratch_fmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const char* text = scratch_vfmt(fmt, args);
    va_end(args);
    
    return text;
}

const char* arena_vfmt(CF_Arena* arena, const char* fmt, va_list args)
{
    u64 length = vsnprintf(NULL, 0, fmt, args) + 1;
    char* text = (char*)cf_arena_alloc(arena, (s32)length);
    vsnprintf(text, length, fmt, args);
    return text;
}

const char* arena_fmt(CF_Arena* arena, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const char* text = arena_vfmt(arena, fmt, args);
    va_end(args);
    
    return text;
}