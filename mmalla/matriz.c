#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Valores máximos para las dimensiones de los bloques y la cantidad de bloques
#define rmax 4
#define maxbloqtam 100

// Función para multiplicar dos matrices almacenadas como vectores y acumular el resultado en la matriz c
void mult(double a[], double b[], double *c, int m) {
    int i, j, k;
    
    // CORRECIÓN: Inicializar c a cero ANTES de la multiplicación
    for (i = 0; i < m * m; i++) {
        c[i] = 0.0;
    }
    
    for (i = 0; i < m; i++) {
        for (j = 0; j < m; j++) {
            for (k = 0; k < m; k++) {
                c[i * m + j] += a[i * m + k] * b[k * m + j];
            }
        }
    }
}

// Función para generar la matriz original A según la función de inicialización
void generar_matriz_original(double *original, int bloqtam, int fila, int columna) {
    int i;
    for (i = 0; i < bloqtam * bloqtam; i++) {
        original[i] = (i + 1) * (float)(fila * columna + 1) * (float)(fila * columna + 1) / (bloqtam * bloqtam);
    }
}

int main(int argc, char *argv[]) {
    int numprocs, myrank, r, bloqtam, i, j, k;
    double *a, *b, *c, *ATMP, *a_original;
    int fila, columna;
    int arriba, abajo, izquierda, derecha;
    MPI_Status status[2];
    MPI_Request request[2];
    
    // Inicializar MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    // El proceso 0 obtiene el tamaño de los bloques desde la entrada estándar
    if (myrank == 0) {
        printf("Ingrese el tamaño de los bloques (bloqtam): ");
        fflush(stdout);
        scanf("%d", &bloqtam);
        
        // Verificar que el número de procesos sea un cuadrado perfecto
        r = (int)sqrt(numprocs);
        if (r * r != numprocs) {
            fprintf(stderr, "Error: El número de procesos debe ser un cuadrado perfecto.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        
        // Verificar restricciones
        if (r > rmax) {
            fprintf(stderr, "Error: r (%d) excede rmax (%d)\n", r, rmax);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        if (bloqtam > maxbloqtam) {
            fprintf(stderr, "Error: bloqtam (%d) excede maxbloqtam (%d)\n", bloqtam, maxbloqtam);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }
    
    // Distribución de los parámetros a todos los procesos
    MPI_Bcast(&bloqtam, 1, MPI_INT, 0, MPI_COMM_WORLD);
    r = (int)sqrt(numprocs);
    
    // Calcular la posición en la malla (fila, columna)
    fila = myrank / r;
    columna = myrank % r;
    
    // Calcular los vecinos para la rotación de bloques
    arriba = ((fila - 1 + r) % r) * r + columna;
    abajo = ((fila + 1) % r) * r + columna;
    izquierda = fila * r + ((columna - 1 + r) % r);
    derecha = fila * r + ((columna + 1) % r);
    
    // Asignar memoria para los bloques
    a = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    b = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    c = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    ATMP = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    a_original = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    
    if (a == NULL || b == NULL || c == NULL || ATMP == NULL || a_original == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        free(a);
        free(b);
        free(c);
        free(ATMP);
        free(a_original);
        return 1;
    }
    
    // Inicializar los bloques según la especificación de la práctica
    generar_matriz_original(a, bloqtam, fila, columna);
    
    // Guardar una copia de la matriz original A para verificar después
    for (i = 0; i < bloqtam * bloqtam; i++) {
        a_original[i] = a[i];
    }
    
    // B es la matriz identidad
    for (i = 0; i < bloqtam * bloqtam; i++) {
        int row = i / bloqtam;
        int col = i % bloqtam;
        b[i] = (row == col) ? 1.0 : 0.0;
    }
    
    // Inicializar c a cero - NOTA: La matriz c se inicializa a cero dentro de la función mult
    for (i = 0; i < bloqtam * bloqtam; i++) {
        c[i] = 0.0;
    }
    
    // Desplazamiento inicial de las matrices a y b para el algoritmo de Cannon
    // Desplazar a a la izquierda fila veces
    int desplazamiento = fila;
    for (i = 0; i < desplazamiento; i++) {
        // Uso de comunicaciones no bloqueantes para evitar interbloqueos
        MPI_Isend(a, bloqtam * bloqtam, MPI_DOUBLE, izquierda, 100 + i, MPI_COMM_WORLD, &request[0]);
        MPI_Irecv(ATMP, bloqtam * bloqtam, MPI_DOUBLE, derecha, 100 + i, MPI_COMM_WORLD, &request[1]);
        
        // Esperar a que se completen ambas operaciones con MPI_Wait individuales
        MPI_Wait(&request[0], &status[0]);
        MPI_Wait(&request[1], &status[1]);
        
        // Intercambiar a y ATMP
        double *temp = a;
        a = ATMP;
        ATMP = temp;
    }
    
    // Barrera para asegurar que todos los procesos han terminado el desplazamiento de a
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Desplazar b hacia arriba columna veces
    desplazamiento = columna;
    for (i = 0; i < desplazamiento; i++) {
        // Uso de comunicaciones no bloqueantes para evitar interbloqueos
        MPI_Isend(b, bloqtam * bloqtam, MPI_DOUBLE, arriba, 200 + i, MPI_COMM_WORLD, &request[0]);
        MPI_Irecv(ATMP, bloqtam * bloqtam, MPI_DOUBLE, abajo, 200 + i, MPI_COMM_WORLD, &request[1]);
        
        // Esperar a que se completen ambas operaciones con MPI_Wait individuales
        MPI_Wait(&request[0], &status[0]);
        MPI_Wait(&request[1], &status[1]);
        
        // Intercambiar b y ATMP
        double *temp = b;
        b = ATMP;
        ATMP = temp;
    }
    
    // Barrera para asegurar que todos los procesos han terminado el desplazamiento de b
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Algoritmo de Cannon
    for (k = 0; k < r; k++) {
        // Multiplicar los bloques a y b, acumulando en c
        mult(a, b, c, bloqtam);
        
        // Desplazar a a la izquierda una posición usando comunicaciones no bloqueantes
        MPI_Isend(a, bloqtam * bloqtam, MPI_DOUBLE, izquierda, 300 + k, MPI_COMM_WORLD, &request[0]);
        MPI_Irecv(ATMP, bloqtam * bloqtam, MPI_DOUBLE, derecha, 300 + k, MPI_COMM_WORLD, &request[1]);
        
        // Esperar a que se completen ambas operaciones con MPI_Wait individuales
        MPI_Wait(&request[0], &status[0]);
        MPI_Wait(&request[1], &status[1]);
        
        // Intercambiar a y ATMP
        double *temp = a;
        a = ATMP;
        ATMP = temp;
        
        // Barrera para sincronizar antes del desplazamiento de b
        MPI_Barrier(MPI_COMM_WORLD);
        
        // Desplazar b hacia arriba una posición usando comunicaciones no bloqueantes
        MPI_Isend(b, bloqtam * bloqtam, MPI_DOUBLE, arriba, 400 + k, MPI_COMM_WORLD, &request[0]);
        MPI_Irecv(ATMP, bloqtam * bloqtam, MPI_DOUBLE, abajo, 400 + k, MPI_COMM_WORLD, &request[1]);
        
        // Esperar a que se completen ambas operaciones con MPI_Wait individuales
        MPI_Wait(&request[0], &status[0]);
        MPI_Wait(&request[1], &status[1]);
        
        // Intercambiar b y ATMP
        temp = b;
        b = ATMP;
        ATMP = temp;
        
        // Barrera para sincronizar antes de la siguiente iteración
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    // Verificar los resultados
    int numerror = 0;
    for (i = 0; i < bloqtam * bloqtam; i++) {
        if (fabs(a_original[i] - c[i]) > 0.0000001) {
            numerror++;
            // Opcional: mostrar los valores que causan error para debugging
            if (numerror < 10) { // Limitar a 10 errores para no saturar la salida
                printf("Proceso %d: Error en posición %d: esperado=%f, obtenido=%f\n", 
                       myrank, i, a_original[i], c[i]);
            }
        }
    }
    
    // Recopilar y mostrar los errores
    if (myrank == 0) {
        printf("Proceso: %d. Numero de errores: %d\n", myrank, numerror);
        
        int otros_errores;
        for (i = 1; i < numprocs; i++) {
            MPI_Recv(&otros_errores, 1, MPI_INT, i, 500, MPI_COMM_WORLD, &status[0]);
            printf("Proceso: %d. Numero de errores: %d\n", i, otros_errores);
        }
    } else {
        MPI_Send(&numerror, 1, MPI_INT, 0, 500, MPI_COMM_WORLD);
    }
    
    // Liberar memoria
    free(a);
    free(b);
    free(c);
    free(ATMP);
    free(a_original);
    
    MPI_Finalize();
    return 0;
}
