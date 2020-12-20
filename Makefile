CFLAGS = -std=gnu99 -Wall -Wextra -pedantic $(shell pkg-config --cflags sdl2)
LIBS = -ldl $(shell pkg-config --libs sdl2)

ifeq ($(RELEASE), 1)
  CFLAGS += -O2 -s
else
  CFLAGS += -Og -ggdb3 -fsanitize=address
endif

libretro: main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
