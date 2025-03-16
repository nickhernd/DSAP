# Makefile para la práctica tcom
# Evaluación de los parámetros del modelo de coste de comunicaciones en MPI

# Compilador
CC = mpicc

# Opciones de compilación
CFLAGS = -Wall -O2

# Librerías
LIBS = -lm

# Nombre del ejecutable
TARGET = tcom

# Ficheros fuente
SRCS = tcom.c

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

# Ejecutar el programa con 2 procesos
run: $(TARGET)
	mpirun -np 2 ./$(TARGET)

# Limpiar archivos temporales
clean:
	rm -f $(OBJS) $(TARGET)

# Dependencias
tcom.o: tcom.c