CC = mpicc
CFLAGS = -Wall -O3
LDFLAGS = -lm

TARGET = matriz
SRC = matriz.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
