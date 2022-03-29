CC=g++
OBJ=bytecode.o compiler.o scanner.o symbols.o cobra.o function.o
FLAGS=-Ofast -s -Wall

all: cobrac clean
debug: FLAGS=-Og -g -Wall
debug: cobra clean
cobrac: $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o cobrac

%.o: %.cpp
	$(CC) $(FLAGS) -c -o $@ $*.cpp

.PHONY: clean

clean:
	rm $(OBJ)	

