CC=g++
OBJ=bytecode.o compiler.o scanner.o symbols.o cobra.o
FLAGS=-Ofast -Wall

all: cobra

cobra: $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o cobra

%.o: %.cpp
	$(CC) $(FLAGS) -c -o $@ $*.cpp

clean:
	rm $(OBJ)	

