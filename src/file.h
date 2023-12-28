#pragma once

#include <stdbool.h>
#include <stddef.h>

bool ReadFileToAllocatedBuffer(const char *filename, unsigned char **buffer, size_t *size);
bool WriteBufferToFile(const char *filename, const void *buffer, size_t size);
bool ReadFileToBuffer(const char* const filename, void* const buffer, const size_t size);
