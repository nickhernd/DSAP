CC = mpicc
CFLAGS = -Wall -O3
LDFLAGS = -lm

all: tcom

tcom: tcom.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f tcom resultados_tcom.csv
