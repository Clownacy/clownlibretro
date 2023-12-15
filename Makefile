SOURCE_DIRECTORY = src
BINARY_DIRECTORY = bin
ifeq ($(RELEASE), 1)
 OBJECT_DIRECTORY = obj/release
else
 OBJECT_DIRECTORY = obj/debug
endif

CFLAGS = -std=gnu99 -Wall -Wextra -pedantic $(shell pkg-config --cflags sdl2 freetype2) -DFREETYPE_FONTS
LIBS = -lm $(shell pkg-config --libs sdl2 freetype2)

SOURCES = main.c audio.c core_runner.c file.c font.c input.c menu.c video.c
OBJECTS = $(SOURCES:%.c=$(OBJECT_DIRECTORY)/%.o)

ifeq ($(RELEASE), 1)
  CFLAGS += -O2 -s
else
  CFLAGS += -Og -ggdb3 -fsanitize=address
endif

$(OBJECT_DIRECTORY)/%.o: $(SOURCE_DIRECTORY)/%.c
	mkdir -p $(OBJECT_DIRECTORY)
	$(CC) $(CFLAGS) -o $@ $^ -c

$(BINARY_DIRECTORY)/clownlibretro: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
