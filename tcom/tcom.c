/**
 * PRÁCTICA 3: tcom - Evaluación de los parámetros del modelo de coste de comunicaciones
 * 
 * Programa para estimar los parámetros β (latencia) y τ (tiempo por byte) del modelo
 * de coste de comunicaciones en MPI usando el esquema ping-pong.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Configuración de las pruebas
#define NUM_REPETICIONES 1000   // Número de repeticiones para cada medición
#define CALENTAMIENTO 10        // Número de iteraciones de calentamiento (no se miden)
#define OUTLIER_THRESHOLD 3.0   // Umbral para eliminar valores atípicos (en desviaciones estándar)

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

// Estructura para almacenar resultados estadísticos
typedef struct {
    double min;
    double max;
    double media;
    double desviacion;
} Estadisticas;

// Función para calcular estadísticas eliminando outliers
Estadisticas calcular_estadisticas(double *tiempos, int n) {
    Estadisticas stats = {0};
    double sum = 0.0, sum_sq = 0.0;
    int i, count = 0;
    
    // Primera pasada para calcular media y desviación
    for (i = 0; i < n; i++) {
        sum += tiempos[i];
        sum_sq += tiempos[i] * tiempos[i];
    }
    stats.media = sum / n;
    stats.desviacion = sqrt((sum_sq / n) - (stats.media * stats.media));
    
    // Segunda pasada para eliminar outliers y calcular min/max
    stats.min = 1e12;
    stats.max = 0.0;
    sum = 0.0;
    count = 0;
    
    for (i = 0; i < n; i++) {
        if (fabs(tiempos[i] - stats.media) <= OUTLIER_THRESHOLD * stats.desviacion) {
            sum += tiempos[i];
            if (tiempos[i] < stats.min) stats.min = tiempos[i];
            if (tiempos[i] > stats.max) stats.max = tiempos[i];
            count++;
        }
    }
    
    // Recalculamos la media sin outliers
    if (count > 0) {
        stats.media = sum / count;
    }
    
    return stats;
}

// Función para realizar el ping-pong entre dos procesos MPI
Estadisticas ping_pong(char *buffer, int tamanio, int rank, int repeticiones) {
    int i;
    double *tiempos = (double *)malloc(repeticiones * sizeof(double));
    MPI_Status status;
    
    // Fase de calentamiento (no medimos estos tiempos)
    for (i = 0; i < CALENTAMIENTO; i++) {
        if (rank == 0) {
            MPI_Send(buffer, tamanio, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(buffer, tamanio, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);
        } else {
            MPI_Recv(buffer, tamanio, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Send(buffer, tamanio, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        }
    }
    
    // Sincronizamos los procesos antes de empezar las mediciones
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Fase de medición
    for (i = 0; i < repeticiones; i++) {
        if (rank == 0) {
            double inicio = MPI_Wtime();
            MPI_Send(buffer, tamanio, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(buffer, tamanio, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);
            double fin = MPI_Wtime();
            tiempos[i] = (fin - inicio) * 500000.0; // Convertir a us y dividir por 2 (ida y vuelta)
        } else {
            MPI_Recv(buffer, tamanio, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Send(buffer, tamanio, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        }
    }
    
    // Proceso 0 calcula estadísticas
    Estadisticas stats = {0};
    if (rank == 0) {
        stats = calcular_estadisticas(tiempos, repeticiones);
    }
    
    free(tiempos);
    return stats;
}

int main(int argc, char **argv) {
    int rank, size;
    char *buffer;
    double beta, tau[NUM_TAMANIOS];
    Estadisticas stats[NUM_TAMANIOS];
    int i;
    
    // Inicializamos MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (size != 2) {
        if (rank == 0) {
            fprintf(stderr, "Este programa requiere exactamente 2 procesos MPI.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    // Reservamos el buffer más grande que necesitaremos
    buffer = (char *)malloc(tamanios[NUM_TAMANIOS - 1]);
    if (buffer == NULL) {
        fprintf(stderr, "Error al reservar memoria para el buffer.\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    // Inicializamos el buffer con datos aleatorios
    if (rank == 0) {
        for (i = 0; i < tamanios[NUM_TAMANIOS - 1]; i++) {
            buffer[i] = (char)(rand() % 256);
        }
    }
    
    // Realizamos las mediciones para cada tamaño
    for (i = 0; i < NUM_TAMANIOS; i++) {
        stats[i] = ping_pong(buffer, tamanios[i], rank, NUM_REPETICIONES);
    }
    
    // Proceso 0 muestra los resultados
    if (rank == 0) {
        // Imprimimos resultados por pantalla
        printf("\nResultados del modelo de coste de comunicaciones:\n");
        printf("------------------------------------------------\n");
        printf("%-10s %-12s %-12s %-12s %-12s\n", "Tamaño", "Tiempo (us)", "τ (us/byte)", "Mínimo", "Máximo");
        printf("------------------------------------------------\n");
        
        for (i = 0; i < NUM_TAMANIOS; i++) {
            if (i == 0) {
                beta = stats[i].media;
                printf("%-10d %-12.2f %-12s %-12.2f %-12.2f\n", 
                       tamanios[i], stats[i].media, "N/A (β)", stats[i].min, stats[i].max);
            } else {
                tau[i] = (stats[i].media - beta) / tamanios[i];
                printf("%-10d %-12.2f %-12.8f %-12.2f %-12.2f\n", 
                       tamanios[i], stats[i].media, tau[i], stats[i].min, stats[i].max);
            }
        }
        
        // Escribimos resultados en CSV
        FILE *csv = fopen("resultados_tcom.csv", "w");
        if (csv != NULL) {
            fprintf(csv, "Tamaño(bytes),Tiempo(us),Tau(us/byte),Min(us),Max(us),Beta(us)\n");
            for (i = 0; i < NUM_TAMANIOS; i++) {
                if (i == 0) {
                    fprintf(csv, "%d,%.2f,N/A,%.2f,%.2f,%.2f\n", 
                            tamanios[i], stats[i].media, stats[i].min, stats[i].max, stats[i].media);
                } else {
                    fprintf(csv, "%d,%.2f,%.8f,%.2f,%.2f,%.2f\n", 
                            tamanios[i], stats[i].media, tau[i], stats[i].min, stats[i].max, beta);
                }
            }
            fclose(csv);
            printf("\nResultados guardados en 'resultados_tcom.csv'\n");
        } else {
            fprintf(stderr, "Error al crear el archivo CSV.\n");
        }
    }
    
    // Liberamos recursos
    free(buffer);
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
