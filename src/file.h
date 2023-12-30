#pragma once

#include <stddef.h>

#include "clowncommon/clowncommon.h"

cc_bool ReadFileToAllocatedBuffer(const char *filename, unsigned char **buffer, size_t *size);
cc_bool WriteBufferToFile(const char *filename, const void *buffer, size_t size);
cc_bool ReadFileToBuffer(const char* const filename, void* const buffer, const size_t size);
