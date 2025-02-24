#!/bin/bash

# Crear el archivo CSV con el encabezado
echo "Iteración,Tamaño Vector,Procesadores,Resultado" > param_contar.csv

# Tamaño inicial del vector
tamano=10000

# Ejecutar el ciclo para cada tamaño de vector (15 tamaños)
for ((i=1; i<=15; i++)); do
    # Ejecutar 30 veces para cada combinación de tamaño de vector y número de procesadores (2, 4, 6, 8)
    for proc in 2 4 6 8; do
        # Ejecutar el comando y redirigir la salida a una variable, pasando el tamaño del vector y el número de procesadores
        for ((j=1; j<=30; j++)); do
            resultado=$(echo $tamano | ./mpirunR -np $proc -bind-to core -f host_file.txt contar_mpi)
            
            # Agregar los resultados al archivo CSV
            echo "$j,$tamano,$proc,$resultado" >> param_contar.csv
        done
    done
    
    # Incrementar el tamaño del vector en 10,000 para la siguiente iteración
    tamano=$((tamano + 10000))
done
