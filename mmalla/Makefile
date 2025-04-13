CC = mpicc
CFLAGS = -Wall -O3
LDFLAGS = -lm

TARGET = mmalla_mpi
SRC = mmalla_mpi.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

run4:
	mpirun -np 4 ./$(TARGET)

run9:
	mpirun -np 9 ./$(TARGET)

run16:
	mpirun -np 16 ./$(TARGET)

.PHONY: clean run4 run9 run16