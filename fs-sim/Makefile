CC		= gcc
CFLAGS 	= -Wall
DEPS = FileSystem.h func.h
SOURCE = main.c FileSystem.c func.c
TARGET	= fs
OBJ		= main.o FileSystem.o func.o

All: $(TARGET) clean compress

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ)

compress:
	zip fs-sim.zip ./README.md ./Makefile ./$(DEPS) ./$(SOURCE) ./$(TARGET)