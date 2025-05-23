# include <math.h> 
# include <stdio.h> 
# include <stdlib.h> 
# include <time.h> 

// Devuelve el numero de primos menores que n
int numero_primos_sec( int n ) 
{ 
	int primo,total=0; 
	for (int i = 2; i <= n; i++) 
	{ 
		if (esPrimo(i)==1)
			total++;
	} 
	return total; 
} 

int esPrimo(int p)
{
	for (int i=2;i<=sqrt(p);i++)
	{
		if (p%i == 0)
			return 0;
	}
	return 1;
}

int main ( int argc, char *argv[] ) 
{ 
	int n, n_factor, n_min, n_max; 
	int primos, primos_parte;
	unsigned t0,t1; 
	n_min = 500; 
	n_max = 5000000; 
	n_factor = 10; 
	printf ( "\n" ); 
	printf ( " Programa SECUENCIAL para contar el número de primos menores que un valor.\n" ); 
	printf ( "\n" ); 
	n = n_min; 
	while ( n <= n_max ) 
	{ 
		t0 = clock();
		primos = numero_primos_sec(n);
		t1 = clock();
		printf( " Primos menores que %10d: %10d. Tiempo secuencial: %5.2f segundos.\n", n, primos, (double)(t1-t0)/CLOCKS_PER_SEC);
		n = n * n_factor; 
	} 

	printf("\n");
	printf("====== PROGRAMA PARALELO ======")
	printf ( "\n" ); 

	int rank = 0;
	int size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(size < 2) {
		printf("No hay número de nodos disponibles.\n");
		MPI_Finalize();
		exit(-1);
	}

	if(rank == 0) {
		
	}

	return 0; 
} 