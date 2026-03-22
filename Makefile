CC = gcc
CFLAGS = -Wall -I./src -I./ext/include
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

TARGET = build/gb.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q build\*.o build\*.exe
