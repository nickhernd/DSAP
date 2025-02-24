/*
 * Autor: Jaime Hernández Delgado
 * DNI: 48776654W
 * 
 * Práctica 1: Contar - DSAP
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <mpi.h>
 
//  ./mpirunR contar_mpi -np 3 -bind-to core host_file.txt 

 // Constantes definidas según el enunciado
 #define MAXENTERO 100      // Valor máximo de los datos donde buscar
 #define REPETICIONES 100000 // Número de repeticiones
 #define NUMBUSCADO 10      // Valor buscado en el conjunto de datos
 #define MAXTAM 100000      // Máximo tamaño del vector
 
 int main(int argc, char *argv[]) {
     int myrank, nprocs;
     int tamanyo;           // Tamaño del vector
     int *vector = NULL;    // Vector de datos
     int numVeces = 0;      // Contador de ocurrencias
     int numVecesLocal = 0; // Contador local para cada proceso
     clock_t t0, tf;        // Variables para medir tiempo
     double tsec, tpar;     // Tiempos secuencial y paralelo
 
     // Inicialización de MPI
     MPI_Init(&argc, &argv);
     MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
     MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
 
     // El proceso 0 (root) lee el tamaño del vector
     if (myrank == 0) {
         do {
             printf("", MAXTAM);  // Tamaño del vector puesto   
             scanf("%d", &tamanyo);
         } while (tamanyo <= 0 || tamanyo > MAXTAM);
     }
 
     // Enviar tamaño a todos los procesos usando Send/Recv
     if (myrank == 0) {
         for (int i = 1; i < nprocs; i++) {
             MPI_Send(&tamanyo, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
         }
     } else {
         MPI_Recv(&tamanyo, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     }
 
     // Reserva de memoria para el vector
     vector = (int *)malloc(tamanyo * sizeof(int));
     if (vector == NULL) {
         printf("Error en la reserva de memoria\n");
         MPI_Finalize();
         return 1;
     }
 
     // El proceso 0 genera el vector aleatorio
     if (myrank == 0) {
         srand(1); // Semilla fija para reproducibilidad
         for (int i = 0; i < tamanyo; i++) {
             vector[i] = 1 + ((double)(MAXENTERO) * rand()) / RAND_MAX;
         }
 
         // Ejecución secuencial
         t0 = clock();
         numVeces = 0;
         for (int i = 0; i < REPETICIONES; i++) {
             for (int j = 0; j < tamanyo; j++) {
                 if (vector[j] == NUMBUSCADO) numVeces++;
             }
         }
         tf = clock();
         tsec = (double)(tf - t0) / CLOCKS_PER_SEC;
 
         printf("%d %d (%.2f%%)", 
                NUMBUSCADO, numVeces, 
                (100.0 * numVeces) / (tamanyo * (double)REPETICIONES));
         printf(" %f ", tsec); // tiempo secuencial 
     }
 
     // Distribución del vector entre los procesos
     int elemsPorProc = tamanyo / nprocs;
     int elemsSobrantes = tamanyo % nprocs;
     int elemsLocales = (myrank == 0) ? elemsPorProc + elemsSobrantes : elemsPorProc;
     int *vectorLocal = (int *)malloc(elemsLocales * sizeof(int));
 
     // Envío del vector a todos los procesos
     if (myrank == 0) {
         // El proceso 0 se queda con su parte
         for (int i = 0; i < elemsLocales; i++) {
             vectorLocal[i] = vector[i];
         }
         // Envía al resto de procesos
         int offset = elemsLocales;
         for (int i = 1; i < nprocs; i++) {
             MPI_Send(&vector[offset], elemsPorProc, MPI_INT, i, 0, MPI_COMM_WORLD);
             offset += elemsPorProc;
         }
     } else {
         MPI_Recv(vectorLocal, elemsPorProc, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     }
 
     // Sincronización antes de comenzar la medición paralela
     MPI_Barrier(MPI_COMM_WORLD);
 
     // Inicio de la ejecución paralela
     if (myrank == 0) t0 = clock();
 
     // Búsqueda en la parte local del vector
     numVecesLocal = 0;
     for (int i = 0; i < REPETICIONES; i++) {
         for (int j = 0; j < elemsLocales; j++) {
             if (vectorLocal[j] == NUMBUSCADO) numVecesLocal++;
         }
     }
 
     // Recolección de resultados
     if (myrank == 0) {
         numVeces = numVecesLocal;
         int numVecesTemp;
         for (int i = 1; i < nprocs; i++) {
             MPI_Recv(&numVecesTemp, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
             numVeces += numVecesTemp;
         }
         tf = clock();
         tpar = (double)(tf - t0) / CLOCKS_PER_SEC;
 
         // Cálculo y muestra de resultados
         double speedup = tsec / tpar;
         double eficiencia = speedup / nprocs;
 
         printf("%d ", nprocs); // numero de procesos
         printf("%f ", tpar); // tiempo paralelo
         printf("%f ", speedup); // speedup
         printf("%f ", eficiencia); // eficiencia
     } else {
         MPI_Send(&numVecesLocal, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
     }
 
     // Liberación de memoria
     free(vectorLocal);
     if (myrank == 0) free(vector);
 
     MPI_Finalize();
     return 0;
 }