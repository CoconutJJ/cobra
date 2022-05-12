CC=g++
OBJ=bytecode.o compiler.o scanner.o symbols.o cobra.o function.o vm.o
FLAGS=-Ofast -Wall

all: cobrac clean
debug: FLAGS=-Og -g -Wall
debug: cobrac
cobrac: $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o cobrac

%.o: %.cpp
	$(CC) $(FLAGS) -c -o $@ $*.cpp

.PHONY: clean

clean:
	rm $(OBJ)	