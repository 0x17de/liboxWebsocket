CC=gcc
CFLAGS=-g -Wall -Werror -pedantic

_OBJ = main.o websocket.o
OBJ = $(patsubst %,obj/%,$(_OBJ))

_DEPS = websocket.h
DEPS = $(patsubst %,src/%,$(_DEPS))


client: $(OBJ)
	$(CC) -o $@ $^

obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f obj/*.o
