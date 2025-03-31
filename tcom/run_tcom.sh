#!/bin/bash

# Configuración
USUARIO=$USU_UACLOUD
PROGRAMA="tcom"
HOSTFILE="hostfile_tcom.txt"
OUTPUT_CSV="resultados_tcom.csv"
NUM_REPETICIONES=1000

# Crear hostfile específico para esta práctica (solo 2 nodos)
echo "clL01-8:1" > $HOSTFILE
echo "clL01-9:1" >> $HOSTFILE

# Compilar el programa
mpicc -o $PROGRAMA ${PROGRAMA}.c -lm

# Ejecutar el programa
echo "Ejecutando mediciones de parámetros de comunicaciones..."
mpirun -np 2 --hostfile $HOSTFILE ./$PROGRAMA

# Verificar si se creó el archivo CSV
if [ -f "$OUTPUT_CSV" ]; then
    echo "Resultados guardados en $OUTPUT_CSV"
    
    # Generar gráficas usando Python (opcional)
    if command -v python3 &>/dev/null; then
        echo "Generando gráficas..."
        python3 << END
import pandas as pd
import matplotlib.pyplot as plt

# Leer datos
data = pd.read_csv('$OUTPUT_CSV')

# Filtrar datos para Tau (excluir la primera fila que es para beta)
tau_data = data[data['Tamaño(bytes)'] != 1]

# Gráfica 1: Tiempo de comunicación vs tamaño del mensaje
plt.figure(figsize=(10, 6))
plt.plot(data['Tamaño(bytes)'], data['Tiempo(us)'], 'bo-')
plt.xscale('log')
plt.yscale('log')
plt.xlabel('Tamaño del mensaje (bytes) - escala log')
plt.ylabel('Tiempo de comunicación (µs) - escala log')
plt.title('Tiempo de comunicación vs Tamaño del mensaje')
plt.grid(True, which="both", ls="--")
plt.savefig('tiempo_vs_tamano.png')
plt.close()

# Gráfica 2: Tau vs tamaño del mensaje
plt.figure(figsize=(10, 6))
plt.plot(tau_data['Tamaño(bytes)'], tau_data['Tau(us/byte)'], 'ro-')
plt.xscale('log')
plt.xlabel('Tamaño del mensaje (bytes) - escala log')
plt.ylabel('τ (µs/byte)')
plt.title('τ (tiempo por byte) vs Tamaño del mensaje')
plt.grid(True, which="both", ls="--")
plt.savefig('tau_vs_tamano.png')
plt.close()

print("Gráficas generadas: tiempo_vs_tamano.png y tau_vs_tamano.png")
END
    fi
else
    echo "Error: No se pudo crear el archivo CSV con los resultados."
    exit 1
fi
