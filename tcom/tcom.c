/**
 * PRÁCTICA 3: tcom - Evaluación de los parámetros del modelo de coste de comunicaciones
 * Autor: Jaime Hernández Delgado
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <mpi.h>
 #include <string.h>
 #include <math.h>
 
 // Número de repeticiones para cada medición
 #define NUM_REPETICIONES 100
 
 // Tamaños de mensaje a probar (en bytes)
 const int tamanios[] = {
     1,          // Para estimar β (latencia)
     256,        // 2^8 bytes (32 doubles)
     512,        // 2^9 bytes (64 doubles)
     1024,       // 2^10 bytes = 1K (128 doubles)
     2048,       // 2^11 bytes = 2K (256 doubles)
     4096,       // 2^12 bytes = 4K (512 doubles)
     8192,       // 2^13 bytes = 8K (1024 doubles)
     16384,      // 2^14 bytes = 16K
     32768,      // 2^15 bytes = 32K
     65536,      // 2^16 bytes = 64K
     131072,     // 2^17 bytes = 128K
     262144,     // 2^18 bytes = 256K
     524288,     // 2^19 bytes = 512K
     1048576,    // 2^20 bytes = 1M
     2097152,    // 2^21 bytes = 2M
     4194304     // 2^22 bytes = 4M
 };
 
 #define NUM_TAMANIOS (sizeof(tamanios) / sizeof(tamanios[0]))
 
// Función para realizar el ping-pong entre dos procesos MPI
 double ping_pong(char *buffer, int tamanio, int rank, int repeticiones) {
     int i;
     double inicio, fin, tiempo_total = 0.0;
     MPI_Status status;
     
     // Sincronizamos los procesos antes de empezar
     MPI_Barrier(MPI_COMM_WORLD);
     
     for (i = 0; i < repeticiones; i++) {
         if (rank == 0) {
             // Proceso 0 envía y luego recibe
             inicio = MPI_Wtime();
             
             MPI_Send(buffer, tamanio, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
             MPI_Recv(buffer, tamanio, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);
             
             fin = MPI_Wtime();
             tiempo_total += (fin - inicio);
         } else {
             // Proceso 1 recibe y luego envía
             MPI_Recv(buffer, tamanio, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
             MPI_Send(buffer, tamanio, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
         }
     }
     
     // Convertimos el tiempo a microsegundos y dividimos por 2 (ida y vuelta)
     return (tiempo_total * 1000000.0) / repeticiones / 2.0;
 }
 

 int main(int argc, char **argv) {
     int rank, size;
     char *buffer;
     double tiempos[NUM_TAMANIOS];
     double beta, tau;
     int i;
     
     // Inicializamos MPI
     MPI_Init(&argc, &argv);
     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
     MPI_Comm_size(MPI_COMM_WORLD, &size);
     
     if (size != 2) {
         if (rank == 0) {
             printf("Este programa requiere exactamente 2 procesos MPI.\n");
         }
         MPI_Finalize();
         return 1;
     }
     
     // Reservamos el buffer más grande que necesitaremos
     buffer = (char *) malloc(tamanios[NUM_TAMANIOS - 1]);
     if (buffer == NULL) {
         printf("Error al reservar memoria para el buffer.\n");
         MPI_Finalize();
         return 1;
     }
     
     // Inicializamos el buffer con datos aleatorios
     if (rank == 0) {
         for (i = 0; i < tamanios[NUM_TAMANIOS - 1]; i++) {
             buffer[i] = (char) (rand() % 256);
         }
     }
     
     // Proceso 0 es el encargado de las mediciones y mostrar resultados
     if (rank == 0) {
         printf("Estimando parámetros del modelo de coste de comunicaciones...\n");
         printf("Realizando %d repeticiones para cada tamaño de mensaje.\n\n", NUM_REPETICIONES);
         
         printf("Tamaño (bytes)\tTiempo (us)\tτ (us/byte)\n");
         printf("-----------------------------------------\n");
     }
     
     // Realizamos las mediciones para cada tamaño
     for (i = 0; i < NUM_TAMANIOS; i++) {
         double tiempo = ping_pong(buffer, tamanios[i], rank, NUM_REPETICIONES);
         
         if (rank == 0) {
             tiempos[i] = tiempo;
             
             if (i == 0) {
                 // El primer tamaño (1 byte) nos da una estimación de β (latencia)
                 beta = tiempo;
                 printf("%d\t\t%.2f\t\tN/A (β)\n", tamanios[i], tiempo);
             } else {
                 // Para el resto calculamos τ (tiempo por byte)
                 tau = (tiempo - beta) / tamanios[i];
                 printf("%d\t\t%.2f\t\t%.8f\n", tamanios[i], tiempo, tau);
             }
         }
     }
     
     if (rank == 0) {
         printf("\nResultados del modelo de coste:\n");
         printf("β (latencia) = %.2f microsegundos\n", beta);
         printf("τ (tiempo/byte) para diferentes tamaños de mensaje:\n");
         
         for (i = 1; i < NUM_TAMANIOS; i++) {
             tau = (tiempos[i] - beta) / tamanios[i];
             printf("Tamaño %d bytes: τ = %.8f microsegundos/byte\n", tamanios[i], tau);
         }
         
         printf("\nModelo de coste: tcom = %.2f + τ * Tamaño_Mensaje\n", beta);
         printf("Donde τ varía según el tamaño del mensaje como se muestra arriba.\n");
     }
     
     // Liberamos recursos
     free(buffer);
     MPI_Finalize();
     
     return 0;
 }