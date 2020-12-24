CFLAGS = -std=gnu99 -Wall -Wextra -pedantic $(shell pkg-config --cflags sdl2 libzip freetype2) -DFREETYPE_FONTS
LIBS = -ldl -lm $(shell pkg-config --libs sdl2 libzip freetype2)

ifeq ($(RELEASE), 1)
  CFLAGS += -O2 -s
else
  CFLAGS += -Og -ggdb3 -fsanitize=address
endif

libretro: main.c audio.c core_runner.c file.c font.c input.c menu2.c video.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
