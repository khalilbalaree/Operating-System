CC		= gcc
CFLAGS 	= -Wall
SOURCE 	= dragonshell.c
TARGET	= dragonshell
OBJ		= dragonshell.o

all: $(TARGET) clean tar

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)
	
clean:
	rm -f $(OBJ)

tar:
	tar -czvf submit.tar ./README.md ./Makefile ./dragonshell ./dragonshell.c 
