#ifndef STRINGFUNCTIONS_H
#define STRINGFUNCTIONS_H

#include <string>
#include <stdarg.h>
#include "PsfxClasses.h"

   
extern std::string NextName(const std::string &Line);
extern std::string GetFilenameOnly(const std::string &str);
extern std::string GetDirectoryFromFilename(const std::string &str);
extern std::string NextName(const std::string &Line,size_t &idx);
extern std::string ExtractNextName(std::string &Line);
extern void ClipWhitespace(std::string &Line);
extern int NextInt(const std::string &Line,size_t &idx);
extern int NextInt(const std::string &Line);
extern bool NextBool(const std::string &Line);
extern const float *NextVector3(const std::string &Line);
extern const float *NextVector2(const std::string &Line);
extern float NextFloat(const std::string &Line,size_t &idx);
extern float NextFloat(const std::string &Line);
extern float NextSpeed(const std::string &Line,size_t &idx);
extern float NextTorque(const std::string &Line);
extern float AsPhysicalValue(const std::string &Line);
extern float NextPower(const std::string &Line);
extern float NextTorque(const std::string &Line,size_t &idx);
extern float NextPower(const std::string &Line,size_t &idx);
extern size_t GoToLine(const std::string &Str,size_t line);
extern std::string ToString(int i);
extern std::string ToString(float i);
extern std::string ToString(bool i);
//! Create a std::string using sprintf-style formatting, with variable arguments.
extern std::string stringFormat(std::string fmt, ...);
/// A quick-and-dirty, non-re-entrant formatting function. Use this only for debugging.
extern const char *QuickFormat(const char *format_str,...);
//! Proper find-and-replace function for strings:
extern void find_and_replace(std::string& source, std::string const& find, std::string const& replace);

std::string ToString(psfxParser::FillMode x);
std::string ToString(psfxParser::CullMode x);
std::string ToString(psfxParser::FilterMode x);
std::string ToString(psfxParser::AddressMode x);
extern std::string GetEnv(const std::string &name);
#endif

