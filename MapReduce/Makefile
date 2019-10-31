CC		= gcc
CFLAGS 	= -Wall -Werror -pthread
DEPS = mapreduce.h threadpool.h
SOURCE = distwc.c mapreduce.c threadpool.c
TARGET	= wordcount
OBJ		= distwc.o mapreduce.o threadpool.o

All: $(TARGET) clean compress

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ)

compress:
	zip mapreduce.zip ./README.md ./Makefile ./$(DEPS) ./$(SOURCE) ./$(TARGET)

	