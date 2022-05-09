CC = gcc

CFLAGS  = -g -Wall -w

TARGET = Action2WithoutSorting

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c -o $(TARGET).out -lpthread -lm

clean:
	$(RM) $(TARGET).out
