#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#include "clowncommon/clowncommon.h"

void PrintDebugV(const char* const format, va_list args);
void PrintInfoV(const char* const format, va_list args);
void PrintWarningV(const char* const format, va_list args);
void PrintErrorV(const char* const format, va_list args);

CC_ATTRIBUTE_PRINTF(1, 2) void PrintDebug(const char* const format, ...);
CC_ATTRIBUTE_PRINTF(1, 2) void PrintInfo(const char* const format, ...);
CC_ATTRIBUTE_PRINTF(1, 2) void PrintWarning(const char* const format, ...);
CC_ATTRIBUTE_PRINTF(1, 2) void PrintError(const char* const format, ...);

#endif /* ERROR_H */
