CC		= gcc
CFLAGS 	= -Wall -Werror -pthread
DEPS = mapreduce.h threadpool.h
TARGET	= distwc
OBJ		= distwc.o mapreduce.o threadpool.o

All: $(TARGET) clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ)