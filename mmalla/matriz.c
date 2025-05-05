#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define MAXBLOQTAM 100
#define RMAX 4

void mult(double a[], double b[], double *c, int m) {
    int i, j, k;
    for (i = 0; i < m; i++) {
        for (j = 0; j < m; j++) {
            for (k = 0; k < m; k++) {
                c[i * m + j] += a[i * m + k] * b[k * m + j];
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int myrank, nproc;
    int r;          // Dimensión de la malla de procesos
    int bloqtam;    // Tamaño del bloque de la matriz
    int fila, columna; // Coordenadas del proceso en la malla
    int arriba, abajo;   // Rangos de los procesos vecinos arriba y abajo
    int izquierda, derecha; // Rangos de los procesos vecinos izquierda y derecha
    int *mifila;     // Vector con los rangos de los procesos en la misma fila
    int numerror = 0; // Contador de errores en la comparación de resultados
    int i, etapa;     // Variables de bucle
    double *a, *b, *c, *atmp; // Punteros a los bloques de las matrices
    MPI_Status status;    // Estructura para información de estado de las comunicaciones
    int *errores = NULL;  // Puntero al vector de errores de todos los procesos (en el proceso 0)

    // Inicializar el entorno MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    // El proceso 0 obtiene los parámetros de ejecución
    if (myrank == 0) {
        printf("Introducir el tamaño de bloque (max %d): ", MAXBLOQTAM);
        scanf("%d", &bloqtam);

        if (bloqtam > MAXBLOQTAM) {
            printf("Error: El tamaño de bloque debe ser <= %d\n", MAXBLOQTAM);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        r = (int)sqrt(nproc);
        if (r * r != nproc) {
            printf("Error: El número de procesos debe ser un cuadrado perfecto\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (r > RMAX) {
            printf("Error: La dimensión de la malla debe ser <= %d\n", RMAX);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Ejecutando con %d procesos en una malla %dx%d\n", nproc, r, r);
        printf("Tamaño de bloque: %d\n", bloqtam);
    }

    // Distribuir los parámetros a todos los procesos
    MPI_Bcast(&bloqtam, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&r, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular las coordenadas del proceso en la malla
    fila = myrank / r;
    columna = myrank % r;

    // Calcular los rangos de los procesos vecinos
    arriba = ((fila - 1 + r) % r) * r + columna;
    abajo = ((fila + 1) % r) * r + columna;
    izquierda = fila * r + ((columna - 1 + r) % r);
    derecha = fila * r + ((columna + 1) % r);

    // Reservar memoria para el vector mifila
    mifila = (int *)malloc(r * sizeof(int));
    if (mifila == NULL) {
        printf("Error: No se pudo reservar memoria para mifila en el proceso %d\n", myrank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Llenar el vector mifila con los rangos de los procesos en la misma fila
    for (i = 0; i < r; i++) {
        mifila[i] = fila * r + i;
    }

    // Reservar memoria para los bloques de las matrices
    a = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    b = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    c = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    atmp = (double *)malloc(bloqtam * bloqtam * sizeof(double));

    if (a == NULL || b == NULL || c == NULL || atmp == NULL) {
        printf("Error: No se pudo reservar memoria en el proceso %d\n", myrank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Inicializar el bloque de la matriz a
    for (i = 0; i < bloqtam * bloqtam; i++) {
        a[i] = (i + 1) * (double)(fila * columna + 1) * (fila * columna + 1) / (bloqtam * bloqtam);
    }

    // Inicializar el bloque de la matriz b (matriz identidad por bloques)
    for (i = 0; i < bloqtam * bloqtam; i++) {
        int ii = i / bloqtam;
        int jj = i % bloqtam;
        if (fila == columna && ii == jj) {
            b[i] = 1.0;
        } else {
            b[i] = 0.0;
        }
    }

    // Inicializar el bloque de la matriz c
    for (i = 0; i < bloqtam * bloqtam; i++) {
        c[i] = 0.0;
    }

    // Copiar el bloque de la matriz a a atmp para la comprobación final
    for (i = 0; i < bloqtam * bloqtam; i++) {
        atmp[i] = a[i];
    }

    // Desplazamiento inicial de los bloques de las matrices a y b
    int despl_fila = fila % r;
    int origen_a = mifila[(columna + despl_fila) % r];
    int destino_a = mifila[(columna - despl_fila + r) % r];

    MPI_Send(a, bloqtam * bloqtam, MPI_DOUBLE, destino_a, 100, MPI_COMM_WORLD);
    MPI_Recv(a, bloqtam * bloqtam, MPI_DOUBLE, origen_a, 100, MPI_COMM_WORLD, &status);


    int despl_col = columna % r;
    int origen_b = ((fila + despl_col) % r) * r + columna;
    int destino_b = ((fila - despl_col + r) % r) * r + columna;

    MPI_Send(b, bloqtam * bloqtam, MPI_DOUBLE, destino_b, 200, MPI_COMM_WORLD);
    MPI_Recv(b, bloqtam * bloqtam, MPI_DOUBLE, origen_b, 200, MPI_COMM_WORLD, &status);

    // Algoritmo de Cannon
    for (etapa = 0; etapa < r; etapa++) {
        mult(a, b, c, bloqtam);

        // Etiquetas de mensaje diferentes en cada iteración
        int tag_a_send = 300 + etapa;
        int tag_a_recv = 400 + etapa;
        int tag_b_send = 500 + etapa;
        int tag_b_recv = 600 + etapa;

        MPI_Send(a, bloqtam * bloqtam, MPI_DOUBLE, izquierda, tag_a_send, MPI_COMM_WORLD);
        MPI_Recv(a, bloqtam * bloqtam, MPI_DOUBLE, derecha, tag_a_recv, MPI_COMM_WORLD, &status);

        MPI_Send(b, bloqtam * bloqtam, MPI_DOUBLE, arriba, tag_b_send, MPI_COMM_WORLD);
        MPI_Recv(b, bloqtam * bloqtam, MPI_DOUBLE, abajo, tag_b_recv, MPI_COMM_WORLD, &status);
    }

    // Comprobar los resultados
    numerror = 0;
    for (i = 0; i < bloqtam * bloqtam; i++) {
        if (fabs(atmp[i] - c[i]) > 0.0000001) {
            numerror++;
        }
    }

    // Recolectar el número de errores de todos los procesos en el proceso 0
    if (myrank == 0) {
        errores = (int *)malloc(nproc * sizeof(int));
        if (errores == NULL) {
             printf("Error: No se pudo reservar memoria para errores en el proceso 0\n");
             MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    MPI_Gather(&numerror, 1, MPI_INT, errores, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // El proceso 0 imprime el número de errores de cada proceso
    if (myrank == 0) {
        for (i = 0; i < nproc; i++) {
            printf("Proceso: %d. Numero de errores: %d\n", i, errores[i]);
        }
        free(errores);
    }

    // Liberar la memoria reservada
    free(a);
    free(b);
    free(c);
    free(atmp);
    free(mifila);

    // Finalizar el entorno MPI
    MPI_Finalize();
    return 0;
}

