CC=g++
OBJ=bytecode.o compiler.o scanner.o symbols.o cobra.o
FLAGS=-Ofast -s -Wall

all: cobra clean

cobra: $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o cobra

%.o: %.cpp
	$(CC) $(FLAGS) -c -o $@ $*.cpp

.PHONY: clean

clean:
	rm $(OBJ)	

