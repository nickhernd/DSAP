/**
 * PRÁCTICA 2: primos - Implementación paralela con MPI
 * 
 * Autor: Jaime Hernández Delgado
 * DNI: 48776654W
 */

// ./mpirunR primos_mpi -np 3 -bind-to core host_file.txt

// mpirun -np <número-de-procesos> ./primos_mpi

 #include <math.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <mpi.h>
 
 int numero_primos(int n, int myrank, int nprocs);
 int esPrimo(int p);
 int numero_primos_sec(int n);
 
 int main(int argc, char *argv[]) {
     int n, n_factor, n_min, n_max;
     int primos, primos_parte;
     int myrank, nprocs;
     double t0, t1, tiempo_paralelo, tiempo_secuencial;
     double *tiempos_parciales;
     
     // Inicialización del entorno MPI
     MPI_Init(&argc, &argv);
     MPI_Comm_rank(MPI_COMM_WORLD, &myrank);  
     MPI_Comm_size(MPI_COMM_WORLD, &nprocs); 
     
    
     n_min = 500;        
     n_max = 5000000;    
     n_factor = 10;   
     
     // Reservar memoria para almacenar tiempos parciales de cada proceso
     tiempos_parciales = (double *)malloc(nprocs * sizeof(double));
     if (tiempos_parciales == NULL) {
         fprintf(stderr, "Error de asignación de memoria\n");
         MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
     }
     
     // Solo el proceso root muestra información inicial
     if (myrank == 0) {
         printf("\n");
         printf("Programa PARALELO para contar el número de primos menores que un valor.\n");
         printf("Ejecutando con %d procesos\n", nprocs);
         printf("\n");
     }
     
     // Bucle principal
     n = n_min;
     while (n <= n_max) {
         // Cálculo secuencial
         if (myrank == 0) {
             t0 = MPI_Wtime(); 
             primos = numero_primos_sec(n);
             t1 = MPI_Wtime();  
             tiempo_secuencial = t1 - t0;
             printf("Primos menores que %d: %d. Tiempo secuencial: %.2f s.\n", 
                    n, primos, tiempo_secuencial);
         }
         
         // Sincronizar todos los procesos antes de comenzar la versión paralela
         MPI_Barrier(MPI_COMM_WORLD);
         
         t0 = MPI_Wtime();
         
         primos_parte = numero_primos(n, myrank, nprocs);
         
         // Cada proceso registra su tiempo de ejecución individual
         t1 = MPI_Wtime();
         double tiempo_parcial = t1 - t0;
         
         // Suma de todos los primos parciales
         MPI_Reduce(&primos_parte, &primos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
         
         // El proceso root calcula el tiempo total de la versión paralela
         if (myrank == 0) {
             tiempo_paralelo = MPI_Wtime() - t0;
         }
         
         MPI_Gather(&tiempo_parcial, 1, MPI_DOUBLE, tiempos_parciales, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
         
         if (myrank == 0) {
             printf("Primos menores que %d: %d. Tiempo paralelo : %.2f s. Tiempos parciales: ", 
                    n, primos, tiempo_paralelo);
                    
             for (int i = 0; i < nprocs; i++) {
                 printf("%.2f ", tiempos_parciales[i]);
             }
             printf("\n");
             
             double speedup = tiempo_secuencial / tiempo_paralelo;
             double eficiencia = speedup / nprocs;
             printf("Speed-up: %.2f, Eficiencia: %.2f\n\n", speedup, eficiencia);
         }
         
         // Preparación del siguiente valor de n
         n = n * n_factor;
     }
     
     // Liberación de recursos y finalización de MPI
     free(tiempos_parciales);
     MPI_Finalize();
     
     return EXIT_SUCCESS;
 }
 

 int numero_primos(int n, int myrank, int nprocs) {
     int total_primos = 0;
     
     for (int i = 2 + myrank; i <= n; i += nprocs) {
         if (esPrimo(i)) {
             total_primos++;
         }
     }
     
     return total_primos;
 }
 
 int numero_primos_sec(int n) {
     int total = 0;
     for (int i = 2; i <= n; i++) {
         if (esPrimo(i)) {
             total++;
         }
     }
     return total;
 }
 
 int esPrimo(int p) {
     if (p <= 1) return 0;
     if (p == 2) return 1;
     if (p % 2 == 0) return 0;
     
     int limite = (int)sqrt(p) + 1;
     for (int i = 3; i <= limite; i += 2) {
         if (p % i == 0) {
             return 0;
         }
     }
     return 1;
 }