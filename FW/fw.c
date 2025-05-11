#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <limits.h>

#define maxn 1000 // Número máximo de vértices
#define maxnumprocs 8 // Número máximo de procesos

// Declaración de funciones
float **Crear_matriz_pesos_consecutivo(int, int);
int **Crear_matriz_caminos_consecutivo(int, int);
void Definir_Grafo(int, float **, int **);
void calcula_camino(float **, int **, int);
void printMatrizPesos(float **, int, int);
void printMatrizCaminos(int **, int, int);
int find_sender(int k, int n, int numprocs);

int main(int argc, char *argv[]) {
    int myrank, numprocs, n = 10, i, j, k, nfilas, resto, nlocal;
    float **dist, **local_dist, *aux;
    int **caminos, **local_caminos, *auxC;

    MPI_Init(&argc, &argv); // Inicializar MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); // Obtener el rango del proceso
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs); // Obtener el número de procesos

    if (argc > 1) sscanf(argv[1], "%d", &n); // Leer n desde la línea de comandos

    if (n > maxn) {
        if (myrank == 0) printf("Error: n excede maxn\n");
        MPI_Finalize();
        return 1;
    }

    // Calcular cómo distribuir las filas
    nfilas = n / numprocs; // Número de filas por proceso
    resto = n % numprocs; // Filas restantes
    nlocal = (myrank == 0) ? nfilas + resto : nfilas;  // Filas extra para el proceso 0

    // Asignar memoria
    if (myrank == 0) {
        dist = Crear_matriz_pesos_consecutivo(n, n); // Asignar matrices completas en el root
        caminos = Crear_matriz_caminos_consecutivo(n, n);
        Definir_Grafo(n, dist, caminos); // Inicializar el grafo en el root
    }
    local_dist = Crear_matriz_pesos_consecutivo(nlocal, n); // Asignar matrices locales
    local_caminos = Crear_matriz_caminos_consecutivo(nlocal, n);
    aux = (float *)malloc(n * sizeof(float)); // Asignar arrays auxiliares
    auxC = (int *)malloc(n * sizeof(int));

    // Distribuir los datos
    MPI_Scatter(dist ? *dist : NULL, nlocal * n, MPI_FLOAT,
                *local_dist, nlocal * n, MPI_FLOAT,
                0, MPI_COMM_WORLD);
    MPI_Scatter(caminos ? *caminos : NULL, nlocal * n, MPI_INT,
                *local_caminos, nlocal * n, MPI_INT,
                0, MPI_COMM_WORLD);

    // Algoritmo de Floyd-Warshall
    for (k = 0; k < n; k++) {
        int sender = find_sender(k, n, numprocs); // Encontrar el proceso que contiene la fila k

        if (myrank == sender) {
            // Copiar la fila k en aux y auxC
            for (j = 0; j < n; j++) {
                aux[j] = local_dist[(k - sender * nfilas - (sender < resto ? sender : resto))][j];
                auxC[j] = local_caminos[(k - sender * nfilas - (sender < resto ? sender : resto))][j];
            }
        }

        MPI_Bcast(aux, n, MPI_FLOAT, sender, MPI_COMM_WORLD); // Difundir la fila k
        MPI_Bcast(auxC, n, MPI_INT, sender, MPI_COMM_WORLD);

        for (i = 0; i < nlocal; i++) { // Actualizar matrices locales
            for (j = 0; j < n; j++) {
                if ((local_dist[i][k] != 0) && (aux[j] != 0)) {
                    if ((local_dist[i][k] + aux[j] < local_dist[i][j]) || (local_dist[i][j] == 0)) {
                        local_dist[i][j] = local_dist[i][k] + aux[j];
                        local_caminos[i][j] = auxC[j];
                    }
                }
            }
        }
    }

    // Recolectar los resultados
    MPI_Gather(*local_dist, nlocal * n, MPI_FLOAT,
               dist ? *dist : NULL, nlocal * n, MPI_FLOAT,
               0, MPI_COMM_WORLD);
    MPI_Gather(*local_caminos, nlocal * n, MPI_INT,
               caminos ? *caminos : NULL, nlocal * n, MPI_INT,
               0, MPI_COMM_WORLD);

    if (myrank == 0) {
        if (n <= 10) { // Imprimir matrices si n <= 10
            printMatrizPesos(dist, n, n);
            printMatrizCaminos(caminos, n, n);
        }
        calcula_camino(dist, caminos, n); // Calcular e imprimir los caminos más cortos

        // Liberar memoria
        free(*dist);
        free(dist);
        free(*caminos);
        free(caminos);
    }

    free(*local_dist);
    free(local_dist);
    free(aux);
    free(auxC);
    MPI_Finalize(); // Finalizar MPI
    return 0;
}

// Funciones auxiliares
float **Crear_matriz_pesos_consecutivo(int Filas, int Columnas) {
    // Crea un array de 2 dimensiones en posiciones contiguas de memoria
    float *mem_matriz;
    float **matriz;
    int fila, col;
    if (Filas <= 0) {
        printf("El número de filas debe ser mayor que cero\n");
        return NULL;
    }
    if (Columnas <= 0) {
        printf("El número de columnas debe ser mayor que cero\n");
        return NULL;
    }
    mem_matriz = malloc(Filas * Columnas * sizeof(float));
    if (mem_matriz == NULL) {
        printf("Espacio de memoria insuficiente\n");
        return NULL;
    }
    matriz = malloc(Filas * sizeof(float *));
    if (matriz == NULL) {
        printf("Espacio de memoria insuficiente\n");
        return NULL;
    }
    for (fila = 0; fila < Filas; fila++)
        matriz[fila] = mem_matriz + (fila * Columnas);
    return matriz;
}

int **Crear_matriz_caminos_consecutivo(int Filas, int Columnas) {
    // Crea un array de 2 dimensiones en posiciones contiguas de memoria
    int *mem_matriz;
    int **matriz;
    int fila, col;
    if (Filas <= 0) {
        printf("El número de filas debe ser mayor que cero\n");
        return NULL;
    }
    if (Columnas <= 0) {
        printf("El número de columnas debe ser mayor que cero\n");
        return NULL;
    }
    mem_matriz = malloc(Filas * Columnas * sizeof(int));
    if (mem_matriz == NULL) {
        printf("Espacio de memoria insuficiente\n");
        return NULL;
    }
    matriz = malloc(Filas * sizeof(int *));
    if (matriz == NULL) {
        printf("Espacio de memoria insuficiente\n");
        return NULL;
    }
    for (fila = 0; fila < Filas; fila++)
        matriz[fila] = mem_matriz + (fila * Columnas);
    return matriz;
}

void Definir_Grafo(int n, float **dist, int **caminos) {
    // Inicializa la matriz de pesos y la de caminos para el algoritmo de Floyd-Warshall.
    // Un 0 en la matriz de pesos indica que no hay arco.
    // Para la matriz de caminos se supone que los vértices se numeran de 1 a n.
    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            if (i == j)
                dist[i][j] = 0;
            else {
                dist[i][j] = (11.0 * rand() / (RAND_MAX + 1.0)); // Aleatorios 0 <= dist < 11
                dist[i][j] = ((double) ((int) (dist[i][j] * 10))) / 10; // Truncamos a 1 decimal
                if (dist[i][j] < 2) dist[i][j] = 0; // Establecemos algunos a 0
            }
            if (dist[i][j] != 0)
                caminos[i][j] = i + 1;
            else
                caminos[i][j] = 0;
        }
    }
}

void calcula_camino(float **a, int **b, int n) {
    int i, count = 2, count2;
    int anterior;
    int *camino;
    int inicio = -1, fin = -1;
    int ch;

    while ((inicio < 0) || (inicio > n) || (fin < 0) || (fin > n)) {
        printf("Vértices inicio y final: (0 0 para salir) ");
        scanf("%d %d", &inicio, &fin);
        while ((ch = getchar()) != '\n' && ch != EOF);
    }

    while ((inicio != 0) && (fin != 0)) {
        anterior = fin;
        while (b[inicio - 1][anterior - 1] != inicio) {
            anterior = b[inicio - 1][anterior - 1];
            count = count + 1;
        }
        count2 = count;
        camino = malloc(count * sizeof(int));
        anterior = fin;
        camino[count - 1] = fin;
        while (b[inicio - 1][anterior - 1] != inicio) {
            anterior = b[inicio - 1][anterior - 1];
            count = count - 1;
            camino[count - 1] = anterior;
        }
        camino[0] = inicio;
        printf("\nCamino más corto de %d a %d:\n", inicio, fin);
        printf("          Peso: %5.1f\n", a[inicio - 1][fin - 1]);
        printf("        Camino: ");
        for (i = 0; i < count2; i++) printf("%d ", camino[i]);
        printf("\n");
        free(camino);
        inicio = -1;
        while ((inicio < 0) || (inicio > n) || (fin < 0) || (fin > n)) {
            printf("Vértices inicio y final: (0 0 para salir) ");
            scanf("%d %d", &inicio, &fin);
            while ((ch = getchar()) != '\n' && ch != EOF);
        }

    }
}

void printMatrizCaminos(int **a, int fila, int col) {
    int i, j;
    char buffer[10];
    printf("     ");
    for (i = 0; i < col; ++i) {
        j = sprintf(buffer, "%c%d", 'V', i + 1);
        printf("%5s", buffer);
    }
    printf("\n");
    for (i = 0; i < fila; ++i) {
        j = sprintf(buffer, "%c%d", 'V', i + 1);
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
    printf("     ");
    for (i = 0; i < col; ++i) {
        j = sprintf(buffer, "%c%d", 'V', i + 1);
        printf("%5s", buffer);
    }
    printf("\n");
    for (i = 0; i < fila; ++i) {
        j = sprintf(buffer, "%c%d", 'V', i + 1);
        printf("%5s", buffer);
        for (j = 0; j < col; ++j)
            printf("%5.1f", a[i][j]);
        printf("\n");
    }
    printf("\n");
}

// Función para encontrar el proceso que contiene la fila k
int find_sender(int k, int n, int numprocs) {
    int nfilas = n / numprocs;
    int resto = n % numprocs;
    for (int i = 0; i < numprocs; i++) {
        if (k < resto + nfilas * (i + 1)) {
            return i;
        }
    }
    return numprocs - 1; // No debería llegar aquí
}
