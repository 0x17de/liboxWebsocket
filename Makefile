CC=gcc
CFLAGS=-g -Wall -Werror -pedantic

_ALLOBJ = websocket.o
_TOBJ = test.o $(_ALLOBJ)
_OBJ = main.o $(_ALLOBJ)
TOBJ = $(patsubst %,obj/%,$(_TOBJ))
OBJ = $(patsubst %,obj/%,$(_OBJ))

_DEPS = websocket.h
DEPS = $(patsubst %,src/%,$(_DEPS))


client: $(OBJ)
	$(CC) -o $@ $^

all: client test

test: $(TOBJ)
	$(CC) -o $@ $^

obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f obj/*.o client test

