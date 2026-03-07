#ifndef STRINGS_H
#define STRINGS_H

fixed char* scratch_string(s32 length);
const char* scratch_vfmt(const char* fmt, va_list args);
const char* scratch_fmt(const char* fmt, ...);

const char* arena_vfmt(CF_Arena* arena, const char* fmt, va_list args);
const char* arena_fmt(CF_Arena* arena, const char* fmt, ...);

#endif //STRINGS_H
