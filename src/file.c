#include "file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

bool FileToMemory(const char *filename, unsigned char **buffer, size_t *size)
{
	FILE *file = fopen(filename, "rb");

	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		*size = ftell(file);
		*buffer = malloc(*size);
		rewind(file);
		fread(*buffer, 1, *size, file);
		fclose(file);

		return true;
	}
	else
	{
		return false;
	}
}

bool MemoryToFile(const char *filename, const unsigned char *buffer, size_t size)
{
	FILE *file = fopen(filename, "wb");

	if (file != NULL)
	{
		fwrite(buffer, 1, size, file);
		fclose(file);

		return true;
	}
	else
	{
		return false;
	}
}
