OBJECT_DIRECTORY = obj

CFLAGS = -std=gnu99 -Wall -Wextra -pedantic $(shell pkg-config --cflags sdl2 libzip freetype2) -DFREETYPE_FONTS
LIBS = -lm $(shell pkg-config --libs sdl2 libzip freetype2)

SOURCES = main.c audio.c core_runner.c file.c font.c input.c menu.c video.c
OBJECTS = $(SOURCES:%.c=$(OBJECT_DIRECTORY)/%.o)

ifeq ($(RELEASE), 1)
  CFLAGS += -O2 -s
else
  CFLAGS += -Og -ggdb3 -fsanitize=address
endif

$(OBJECT_DIRECTORY)/%.o: %.c
	mkdir -p $(OBJECT_DIRECTORY)
	$(CC) $(CFLAGS) -o $@ $^ -c

libretro: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
