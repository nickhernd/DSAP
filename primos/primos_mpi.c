/**
 * PRÁCTICA 2: primos - Implementación paralela con MPI
 * 
 * Programa que calcula el número de enteros primos menores que un valor dado n
 * utilizando múltiples procesos y una distribución cíclica de la carga.
 */

 #include <math.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <mpi.h>
 
 int numero_primos(int n, int myrank, int nprocs);
 int esPrimo(int p);
 int numero_primos_sec(int n);
 
 int main(int argc, char *argv[])
 {
     int n, n_factor, n_min, n_max;
     int primos, primos_parte;
     int myrank, nprocs;
     double t0, t1, tiempo_paralelo, tiempo_secuencial;
     double *tiempos_parciales;
     
     // Inicializamos MPI
     MPI_Init(&argc, &argv);
     MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
     MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
     
     // Parámetros de la simulación
     n_min = 500;
     n_max = 5000000;
     n_factor = 10;
     
     // Reservamos memoria para los tiempos parciales
     tiempos_parciales = (double *)malloc(nprocs * sizeof(double));
     if (tiempos_parciales == NULL) {
         printf("Error de asignación de memoria\n");
         MPI_Abort(MPI_COMM_WORLD, 1);
     }
     
     // Mostramos información inicial (solo proceso 0)
     if (myrank == 0) {
         printf("\n");
         printf(" Programa PARALELO para contar el número de primos menores que un valor.\n");
         printf(" Ejecutando con %d procesos\n", nprocs);
         printf("\n");
     }
     
     // Para cada valor de n entre n_min y n_max
     n = n_min;
     while (n <= n_max) {
         // El proceso root calcula la versión secuencial primero
         if (myrank == 0) {
             t0 = MPI_Wtime();
             primos = numero_primos_sec(n);
             t1 = MPI_Wtime();
             tiempo_secuencial = t1 - t0;
             printf(" Primos menores que %10d: %10d. Tiempo secuencial: %f segundos.\n", 
                    n, primos, tiempo_secuencial);
         }
         
         // Sincronizamos todos los procesos antes de comenzar la versión paralela
         MPI_Barrier(MPI_COMM_WORLD);
         
         // Comenzamos a medir el tiempo de la versión paralela
         t0 = MPI_Wtime();
         
         // Cada proceso calcula su parte
         primos_parte = numero_primos(n, myrank, nprocs);
         
         // Cada proceso registra su tiempo parcial
         t1 = MPI_Wtime();
         double tiempo_parcial = t1 - t0;
         
         // Recopilamos el número total de primos usando MPI_Reduce
         MPI_Reduce(&primos_parte, &primos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
         
         // Finalizamos la medición del tiempo paralelo (solo proceso 0)
         if (myrank == 0) {
             tiempo_paralelo = MPI_Wtime() - t0;
         }
         
         // Recopilamos los tiempos parciales de cada proceso
         // Importante: Esto no debe contabilizarse en el tiempo total
         MPI_Gather(&tiempo_parcial, 1, MPI_DOUBLE, tiempos_parciales, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
         
         // El proceso 0 muestra los resultados
         if (myrank == 0) {
             printf(" Primos menores que %10d: %10d. Tiempo paralelo : %f s. Tiempos parciales: ", 
                    n, primos, tiempo_paralelo);
                    
             for (int i = 0; i < nprocs; i++) {
                 printf("%f ", tiempos_parciales[i]);
             }
             printf("\n");
             
             // Calculamos y mostramos el speed-up y la eficiencia
             double speedup = tiempo_secuencial / tiempo_paralelo;
             double eficiencia = speedup / nprocs;
             printf(" Speed-up: %f, Eficiencia: %f\n\n", speedup, eficiencia);
         }
         
         // Pasamos al siguiente valor de n
         n = n * n_factor;
     }
     
     // Liberamos memoria y finalizamos MPI
     free(tiempos_parciales);
     MPI_Finalize();
     
     return 0;
 }
 
 /**
  * Función que cuenta el número de primos menores o iguales que n
  * calculados por un proceso específico usando distribución cíclica
  */
 int numero_primos(int n, int myrank, int nprocs)
 {
     int total_primos = 0;
     
     // Distribución cíclica: cada proceso evalúa los números 
     // que coinciden con su rango (mod nprocs)
     for (int i = 2 + myrank; i <= n; i = i + nprocs) {
         if (esPrimo(i) == 1) {
             total_primos++;
         }
     }
     
     return total_primos;
 }
 
 /**
  * Función que cuenta el número de primos menores o iguales que n
  * (versión secuencial para comparar tiempos)
  */
 int numero_primos_sec(int n)
 {
     int total = 0;
     for (int i = 2; i <= n; i++) {
         if (esPrimo(i) == 1) {
             total++;
         }
     }
     return total;
 }
 
 /**
  * Determina si un número es primo
  * Devuelve 1 si es primo, 0 en caso contrario
  */
 int esPrimo(int p)
 {
     for (int i = 2; i <= sqrt(p); i++) {
         if (p % i == 0) {
             return 0;
         }
     }
     return 1;
 }