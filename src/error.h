#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#include "clowncommon/clowncommon.h"

/* Namespacing, to prevent linker conflicts with static cores. */
#define PrintDebug ClownLibretro_PrintDebug
#define PrintInfo ClownLibretro_PrintInfo
#define PrintWarning ClownLibretro_PrintWarning
#define PrintError ClownLibretro_PrintError

void PrintDebugV(const char* const format, va_list args);
void PrintInfoV(const char* const format, va_list args);
void PrintWarningV(const char* const format, va_list args);
void PrintErrorV(const char* const format, va_list args);

CC_ATTRIBUTE_PRINTF(1, 2) void PrintDebug(const char* const format, ...);
CC_ATTRIBUTE_PRINTF(1, 2) void PrintInfo(const char* const format, ...);
CC_ATTRIBUTE_PRINTF(1, 2) void PrintWarning(const char* const format, ...);
CC_ATTRIBUTE_PRINTF(1, 2) void PrintError(const char* const format, ...);

#endif /* ERROR_H */
