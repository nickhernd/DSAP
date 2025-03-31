#!/bin/bash

# Configuración
USUARIO=$USU_UACLOUD
PROGRAMA="primos_mpi"
HOSTFILE="hostfile.txt"
OUTPUT_CSV="resultados_primos.csv"
MAX_NODOS=4
MAX_CORES_POR_NODO=4

# Crear archivo CSV con cabeceras
echo "n,nprocs,nodos,cores_por_nodo,tiempo_secuencial,tiempo_paralelo,speedup,eficiencia" > $OUTPUT_CSV

# Compilar el programa
mpicc -o $PROGRAMA ${PROGRAMA}.c -lm

# Función para ejecutar pruebas con diferentes configuraciones
function ejecutar_pruebas {
    local nprocs=$1
    local nodos=$2
    local cores_por_nodo=$3
    
    # Crear hostfile temporal
    local hostfile_temp="hostfile_${nprocs}_${nodos}_${cores}.tmp"
    head -n $nodos $HOSTFILE | awk -v cores=$cores_por_nodo '{print $1 ":" cores}' > $hostfile_temp
    
    # Ejecutar el programa
    echo "Ejecutando con $nprocs procesos ($nodos nodos, $cores_por_nodo cores por nodo)..."
    output=$(mpirun -np $nprocs --hostfile $hostfile_temp ./$PROGRAMA 2>&1)
    
    # Extraer resultados
    while read -r line; do
        if [[ $line =~ "Primos menores que" ]]; then
            if [[ $line =~ "Tiempo secuencial" ]]; then
                n=$(echo $line | awk '{print $4}' | tr -d ',')
                tiempo_secuencial=$(echo $line | awk '{print $8}')
            elif [[ $line =~ "Tiempo paralelo" ]]; then
                tiempo_paralelo=$(echo $line | awk '{print $8}')
            fi
        elif [[ $line =~ "Speed-up" ]]; then
            speedup=$(echo $line | awk '{print $2}' | tr -d ',')
            eficiencia=$(echo $line | awk '{print $4}')
            
            # Guardar en CSV
            echo "$n,$nprocs,$nodos,$cores_por_nodo,$tiempo_secuencial,$tiempo_paralelo,$speedup,$eficiencia" >> $OUTPUT_CSV
        fi
    done <<< "$output"
    
    # Eliminar hostfile temporal
    rm $hostfile_temp
}

# Configuraciones de prueba según la práctica:
# 1. 1 nodo, con 2, 3 y 4 cores (2, 3 y 4 procesos)
for cores in 2 3 4; do
    ejecutar_pruebas $cores 1 $cores
done

# 2. 2 nodos, con 2, 3 y 4 cores por nodo (4, 6 y 8 procesos)
for cores in 2 3 4; do
    ejecutar_pruebas $((2*cores)) 2 $cores
done

# 3. 4 nodos, con 1, 2, 3 y 4 cores por nodo (4, 8, 12 y 16 procesos)
for cores in 1 2 3 4; do
    ejecutar_pruebas $((4*cores)) 4 $cores
done

echo "Pruebas completadas. Resultados guardados en $OUTPUT_CSV"
