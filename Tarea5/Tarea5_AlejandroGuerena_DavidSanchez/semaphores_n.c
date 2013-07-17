#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define atomic_xchg(A,B) __asm__ __volatile__( \
" lock xchg %1,%0 ;\n" \
: "=ir" (A) \
: "m" (B), "ir" (A) \
);

#define CICLOS 10
#define N_PROCESS 4

typedef struct {
    int pid;
} DATOSPROCESO;

//struct DATOSPROCESO {
//    int pid;
//};

typedef struct {
    int cola[N_PROCESS];
    int ent;
    int sal;
} COLAPROC;

typedef struct {
    int contador;
    COLAPROC bloqueados;
    DATOSPROCESO lproceso[N_PROCESS];
} SEMAPHORE;


char *pais[N_PROCESS]={"Peru","Bolivia","Colombia", "Argentina"};
int *g;

//struct DATOSPROCESO lproceso[3];
SEMAPHORE *sem;

void initsem(SEMAPHORE *initsem, int valor);
void waitsem(SEMAPHORE *waitsem, int nproc);
void signalsem(SEMAPHORE *signalsem);
void mete_a_cola(COLAPROC *q,int nproceso);
int sacar_de_cola(COLAPROC *q);

void proceso(int i)
{

    int k;

    for(k=0;k<CICLOS;k++)
    {
        // Llamada waitsem

        //printf("Valor del contador antes del wait %d proceso %s\n", sem->contador, pais[i]);
        fflush(stdout);
        waitsem(sem, i);

        printf("--Entra %s %d\n",pais[i], k);
        fflush(stdout);
        sleep(rand()%3);
        printf("**Sale  %s %d\n",pais[i], k);

        // Llamada signalsem
        signalsem(sem);

        // Espera aleatoria fuera de la sección crítica
        sleep(rand()%3);
    }

    exit(0); // Termina el proceso
}


int main()
{
    int tpid;
    int status;
    int shmid, shmidg;
    int i;

    // Declarar memoria compartida
    shmidg=shmget(0x1234,sizeof(g),0666|IPC_CREAT);
    if(shmidg==-1)
    {
        perror("Error en la memoria compartida\n");
        exit(1);
    }

    shmid=shmget(0x2945,sizeof(sem),0666|IPC_CREAT);
    if(shmid==-1)
    {
        perror("Error en la memoria compartida\n");
        exit(1);
    }
    
    g=shmat(shmidg,NULL,0);
    if(g==NULL)
    {
        perror("Error en el shmat\n");
        exit(2);
    }

    sem=shmat(shmid,NULL,0);
    if(sem==NULL)
    {
        perror("Error en el shmat\n");
        exit(2);
    }
    
    // Incializar el contador del semáforo en 1 una vez que esté
    // en memoria compartida, de manera que solo a un proceso se le
    // permitirá entrar a la sección crítica
    *g=0;
    initsem(sem,1);
    srand(getpid());    

    printf("El contador se inicializa con: %d \n", sem->contador);

    for(i=0;i<N_PROCESS;i++)
    {
        // Crea un nuevo proceso hijo que ejecuta la función proceso()
        tpid=fork();
        if(tpid==0)
        {
            sem->lproceso[i].pid = getpid();
            //printf("Se crea proceso %d con PID: %d y LPID: %d\n", i, getpid(), sem->lproceso[i].pid);
            proceso(i);
        }
    }

    for(i=0;i<N_PROCESS;i++)
        tpid = wait(&status);
    

    // Eliminar la memoria compartida
    shmdt(g);
    shmdt(sem);

}


void waitsem(SEMAPHORE *waitsem, int nproc)
{
    int l;

    l=1;
    do { atomic_xchg(l, *g); } while(l!=0);
    // SC Inicia

    // Decrementar contador de semaforo
    waitsem->contador--;

    if(waitsem->contador < 0)
    {
        // Insertar proceso en cola de bloqueados
        mete_a_cola(&waitsem->bloqueados, nproc);
        
        printf("Se bloquea proceso %s PID: %d \n", pais[nproc], waitsem->lproceso[nproc].pid);
        fflush(stdout);
        
        *g=0;
        // Bloquear proceso
        kill(waitsem->lproceso[nproc].pid,SIGSTOP); 
    }
    else
    {
        //printf("NO se bloquea proceso %s PID: %d \n", pais[nproc], waitsem->lproceso[nproc].pid);
        //fflush(stdout);
        *g=0;
    }
    // SC Termina

}

void signalsem(SEMAPHORE *signalsem)
{
    int l;
    int contproceso;

    l=1;
    do { atomic_xchg(l, *g); } while(l!=0);
    // SC Inicia

    // Incrementar contador de semaforo
    signalsem->contador++;

    if(signalsem->contador <= 0)
    {
        // Sacar proceso de cola de bloqueados
        contproceso = sacar_de_cola(&signalsem->bloqueados); 
        
        printf("Se desbloquea proceso %s PID: %d \n", pais[contproceso], signalsem->lproceso[contproceso].pid);
        fflush(stdout);
        
        // Se pone en ejecucion proceso previamente bloqueado
        kill(signalsem->lproceso[contproceso].pid,SIGCONT);
    }

    *g=0;
    // SC Termina

}


void initsem(SEMAPHORE *initsem,  int valor)
{

    int l;

    l=1;
    do { atomic_xchg(l, *g); } while(l!=0);
    // SC Inicia
    // Se inicializa contador de semaforo
    initsem->contador = 1;
    *g=0;
    // SC Termina
}

void mete_a_cola(COLAPROC *q,int nproceso)
{
    q->cola[q->ent]=nproceso;
    q->ent++;
    if(q->ent>(N_PROCESS-1))
       q->ent=0;
}

int sacar_de_cola(COLAPROC *q)
{
    int sproceso;
    
    sproceso=q->cola[q->sal];
    q->sal++;
    if(q->sal>(N_PROCESS-1))
        q->sal=0;
    return(sproceso);
}

