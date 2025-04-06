CC = mpicc
CFLAGS = -Wall -O3 -lm
TARGET = primos_mpi

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean