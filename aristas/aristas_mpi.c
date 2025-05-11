// Autor: Jaime Hernández Delgado
// DNI : 48776654W
// mpirun -np 4 ./aristas_mpi pont
// display pont_edge_paralelo.pgm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

// Estructura que contiene lo necesario para almacenar una imagen en formato PGM
typedef struct {
    int row;  // número de filas en la imagen
    int col;  // número de columnas en la imagen
    int max_gray; // maximo valor gray
    int **matrix; // matriz de pixeles entre 0 y 255
} PGMData;

int **CrearArray2D_int(int, int);
void LiberarArray2D_int(int, double **);
void readPGM(char *, PGMData *);
void writePGM(char *, PGMData *);
void Filtro_Laplace(int **, int **, int, int);

int main(int argc, char **argv) {
    int rank, size, row, col, max_gray, rowlocal, i, j;
    PGMData img_data, img_edge;
    char archivo_imagen_ori[100] = "logo.pgm";
    char archivo_imagen_aristas[100] = "logo_edge_paralelo.pgm";

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    switch (argc) {
        case 2:
            strcpy(archivo_imagen_ori, argv[1]);
            strcat(archivo_imagen_ori, ".pgm");
            strcpy(archivo_imagen_aristas, argv[1]);
            strcat(archivo_imagen_aristas, "_edge_paralelo.pgm");
            break;
        case 1:
            break;
        default:
            if (rank == 0) {
                printf("Demasiados parametros\n");
            }
            MPI_Finalize();
            return 1; // Use a non-zero exit code for errors
    }

    // ---  Process 0: Read and Broadcast Image Data ---
    if (rank == 0) {
        printf("\n  *************** DATOS DE LA EJECUCION ***************************\n");
        printf("  * Archivo imagen original   : %25s         *\n", archivo_imagen_ori);
        printf("  * Archivo imagen con aristas: %25s         *\n", archivo_imagen_aristas);
        printf("  *****************************************************************\n\n");
        printf("  Leyendo imagen \"%s\" ... \n", archivo_imagen_ori);

        readPGM(archivo_imagen_ori, &img_data);
        row = img_data.row;
        col = img_data.col;
        max_gray = img_data.max_gray;

        printf("  Dimension de la imagen: %d x %d\n", row, col);

        img_edge.matrix = CrearArray2D_int(row, col); // Allocate for the final result
        img_edge.row = row;
        img_edge.col = col;
        img_edge.max_gray = max_gray;
    }

    MPI_Bcast(&row, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&col, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_gray, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // --- Calculate Local Rows and Offsets ---
    int *sendcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));

        int base_rows = row / size;
        int extra = row % size;
        int offset = 0;

        for (i = 0; i < size; i++) {
            sendcounts[i] = base_rows;
            if (i < extra) {
                sendcounts[i]++;
            }
            sendcounts[i] *= col; // Number of elements to send (rows * cols)
            displs[i] = offset * col;
            offset += base_rows;
            if (i < extra) {
                offset++;
            }
        }
    }

    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

    rowlocal = sendcounts[rank] / col;
    int **local_matrix = CrearArray2D_int(rowlocal + 2, col); // +2 for ghost rows
    int **local_edge_matrix = CrearArray2D_int(rowlocal, col);

    // --- Scatter the Image Data ---
    if (rank == 0) {
        for (i = 0; i < row; i++) {
          for (j = 0; j < col; j++){
            img_edge.matrix[i][j] = img_data.matrix[i][j];
          }
        }
        
        for (i = 0; i < size; i++) {
          int rows_to_send = sendcounts[i] / col;
          if (i != 0) {
            int offset = displs[i] / col;
            for (int k = 0; k < rows_to_send + 2; k++){
              for (int l = 0; l < col; l++){
                if (k == 0) {
                  local_matrix[k][l] = img_data.matrix[offset - 1][l];
                } else if (k == rows_to_send + 1) {
                  local_matrix[k][l] = img_data.matrix[offset + k - 1][l];
                } else {
                  local_matrix[k][l] = img_data.matrix[offset + k - 1][l];
                }
              }
            }
            for (int k = 0; k < rows_to_send + 2; k++) {
              MPI_Send(local_matrix[k], col, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
          } else {
            for (int k = 0; k < rowlocal + 2; k++){
              for (int l = 0; l < col; l++){
                if (k == 0) {
                  local_matrix[k][l] = img_data.matrix[k][l];
                } else if (k == rowlocal + 1) {
                  local_matrix[k][l] = img_data.matrix[k][l];
                } else {
                  local_matrix[k][l] = img_data.matrix[k][l];
                }
              }
            }
          }
        }
    } else {
      for (int k = 0; k < rowlocal + 2; k++){
        MPI_Recv(local_matrix[k], col, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
    }

    // --- Apply Laplace Filter ---
    double start_time, end_time;
    start_time = MPI_Wtime();

    Filtro_Laplace(local_matrix, local_edge_matrix, rowlocal + 2, col); // Apply on local data

    end_time = MPI_Wtime();
    printf("Proceso %d: Tiempo de cálculo del filtro = %lf segundos\n", rank, end_time - start_time);

    // --- Gather the Results ---
    int *recvcounts = NULL;
    int *recvdispls = NULL;

    if (rank == 0) {
        recvcounts = (int *)malloc(size * sizeof(int));
        recvdispls = (int *)malloc(size * sizeof(int));

        int base_rows = row / size;
        int extra = row % size;
        int offset = 0;

        for (i = 0; i < size; i++) {
            recvcounts[i] = base_rows;
            if (i < extra) {
                recvcounts[i]++;
            }
            recvcounts[i] *= col;
            recvdispls[i] = offset * col;
            offset += base_rows;
            if (i < extra) {
                offset++;
            }
        }
    }

    if (rank != 0) {
      for (int k = 0; k < rowlocal; k++){
        MPI_Send(local_edge_matrix[k], col, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
    } else {
      for (int i = 1; i < size; i++) {
        int rows_to_recv = recvcounts[i] / col;
        int offset = recvdispls[i] / col;
        int **temp_matrix = CrearArray2D_int(rows_to_recv, col);

        for (int k = 0; k < rows_to_recv; k++){
          MPI_Recv(temp_matrix[k], col, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        for (int k = 0; k < rows_to_recv; k++){
          for (int l = 0; l < col; l++){
            img_edge.matrix[offset + k][l] = temp_matrix[k][l];
          }
        }
        LiberarArray2D_int(rows_to_recv, temp_matrix);
      }
    }

    // --- Process 0: Write the Result ---
    if (rank == 0) {
        printf("  Guardando la imagen con la detección de aristas en \"%s\"\n\n", archivo_imagen_aristas);
        writePGM(archivo_imagen_aristas, &img_edge);

        LiberarArray2D_int(img_data.row, img_data.matrix);
        LiberarArray2D_int(img_edge.row, img_edge.matrix);
        free(sendcounts);
        free(displs);
        free(recvcounts);
        free(recvdispls);
    }

    LiberarArray2D_int(rowlocal + 2, local_matrix);
    LiberarArray2D_int(rowlocal, local_edge_matrix);

    MPI_Finalize();
    return 0;
}