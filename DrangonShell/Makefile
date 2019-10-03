CC		= gcc
CFLAGS 	= -Wall
SOURCE 	= dragonshell.c
TARGET	= dragonshell
OBJ		= dragonshell.o

all: $(TARGET) clean zip

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)
	
clean:
	rm -f $(OBJ)

zip:
	zip dragonshell.zip ./README.md ./Makefile ./dragonshell ./dragonshell.c 
