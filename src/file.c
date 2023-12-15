#include "file.h"

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"

bool FileToMemory(const char *filename, unsigned char **buffer, size_t *size)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");

	if (file != NULL)
	{
		SDL_RWseek(file, 0, RW_SEEK_END);
		*size = SDL_RWtell(file);
		*buffer = SDL_malloc(*size);
		SDL_RWseek(file, 0, RW_SEEK_SET);
		SDL_RWread(file, *buffer, 1, *size);
		SDL_RWclose(file);

		return true;
	}
	else
	{
		return false;
	}
}

bool MemoryToFile(const char *filename, const unsigned char *buffer, size_t size)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "wb");

	if (file != NULL)
	{
		SDL_RWwrite(file, buffer, 1, size);
		SDL_RWclose(file);

		return true;
	}
	else
	{
		return false;
	}
}
