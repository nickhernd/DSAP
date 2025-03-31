CC = mpicc
CFLAGS = -Wall -O3 -lm
TARGET = tcom

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) *.o resultados_tcom.csv

.PHONY: all clean
