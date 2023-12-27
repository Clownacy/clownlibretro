#include "file.h"

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"

bool FileToMemory(const char* const filename, unsigned char** const buffer, size_t* const size)
{
	bool success = false;

	SDL_RWops* const file = SDL_RWFromFile(filename, "rb");

	if (file == NULL)
		return false;

	SDL_RWseek(file, 0, RW_SEEK_END);
	*size = SDL_RWtell(file);
	*buffer = (unsigned char*)SDL_malloc(*size);

	if (*buffer != NULL)
	{
		SDL_RWseek(file, 0, RW_SEEK_SET);
		SDL_RWread(file, *buffer, 1, *size);

		success = true;
	}

	SDL_RWclose(file);

	return success;
}

bool MemoryToFile(const char* const filename, const void* const buffer, const size_t size)
{
	SDL_RWops* const file = SDL_RWFromFile(filename, "wb");

	if (file == NULL)
		return false;

	SDL_RWwrite(file, buffer, 1, size);
	SDL_RWclose(file);

	return true;
}
