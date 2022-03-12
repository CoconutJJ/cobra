CC=g++
OBJ=bytecode.o compiler.o scanner.o symbols.o cobra.o
FLAGS=-O3 -Wall

all: cobra

cobra: $(OBJ)
	$(CC) $(OBJ) -o cobra

%.o: *.cpp
	$(CC) $(FLAGS) -c -o $@ $*.cpp

clean:
	rm $(OBJ)

