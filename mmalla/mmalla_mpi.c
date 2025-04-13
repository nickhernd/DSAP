#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define MAXBLOQTAM 100
#define RMAX 4

void mult(double a[], double b[], double *c, int m) {
    int i, j, k;
    for (i = 0; i < m; i++)
        for (j = 0; j < m; j++)
            for (k = 0; k < m; k++)
                c[i*m+j] = c[i*m+j] + a[i*m+k] * b[k*m+j];
    return;
}

int main(int argc, char *argv[]) {
    int myrank, nproc;
    int r;
    int bloqtam;
    int fila, columna;
    int arriba, abajo;
    int izquierda, derecha;
    int *mifila;
    int numerror = 0;
    int i, etapa;
    double *a, *b, *c, *atmp;
    MPI_Status status;
    int *errores = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

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

    MPI_Bcast(&bloqtam, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&r, 1, MPI_INT, 0, MPI_COMM_WORLD);

    fila = myrank / r;
    columna = myrank % r;

    arriba = ((fila - 1 + r) % r) * r + columna;
    abajo = ((fila + 1) % r) * r + columna;
    izquierda = fila * r + ((columna - 1 + r) % r);
    derecha = fila * r + ((columna + 1) % r);

    mifila = (int *)malloc(r * sizeof(int));
    for (i = 0; i < r; i++) {
        mifila[i] = fila * r + i;
    }

    a = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    b = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    c = (double *)malloc(bloqtam * bloqtam * sizeof(double));
    atmp = (double *)malloc(bloqtam * bloqtam * sizeof(double));

    if (a == NULL || b == NULL || c == NULL || atmp == NULL) {
        printf("Error: No se pudo reservar memoria\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i < bloqtam * bloqtam; i++) {
        a[i] = (i + 1.0) * (double)(fila * columna + 1) * (fila * columna + 1) / (bloqtam * bloqtam);
    }

    for (i = 0; i < bloqtam * bloqtam; i++) {
        int ii = i / bloqtam;
        int jj = i % bloqtam;
        if (fila == columna && ii == jj) {
            b[i] = 1.0;
        } else {
            b[i] = 0.0;
        }
    }

    for (i = 0; i < bloqtam * bloqtam; i++) {
        c[i] = 0.0;
    }

    for (i = 0; i < bloqtam * bloqtam; i++) {
        atmp[i] = a[i];
    }

    int despl_fila = fila % r;
    int origen = mifila[(columna + despl_fila) % r];
    int destino = mifila[(columna - despl_fila + r) % r];
    
    MPI_Sendrecv_replace(a, bloqtam * bloqtam, MPI_DOUBLE, 
                          destino, 0, origen, 0, 
                          MPI_COMM_WORLD, &status);

    int despl_col = columna % r;
    origen = ((fila + despl_col) % r) * r + columna;
    destino = ((fila - despl_col + r) % r) * r + columna;
    
    MPI_Sendrecv_replace(b, bloqtam * bloqtam, MPI_DOUBLE, 
                          destino, 1, origen, 1, 
                          MPI_COMM_WORLD, &status);

    for (etapa = 0; etapa < r; etapa++) {
        mult(a, b, c, bloqtam);
        
        MPI_Sendrecv_replace(a, bloqtam * bloqtam, MPI_DOUBLE, 
                             izquierda, 2 + etapa, derecha, 2 + etapa, 
                             MPI_COMM_WORLD, &status);
        
        MPI_Sendrecv_replace(b, bloqtam * bloqtam, MPI_DOUBLE, 
                             arriba, 2 + r + etapa, abajo, 2 + r + etapa, 
                             MPI_COMM_WORLD, &status);
    }

    numerror = 0;
    for (i = 0; i < bloqtam * bloqtam; i++) {
        if (fabs(atmp[i] - c[i]) > 0.0000001) {
            numerror++;
        }
    }

    if (myrank == 0) {
        errores = (int *)malloc(nproc * sizeof(int));
    }

    MPI_Gather(&numerror, 1, MPI_INT, errores, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (myrank == 0) {
        for (i = 0; i < nproc; i++) {
            printf("Proceso: %d. Numero de errores: %d\n", i, errores[i]);
        }
        free(errores);
    }

    free(a);
    free(b);
    free(c);
    free(atmp);
    free(mifila);

    MPI_Finalize();
    return 0;
}