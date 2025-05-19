// Autor: Jaime Hernández Delgado

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int KMAXNPROCS = 8;    
const int KMAXN = 1000; 
const float INFINITY_FLOAT = 999999.0f;

// Codigo copiado del ejemplo del profesor.
// Crea un array bidimensional asegurandose de que las posiciones
// reservadas en memoria son contiguas. Tipo INT.
int **Crear_matriz_caminos_consecutivo(int Filas, int Columnas) {
    // crea un array de 2 dimensiones en posiciones contiguas de memoria
    int *mem_matriz;
    int **matriz;
    int fila, col;
    if (Filas <= 0) {
        printf("El numero de filas debe ser mayor que cero\n");
        return NULL;
    }
    if (Columnas <= 0) {
        printf("El numero de columnas debe ser mayor que cero\n");
        return NULL;
    }
    mem_matriz = (int *)malloc(Filas * Columnas * sizeof(int));
    if (mem_matriz == NULL) {
        printf("Insuficiente espacio de memoria\n");
        return NULL;
    }
    matriz = (int **)malloc(Filas * sizeof(int *));
    if (matriz == NULL) {
        printf("Insuficiente espacio de memoria\n");
        free(mem_matriz);
        return NULL;
    }
    for (fila = 0; fila < Filas; fila++)
        matriz[fila] = mem_matriz + (fila * Columnas);

    return matriz;
}

// Codigo copiado del ejemplo
// Crea un array bidimensional asegurandose de que las posiciones
// reservadas en memoria son contiguas. Tipo FLOAT.
float **Crear_matriz_pesos_consecutivo(int Filas, int Columnas) {
    // crea un array de 2 dimensiones en posiciones contiguas de memoria
    float *mem_matriz;
    float **matriz;
    int fila, col;
    if (Filas <= 0) {
        printf("El numero de filas debe ser mayor que cero\n");
        return NULL;
    }
    if (Columnas <= 0) {
        printf("El numero de columnas debe ser mayor que cero\n");
        return NULL;
    }
    mem_matriz = (float *)malloc(Filas * Columnas * sizeof(float));
    if (mem_matriz == NULL) {
        printf("Insuficiente espacio de memoria\n");
        return NULL;
    }
    matriz = (float **)malloc(Filas * sizeof(float *));
    if (matriz == NULL) {
        printf("Insuficiente espacio de memoria\n");
        free(mem_matriz);
        return NULL;
    }
    for (fila = 0; fila < Filas; fila++)
        matriz[fila] = mem_matriz + (fila * Columnas);

    return matriz;
}

// Codigo copiado del ejemplo
void Definir_Grafo(int n, float **dist, int **caminos) {
    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            if (i == j)
                dist[i][j] = 0;
            else {
                dist[i][j] = (11.0 * rand() / (RAND_MAX + 1.0)); // aleatorios 0 <= dist < 11
                dist[i][j] = ((double)((int)(dist[i][j] * 10))) / 10; // truncamos a 1 decimal
                if (dist[i][j] < 2) dist[i][j] = INFINITY_FLOAT; // establecemos algunos a infinito
            }
            if (dist[i][j] != INFINITY_FLOAT && i != j)
                caminos[i][j] = i + 1;
            else
                caminos[i][j] = 0;
        }
    }
}

void printMatrizCaminos(int **a, int fila, int col) {
    int i, j;
    char buffer[10];
    printf("      ");
    for (i = 0; i < col; ++i) {
        j = sprintf(buffer, "V%d", i + 1);
        printf("%5s", buffer);
    }
    printf("\n");
    for (i = 0; i < fila; ++i) {
        j = sprintf(buffer, "V%d", i + 1);
        printf("%5s", buffer);
        for (j = 0; j < col; ++j)
            printf("%5d", a[i][j]);
        printf("\n");
    }
    printf("\n");
}

void printMatrizPesos(float **a, int fila, int col) {
    int i, j;
    char buffer[10];
    printf("      ");
    for (i = 0; i < col; ++i) {
        j = sprintf(buffer, "V%d", i + 1);
        printf("%6s", buffer);
    }
    printf("\n");
    for (i = 0; i < fila; ++i) {
        j = sprintf(buffer, "V%d", i + 1);
        printf("%6s", buffer);
        for (j = 0; j < col; ++j) {
            if (a[i][j] == INFINITY_FLOAT)
                printf("   INF");
            else
                printf("%6.1f", a[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

/* Desasignamos un puntero a un array 2d*/
void deallocate_array_int(int **array, int row_dim) {
    if (array == NULL || row_dim <= 0) return;
    free(array[0]);
    free(array);
}

void deallocate_array_float(float **array, int row_dim) {
    if (array == NULL || row_dim <= 0) return;
    free(array[0]);
    free(array);
}

int main(int argc, char **argv) {
    MPI_Status status;
    int ierr, rank, nThreads;
    int n = 0, **caminos = NULL;
    float **dist = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nThreads);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nThreads < 2) {
        if (rank == 0)
            printf("Debe haber como minimo 2 hilos para ejecutar este programa.\n");
        MPI_Finalize();
        return 0;
    }

    if (nThreads > KMAXNPROCS) {
        if (rank == 0)
            printf("El numero de procesos (%d) sobrepasa el maximo (%d).\nSe conservarán (%d) procesos.\n", nThreads, KMAXNPROCS, KMAXNPROCS);
        nThreads = KMAXNPROCS;
    }

    // Obtencion de N
    if (rank == 0) {
        if (argc == 1) {
            printf("Introduce numero de vertices del grafo: \n");
            if (!scanf("%d", &n))
                printf("Error en introduccion de datos, puede\nque el funcionamiento no sea el deseado...\n");

            if (n > KMAXN) {
                printf("El numero de vertices (%d) sobrepasa el maximo (%d)\nNos quedamos con %d\n", n, KMAXN, KMAXN);
                n = KMAXN;
            } else if (n <= 0) {
                printf("El numero de vertices debe ser mayor que cero. Se usara el valor por defecto: 5\n");
                n = 5;
            } else
                printf("El grafo tendra %d vertices.\n", n);
        } else {
            n = atoi(argv[1]);
            if (n <= 0) {
                printf("El numero de vertices debe ser mayor que cero. Se usara el valor por defecto: 5\n");
                n = 5;
            } else if (n > KMAXN) {
                printf("El numero de vertices (%d) sobrepasa el maximo (%d)\nNos quedamos con %d\n", n, KMAXN, KMAXN);
                n = KMAXN;
            } else
                printf("Numero de vertices en el grafo: %d\n", n);
        }
    }

    // Mando el numero de vertices a los esclavos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculo porciones
    int rows_per_process = (n + nThreads - 1) / nThreads;

    // Necesario para el scatterv y gatherv
    int *sendCounts = (int *)malloc(sizeof(int) * nThreads);
    int *displs = (int *)malloc(sizeof(int) * nThreads);

    // Preparar variables donde se guardara la informacion distribuida
    int my_rows = (rank < n % nThreads) ? rows_per_process : rows_per_process - 1;
    if (my_rows < 0) my_rows = 0;
    int **auxCaminos = Crear_matriz_caminos_consecutivo(my_rows, n);
    float **auxPesos = Crear_matriz_pesos_consecutivo(my_rows, n);

    if (rank == 0) {
        dist = Crear_matriz_pesos_consecutivo(n, n);
        caminos = Crear_matriz_caminos_consecutivo(n, n);
        Definir_Grafo(n, dist, caminos);
        if (n <= 10) {
            printf("Matriz de Pesos Inicial:\n");
            printMatrizPesos(dist, n, n);
            printf("Matriz de Caminos Inicial:\n");
            printMatrizCaminos(caminos, n, n);
        }
    }

    for (int i = 0; i < nThreads; i++) {
        int start_row = i * rows_per_process;
        int num_rows = (i < n % nThreads) ? rows_per_process : rows_per_process - 1;
        if (num_rows < 0) num_rows = 0;
        sendCounts[i] = num_rows * n;
        displs[i] = start_row * n;
    }

    if (sendCounts == NULL || displs == NULL) {
        printf("Error en reserva de memoria para sendCounts o displs, hilo %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Scatterv(rank == 0 ? &caminos[0][0] : NULL, sendCounts, displs, MPI_INT,
                 &auxCaminos[0][0], my_rows * n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(rank == 0 ? &dist[0][0] : NULL, sendCounts, displs, MPI_FLOAT,
                 &auxPesos[0][0], my_rows * n, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // FW MPI
    for (int k = 0; k < n; k++) {
        int owner_rank = k / rows_per_process;
        int owner_row = k % rows_per_process;
        float *broadcast_row_pesos = NULL;
        int *broadcast_row_caminos = NULL;

        if (rank == owner_rank) {
            if (owner_row < my_rows) {
                broadcast_row_pesos = auxPesos[owner_row];
                broadcast_row_caminos = auxCaminos[owner_row];
            } else {
                // This should ideally not happen given the distribution logic
                broadcast_row_pesos = (float *)malloc(n * sizeof(float));
                broadcast_row_caminos = (int *)malloc(n * sizeof(int));
                for (int i = 0; i < n; ++i) {
                    broadcast_row_pesos[i] = INFINITY_FLOAT; // Or some other default
                    broadcast_row_caminos[i] = 0;
                }
            }
        } else {
            broadcast_row_pesos = (float *)malloc(n * sizeof(float));
            broadcast_row_caminos = (int *)malloc(n * sizeof(int));
        }

        MPI_Bcast(broadcast_row_pesos, n, MPI_FLOAT, owner_rank, MPI_COMM_WORLD);
        MPI_Bcast(broadcast_row_caminos, n, MPI_INT, owner_rank, MPI_COMM_WORLD);

        for (int i = 0; i < my_rows; i++) {
            for (int j = 0; j < n; j++) {
                if (auxPesos[i][k] != INFINITY_FLOAT && broadcast_row_pesos[j] != INFINITY_FLOAT) {
                    if (auxPesos[i][j] > auxPesos[i][k] + broadcast_row_pesos[j]) {
                        auxPesos[i][j] = auxPesos[i][k] + broadcast_row_pesos[j];
                        auxCaminos[i][j] = broadcast_row_caminos[j];
                    }
                }
            }
        }

        if (rank != owner_rank) {
            free(broadcast_row_pesos);
            free(broadcast_row_caminos);
        } else if (owner_row >= my_rows) {
            free(broadcast_row_pesos);
            free(broadcast_row_caminos);
        }
    }

    MPI_Gatherv(&auxCaminos[0][0], my_rows * n, MPI_INT,
                rank == 0 ? &caminos[0][0] : NULL, sendCounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gatherv(&auxPesos[0][0], my_rows * n, MPI_FLOAT,
                rank == 0 ? &dist[0][0] : NULL, sendCounts, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Liberacion de memoria
    if (rank == 0) {
        if (n <= 10) {
            printf("Matriz de Pesos Inicial:\n");
            printMatrizPesos(dist, n, n);
            printf("Matriz de Caminos Inicial:\n");
            printMatrizCaminos(caminos, n, n);
            printf("Matriz de Pesos Final:\n");
            printMatrizPesos(dist, n, n);
            printf("Matriz de Caminos Final:\n");
            printMatrizCaminos(caminos, n, n);
        } else {
            printf("El numero de vertices (%d) es mayor que 10, no se mostraran las matrices finales.\n", n);
        }

        deallocate_array_float(dist, n);
        deallocate_array_int(caminos, n);
    }
    deallocate_array_int(auxCaminos, my_rows);
    deallocate_array_float(auxPesos, my_rows);
    free(sendCounts);
    free(displs);

    MPI_Finalize();
    return 0;
}