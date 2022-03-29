CC=g++
OBJ=vm.o
FLAGS=-Ofast -s -Wall

all: cobra clean
debug: FLAGS=-Og -g -Wall
debug: cobra clean
cobra: $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o cobra

%.o: %.cpp
	$(CC) $(FLAGS) -c -o $@ $*.cpp

.PHONY: clean

clean:
	rm $(OBJ)	
