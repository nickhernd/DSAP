CC = mpicc
CFLAGS = -Wall -O3
TARGET = contar_mpi

all: $(TARGET)

$(TARGET): contar_mpi.c
	$(CC) $(CFLAGS) -o $(TARGET) contar_mpi.c

clean:
	rm -f $(TARGET)