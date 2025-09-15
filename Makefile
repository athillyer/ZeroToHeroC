TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./bin/dbview -f ./mynewdb.db -n
	./bin/dbview -f ./mynewdb.db -a "Test Person, 123 Fake St, 2000"
	./bin/dbview -f ./mynewdb.db

default: $(TARGET)

clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -c $< -o $@ -Iinclude


