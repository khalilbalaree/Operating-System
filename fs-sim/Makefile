CC		= gcc
CFLAGS 	= -Wall
DEPS = FileSystem.h func.h
SOURCE = main.c FileSystem.c func.c
TARGET	= fs
OBJ		= main.o FileSystem.o func.o

All: $(TARGET) clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ)