// Autor : Jaime Hernández Delgado

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

// Estructura para almacenar una imagen en formato PGM
typedef struct
{
    int row;      // número de filas en la imagen
    int col;      // número de columnas en la imagen
    int max_gray; // máximo valor de gris
    int **matrix; // matriz de píxeles entre 0 y 255
} PGMData;

// Declaraciones de funciones (asumidas en aux.c)
int **CrearArray2D_int(int, int);
void LiberarArray2D_int(int, double **);
void readPGM(char *, PGMData *);
void writePGM(char *, PGMData *);
void Filtro_Laplace(int **, int **, int, int);
void crea_pgm(int, int, int, int **, PGMData *);

int main(int argc, char **argv)
{
    int rank, size;
    double start_time, end_time;

    // Inicializar MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Declaración de variables
    char archivo_imagen_ori[100] = "logo.pgm";
    char archivo_imagen_aristas[100] = "logo_edge_paralelo.pgm"; // Cambiado el nombre del archivo de salida para la versión paralela
    PGMData img_data, img_edge;
    int row, col, max_gray;
    int row_local, row_start;
    int **local_matrix, **local_matrix_edge;
    MPI_Status status;

    // Manejo de argumentos de línea de comandos
    if (argc > 1)
    {
        strcpy(archivo_imagen_ori, argv[1]);
        strcat(archivo_imagen_ori, ".pgm");
        strcpy(archivo_imagen_aristas, argv[1]);
        strcat(archivo_imagen_aristas, "_edge_paralelo.pgm"); // Añadido sufijo "_paralelo"
    }

    // Proceso 0: Leer la imagen original
    if (rank == 0)
    {
        printf("\n *************** DATOS DE LA EJECUCION ***************************\n");
        printf(" * Archivo imagen original       : %25s           *\n", archivo_imagen_ori);
        printf(" * Archivo imagen con aristas (paralelo): %25s           *\n", archivo_imagen_aristas); // Indicando que es la versión paralela
        printf(" *****************************************************************\n\n");
        printf(" Leyendo imagen \"%s\" ... \n", archivo_imagen_ori);

        readPGM(archivo_imagen_ori, &img_data);
        printf(" Dimension de la imagen: %d x %d\n", img_data.row, img_data.col);

        // Guardar dimensiones y max_gray para enviar a otros procesos
        row = img_data.row;
        col = img_data.col;
        max_gray = img_data.max_gray;
    }

    // Broadcast de las dimensiones de la imagen y el valor máximo de gris
    MPI_Bcast(&row, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&col, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_gray, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular el número de filas para cada proceso
    row_local = row / size;
    if (rank == 0)
    {
        row_local += row % size; // El proceso 0 toma el resto de las filas
    }

    // Calcular la fila de inicio para cada proceso
    row_start = 0;
    for (int i = 0; i < rank; i++)
    {
        row_start += row / size;
        if (i == 0)
            row_start += row % size; // Sumar el resto solo una vez al calcular los desplazamientos
    }

    // Alocar memoria para las matrices locales
    // Cada proceso almacena sus filas locales + 2 filas extra (superior e inferior) para el cálculo de Laplace
    // El primer y último proceso solo necesitan una fila extra.
    if (rank == 0) {
        local_matrix = CrearArray2D_int(row_local + 1, col); // +1 para la fila inferior
        local_matrix_edge = CrearArray2D_int(row_local, col);
    }
    else if (rank == size - 1){
        local_matrix = CrearArray2D_int(row_local + 1, col); // +1 para la fila superior
        local_matrix_edge = CrearArray2D_int(row_local, col);
    }
    else {
        local_matrix = CrearArray2D_int(row_local + 2, col);
        local_matrix_edge = CrearArray2D_int(row_local, col);
    }

    // Distribuir los datos de la imagen original
    if (rank == 0)
    {
        // El proceso 0 envía porciones de la matriz original a cada proceso
        for (int i = 1; i < size; i++)
        {
            int dest_row_local = row / size;
            if (i == 0)
                dest_row_local += row % size;
            int dest_row_start = 0;
             for (int j = 0; j < i; j++)
            {
                dest_row_start += row / size;
                 if (j == 0)
                    dest_row_start += row % size;
            }
            MPI_Send(&img_data.matrix[dest_row_start][0], dest_row_local * col, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // El proceso 0 copia su porción de la matriz original a local_matrix
        for (int i = 0; i < row_local; i++)
        {
            for (int j = 0; j < col; j++)
            {
                local_matrix[i][j] = img_data.matrix[i][j];
            }
        }
    }
    else
    {
        // Los procesos no-cero reciben su porción de la matriz original
        MPI_Recv(&local_matrix[1][0], row_local * col, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }

     // Enviar la última fila de cada proceso a su vecino inferior, y la primera fila a su vecino superior
    if (size > 1) { // Para evitar problemas si solo hay un proceso
        if (rank != size - 1)
            MPI_Send(&local_matrix[row_local - 1][0], col, MPI_INT, rank + 1, 1, MPI_COMM_WORLD); // Enviar la última fila
        if (rank != 0)
            MPI_Recv(&local_matrix[0][0], col, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &status);     // Recibir la fila superior
        if (rank != 0)
            MPI_Send(&local_matrix[1][0], col, MPI_INT, rank - 1, 2, MPI_COMM_WORLD);         // Enviar la primera fila
        if (rank != size - 1)
           MPI_Recv(&local_matrix[row_local + (rank != size -1)][0], col, MPI_INT, rank + 1, 2, MPI_COMM_WORLD, &status); // Recibir la fila inferior
    }

    // Aplicar el filtro de Laplace en paralelo
    printf(" Proceso %d aplicando el filtro de Laplace...\n", rank);
    start_time = MPI_Wtime();
    if (rank == 0)
    {
        Filtro_Laplace(local_matrix, local_matrix_edge, row_local, col); // Procesa su porción
    }
    else
    {
       Filtro_Laplace(local_matrix, local_matrix_edge, row_local, col);
    }
     end_time = MPI_Wtime();
     printf(" Proceso %d: Tiempo de cálculo del filtro = %f segundos\n", rank, end_time - start_time);

    // Recoger los resultados en el proceso 0
    if (rank == 0)
    {
        img_edge.row = row;
        img_edge.col = col;
        img_edge.max_gray = max_gray;
        img_edge.matrix = CrearArray2D_int(row, col);

        // Copiar la porción del proceso 0
        for (int i = 0; i < row_local; i++)
        {
            for (int j = 0; j < col; j++)
            {
                img_edge.matrix[i][j] = local_matrix_edge[i][j];
            }
        }

        // Recibir los resultados de los otros procesos
        for (int i = 1; i < size; i++)
        {
            int src_row_local = row / size;
            if (i == 0)
                src_row_local += row % size;
            int src_row_start = 0;
            for (int j = 0; j < i; j++)
            {
                 src_row_start += row / size;
                 if (j == 0)
                    src_row_start += row % size;
            }
            MPI_Recv(&img_edge.matrix[src_row_start][0], src_row_local * col, MPI_INT, i, 3, MPI_COMM_WORLD, &status);
        }

        // Guardar la imagen resultante
        printf(" Proceso 0 guardando la imagen con la detección de aristas en \"%s\"\n\n", archivo_imagen_aristas);
        writePGM(archivo_imagen_aristas, &img_edge);
    }
    else
    {
        // Los procesos no-cero envían sus resultados al proceso 0
        MPI_Send(&local_matrix_edge[0][0], row_local * col, MPI_INT, 0, 3, MPI_COMM_WORLD);
    }

    // Liberar la memoria asignada
    if (rank == 0) {
        LiberarArray2D_int(img_data.row, img_data.matrix);
        LiberarArray2D_int(img_edge.row, img_edge.matrix);
    }
    LiberarArray2D_int(row_local + (rank == 0 ? 1 : (rank == size -1 ? 1: 2)), local_matrix);
    LiberarArray2D_int(row_local, local_matrix_edge);

    // Finalizar MPI
    MPI_Finalize();
    return 0;
}

