CC = gcc

CFLAGS  = -g -Wall -w

TARGET = Action2WithoutSorting

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET).out $(TARGET).c

clean:
	$(RM) $(TARGET).out
