#include "file.h"

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"

bool ReadFileToAllocatedBuffer(const char* const filename, unsigned char** const buffer, size_t* const size)
{
	bool success = false;

	SDL_RWops* const file = SDL_RWFromFile(filename, "rb");

	if (file != NULL)
	{
		*size = SDL_RWsize(file);
		*buffer = (unsigned char*)SDL_malloc(*size);

		if (*buffer != NULL)
		{
			SDL_RWread(file, *buffer, 1, *size);

			success = true;
		}

		SDL_RWclose(file);
	}

	return success;
}

bool WriteBufferToFile(const char* const filename, const void* const buffer, const size_t size)
{
	bool success = false;

	SDL_RWops* const file = SDL_RWFromFile(filename, "wb");

	if (file != NULL)
	{
		SDL_RWwrite(file, buffer, 1, size);
		SDL_RWclose(file);
		success = true;
	}

	return success;
}

bool ReadFileToBuffer(const char* const filename, void* const buffer, const size_t size)
{
	bool success = false;

	SDL_RWops* const file = SDL_RWFromFile(filename, "rb");

	if (file != NULL)
	{
		SDL_RWread(file, buffer, 1, size);
		SDL_RWclose(file);
		success = true;
	}

	return success;
}
