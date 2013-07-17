#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <errno.h>
#include "semaphores.h"

#define CICLOS 10

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
// La union ya está definida en sys/sem.h
#else
// Tenemos que definir la union
union semun 
{ 
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};
#endif

char *pais[3]={"Peru","Bolivia","Colombia"};
union semun arg;


void proceso(int i, int idsem)
{
    int k;
    int l;

    for(k=0;k<CICLOS;k++)
    {

        printf("Llega el pais: %s\n", pais[i]);
        // Llamada waitsem
        if(Semwait(idsem) == -1){
            printf("Error al en el wait del semaforo: %d\n", errno);
            exit(0);
        }

        // Entrada a la sección crítica
        printf("--Entra %s %d\n",pais[i], k);
        fflush(stdout);
        sleep(rand()%3);
        printf("**Sale  %s %d\n",pais[i], k);

        // Salida de la sección crítica

        // Llamada sigsem
        if(Semsignal(idsem) == -1){
            printf("Error al en el signal del semaforo: %d\n", errno);
            exit(0);
        }

        // Espera aleatoria fuera de la sección crítica
        sleep(rand()%3);
    }

    exit(0); // Termina el proceso
}

int main()
{
    int pid;  
    int i;
    int status;
    int id_semaforo;
    

    // Obtener ID del semaforo
    // Solo se genera un semaforo 
    id_semaforo = semget(IPC_PRIVATE, 1, 0600);
    if(id_semaforo== -1){
        printf("Error al generar semaforo: %d\n", errno);
        exit(0);
    }

    // Inicializa el semaforo con valor inicial 1
    if(Seminit(id_semaforo, 1) == -1){
       printf("Error al inicializar semaforo: %d\n", errno);
       exit(0);  
    }

    srand(getpid());
    for(i=0;i<3;i++)
    {
        // Crea un nuevo proceso hijo que ejecuta la función proceso()
        pid=fork();
        if(pid==0)
            proceso(i, id_semaforo);
    }

    for(i=0;i<3;i++)
    {
        pid = wait(&status);
    }

}


int Seminit(int idsem, int val){

    arg.val = val;
    //printf("idsem: %d\n", idsem);
    return semctl(idsem, 0, SETVAL, arg);

}

// Funcion wait del semaforo
int Semwait(int idsem){

    struct sembuf Operacion;

    Operacion.sem_num = 0;
    Operacion.sem_op  = -1;
    Operacion.sem_flg = 0;

    return semop (idsem, &Operacion, 1);

}


// Funcion signal del semaforo
int Semsignal(int idsem){

    struct sembuf Operacion;

    Operacion.sem_num = 0;
    Operacion.sem_op  = 1;
    Operacion.sem_flg = 0;

    return semop (idsem, &Operacion, 1);

}













