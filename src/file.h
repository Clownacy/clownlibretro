#pragma once

#include <stdbool.h>
#include <stddef.h>

bool FileToMemory(const char *filename, unsigned char **buffer, size_t *size);
bool MemoryToFile(const char *filename, const unsigned char *buffer, size_t size);
