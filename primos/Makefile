# Makefile para la práctica de primos con MPI

# Compilador
CC = mpicc

# Opciones de compilación
CFLAGS = -Wall -O

# Librerías
LIBS = -lm

# Nombre del ejecutable
TARGET = primos_mpi

# Ficheros fuente
SRCS = primos_mpi.c

# Objetos
OBJS = $(SRCS:.c=.o)

# Regla por defecto
all: $(TARGET)

# Regla para el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Regla para compilar .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Limpiar archivos temporales
clean:
	rm -f $(OBJS) $(TARGET)

# Dependencias
primos_mpi.o: primos_mpi.c
