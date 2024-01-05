#include "error.h"

#include <stdio.h>

#define DO_PRINTER(CATEGORY)\
void Print##CATEGORY##V(const char* const format, va_list args)\
{\
	fputs(#CATEGORY ": ", stderr);\
	vfprintf(stderr, format, args);\
	fputc('\n', stderr);\
}\
\
void Print##CATEGORY(const char* const format, ...)\
{\
	va_list args;\
	va_start(args, format);\
	Print##CATEGORY##V(format, args);\
	va_end(args);\
}

DO_PRINTER(Debug)
DO_PRINTER(Info)
DO_PRINTER(Warning)
DO_PRINTER(Error)

#undef DO_PRINTER
