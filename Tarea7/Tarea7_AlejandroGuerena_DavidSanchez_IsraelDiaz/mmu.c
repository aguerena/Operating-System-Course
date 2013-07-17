#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include "semaphores.h"
#include "mmu.h"

#define VERSION "mmu versión 10.0326.00\n"

#define PAGESIZE 4096
#define PHISICALMEMORYSIZE 32*1024
#define SYSTEMFRAMETABLESIZE PHISICALMEMORYSIZE/PAGESIZE
#define TOTFRAMES SYSTEMFRAMETABLESIZE
#define MAXPROC 4
#define PROCESSPAGETABLESIZE 2*SYSTEMFRAMETABLESIZE/MAXPROC 
#define TABLESSIZE 2*SYSTEMFRAMETABLESIZE*sizeof(struct SYSTEMFRAMETABLE) 



struct SYSTEMFRAMETABLE *systemframetable;
struct PROCESSPAGETABLE processpagetable[PROCESSPAGETABLESIZE]; 

int systemframetablesize = SYSTEMFRAMETABLESIZE;
int processpagetablesize = PROCESSPAGETABLESIZE;
int framesbegin;
int framesend;
int idproc;
char *base = NULL;
long starttime;
int totalpagefaults=0;
int debugmode=0;
int exmut;

// -----------------------------------------
// < ----- Redefinición de funciones ----- >
// -----------------------------------------

void *getbaseaddr();
void initprocesspagetable();
void freeprocessmem();
unsigned long thisinstant();
void settimer();
void exiterror();

// -------------------------------
// < ----- Signal Handlers ----- >
// -------------------------------
void detachallpages(int sig)
{
    // Desconecta la memoria compartida
    int i;
    char *ptr;

    for(i=0;i<PROCESSPAGETABLESIZE;i++)
    {
        if(processpagetable[i].presente  && processpagetable[i].attached)
        {
            processpagetable[i].attached=0;
            ptr=base+i*PAGESIZE;
            if(shmdt(ptr) == -1)
                fprintf(stderr,"Detachallpages, Proceso %d, Error en el shmdt %p, página=%i\n",idproc,ptr,i);
        }
    }
    return;
}

void seg_handler(int sig,siginfo_t *sip,void *notused)
{
    int i;
    int pag_del_proceso;
    char *vaddress;
    int flags;
    int result;
    char *ptr,*page_ptr;
    

    ptr=sip->si_addr;
    vaddress=(char *)((int) sip->si_addr - (int) base);
    pag_del_proceso=(int) vaddress / PAGESIZE;

    if(pag_del_proceso>=PROCESSPAGETABLESIZE)
    {
        fprintf(stderr,"Error: dirección fuera del espacio asignado al proceso\n");
        exiterror();
    }


    if(processpagetable[pag_del_proceso].modificado)
        flags=SHM_RND;
    else  
        flags=SHM_RND | SHM_RDONLY;

    // Si la página está presente y conetcada, trataron de modificarla
    // Poner el bit de modificado en 1
    if(processpagetable[pag_del_proceso].presente && 
       processpagetable[pag_del_proceso].attached)
    {
        if(debugmode)
        {
            printf("---------------------------\nPage fault handler\n");
            printf("Página modificada de la dirección=%X\n",vaddress);
            printf("Proceso=%d Página=%d\n",idproc,pag_del_proceso);
        }
        processpagetable[pag_del_proceso].modificado=1;
        page_ptr=base+pag_del_proceso*PAGESIZE;
        shmdt(page_ptr);
        flags=SHM_RND;
    }

   
    // Poner la marca de tiempo
    processpagetable[pag_del_proceso].tlastaccess=thisinstant();

    // Fallo de página cuando la página no está presente
    if(! processpagetable[pag_del_proceso].presente)
    {

        if(debugmode)
        {
            printf("---------------------------\nPage fault handler\n");
            printf("Direccion que provocó el fallo=%X\n",(int) vaddress);
            printf("Proceso=%d Página=%d\n",idproc,pag_del_proceso);
        }
        
        // Establece el tiempo de llegada de la página
        processpagetable[pag_del_proceso].tarrived=processpagetable[pag_del_proceso].tlastaccess;
        // Cuenta los fallos de página por proceso
        totalpagefaults++;
        // Llama a la rutina de fallos de página
        semaphore_wait(exmut);
        result=pagefault(vaddress);
        semaphore_signal(exmut);
        if(result==-1)
        {
            fprintf(stderr,"ERROR, no hay memoria disponible para el proceso %d, proceso abortado.\n",idproc);
            exiterror();
        }

        if(debugmode)
        {
            printf("Proceso=%d, Página %d cargada en el marco %d\n",idproc,pag_del_proceso,processpagetable[pag_del_proceso].framenumber);
            printf("Tabla de páginas Proc -> Pag Pr Mo Fr\n");
            for(i=0;i<PROCESSPAGETABLESIZE;i++)
                printf("                    %d ->   %d  %d  %d %2X\n",idproc,
                                  i,
                                  processpagetable[i].presente,
                                  processpagetable[i].modificado,
                                  processpagetable[i].framenumber);
        }
    }

    // Liberar los marcos que no están asigados a páginas
    for(i=0;i<PROCESSPAGETABLESIZE;i++)
        if(!processpagetable[i].presente && processpagetable[i].attached)
        {
            if(debugmode)
                printf("Proceso %d, expulsa página %d\n",idproc,i);
            processpagetable[i].modificado=0;
            processpagetable[i].attached=0;
            if(shmdt(base + i*PAGESIZE)==-1)
            {
                fprintf(stderr,"Error en el shmdt");
            }
        }

    //  Mapear la página con la memoria compartida
    if(processpagetable[pag_del_proceso].presente)
    {
        processpagetable[pag_del_proceso].attached=1;

        if ((ptr=shmat(systemframetable[processpagetable[pag_del_proceso].framenumber].shmidframe, ptr, flags)) ==NULL)
        {
            fprintf(stderr,"Error al conectarse con memoria compartida\n");
            exiterror();
        }  

    }

    return;
}


void bus_handler(int i)
{
    printf("bus error handler\n");
}

// -----------------------------------
// < ----- Inicio del programa ----- >
// -----------------------------------

main(int argc, char *argv[])
{
    int i,x;
    int statval;
    void *tablesarea = (void *)0;
    void *framesptr = (void *)0;
    void *thisframeptr = (void *)0;
    long endtime,tottime;
    int shmidtables,shmidframes;
    struct shmid_ds shmbuf;
    struct timeval ts;
    struct sigaction act;

    if(argc>2)
    {
        fprintf(stderr,"Error en los argumentos\nUso: procesos [/debug|/version]\n");
        exit(1);
    }
    if(argc==2)
        if(strcmp(argv[1],"/debug")==0)
            debugmode=1;
        else if (strcmp(argv[1],"/version")==0)
        {
            printf(VERSION);
            exit(0);
        }
        else
        {
            fprintf(stderr,"Error en los argumentos\nUso: procesos [/debug|/version]\n");
            exit(1);
        }        

    // Toma el tiempo de inicio
    gettimeofday(&ts,NULL);
    starttime=ts.tv_sec*1000000+ts.tv_usec;

    // Establecer handler para fallos de página
    act.sa_sigaction=seg_handler;
    sigaddset(&act.sa_mask,SIGSEGV);
    sigaddset(&act.sa_mask,SIGALRM);
    act.sa_flags=SA_SIGINFO;
    sigaction(SIGSEGV,&act,0);

    // Establecer el handler para el timer
    act.sa_handler=detachallpages;
    sigaddset(&act.sa_mask,SIGSEGV);
    sigaddset(&act.sa_mask,SIGALRM);
    sigaction(SIGALRM,&act,0);

    act.sa_handler=bus_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGBUS,&act,0);

    // Establecer apuntadores
    tablesarea=getbaseaddr();
    if(debugmode)
        printf("Tables area = %p\n",tablesarea);
    framesptr=tablesarea+PAGESIZE;
    if(debugmode)
        printf("Frames area = %p\n",framesptr);
    base=framesptr+PHISICALMEMORYSIZE;
    if(debugmode)
        printf("Base = %p\n",base);

    // Poner tablas en memoria compartida
    shmidtables=shmget((key_t) 12345,TABLESSIZE,0666|IPC_CREAT);
    if(shmidtables==-1)
    {
        fprintf(stderr,"Error en el shmget\n");
        exit(EXIT_FAILURE);
    }
    tablesarea=shmat(shmidtables,tablesarea,0);
    if(tablesarea==(void *)-1)
    {
        fprintf(stderr,"Error en el shmat\n");
        exit(EXIT_FAILURE);
    }

    framesbegin=1+((int) tablesarea/PAGESIZE);
    framesend=framesbegin+SYSTEMFRAMETABLESIZE;
    printf("Primer marco %X\n",framesbegin);

    systemframetable=(struct SYSTEMFRAMETABLE *) tablesarea-framesbegin;


    // Crea la tabla de marcos disponibles del sistema
    for(i=framesbegin;i<framesend;i++)
    {
        if ((shmidframes = shmget(2234+i, PAGESIZE, SHM_RND | IPC_CREAT | 0777)) <0)
        {
            printf("Error could not create shared memory\n");
            exit(1);
        }
        systemframetable[i].assigned=0;
        systemframetable[i].shmidframe=shmidframes;
        thisframeptr=framesptr+PAGESIZE*(i-framesbegin);
        thisframeptr=shmat(systemframetable[i].shmidframe,thisframeptr,SHM_RND);
        if(thisframeptr==NULL)
        {
            fprintf(stderr,"Error en el shmat\n");
            exit(EXIT_FAILURE);
        }

        systemframetable[i].paddress=(char *) thisframeptr;
        if(debugmode)
            printf("Frame %X, Dirección %p %p \n",i,thisframeptr,systemframetable[i].paddress);
    }
    // Para los marcos virtuales
    for(i=framesbegin;i<framesend;i++)
        systemframetable[SYSTEMFRAMETABLESIZE+i].assigned=0;

    // Un semaforo por si las moscas
    exmut=semget((key_t) 3456,1,0600|IPC_CREAT);
    set_semvalue(1,exmut);

    // Crea los procesos
    for(idproc=0;idproc<MAXPROC;idproc++)
    {
        if(fork()==0)
        {
            settimer();
            
            // base=getbaseaddr();
            
            initprocesspagetable();

            switch(idproc)
            {
                case 0:
                    proc0();
                    break;
                case 1:
                    proc1();
                    break;
                case 2:
                    proc2();
                    break;
                case 3:
                    proc3();
                    break; 
            }


            freeprocessmem();
            printf("Termina proceso %d, Total de fallos de página = %d\n",idproc,totalpagefaults);
            exit(0);
        }
     
    }

    // Espera a que terminen los procesos
    for(i=0;i<MAXPROC;i++)
        wait(&statval);

    gettimeofday(&ts,NULL);
    endtime=ts.tv_sec*1000000+ts.tv_usec;
    tottime=endtime-starttime;
    
    printf("Tiempo total de ejecución %1.6f\n",(float) tottime/1000000);

    // Eliminar memoria compartida
    for(i=framesbegin;i<framesbegin+SYSTEMFRAMETABLESIZE;i++)
    {
        if(shmctl(systemframetable[i].shmidframe,IPC_RMID,&shmbuf)==-1)
            fprintf(stderr,"Error al eliminar frame # %d",i);
    } 

    if(shmdt(tablesarea)==-1)
        fprintf(stderr,"Error en el shmdt final\n");
    if(shmctl(shmidtables,IPC_RMID,&shmbuf)==-1)
        fprintf(stderr,"Error al eliminar memoria compartida\n");

    // Eliminar el semáforo
    del_semvalue(exmut);

}

// -------------------------
// < ----- Funciones ----- >
// -------------------------

   
void *getbaseaddr()
{
    void *ptr;
    // get the pointer to the end
    ptr = sbrk(0);

    // round it to page boundary

    ptr = (char *) (((unsigned long)ptr) + (((unsigned long)SHMLBA) - (((unsigned long)ptr) % ((unsigned long)SHMLBA))));
    return(ptr);
}

int countframesassigned()
{
    int i,j=0;
    //  Cuenta los marcos asignados al proceso
    if(debugmode)
        printf("Páginas del proceso -->");
    for(i=0;i<PROCESSPAGETABLESIZE;i++)
        if(processpagetable[i].presente)
        {
            if(debugmode)
                printf(" %d ",i);
            j++;
        }
    if(debugmode)
        printf("\n"); 
    return(j);
}


void initprocesspagetable()
{
    int i;
    for(i=0;i<PROCESSPAGETABLESIZE;i++)
    {
        processpagetable[i].presente=0;
        processpagetable[i].framenumber=NINGUNO;
        processpagetable[i].modificado=0;
        processpagetable[i].attached=0;
    }
    return;
}


void freeprocessmem()
{   
    int i;
    char *ptr;
    struct itimerval itimer, otimer;

    // Detiene el timer
    itimer.it_interval.tv_sec=0;
    itimer.it_interval.tv_usec=0;
    itimer.it_value.tv_sec=0;
    itimer.it_value.tv_usec=0;

    // Libera los marcos de la memoria
    if(setitimer(ITIMER_REAL,&itimer,&otimer)<0)
    {
        fprintf(stderr,"Error en el settimer");
        exit(1);
    }

    
    if(debugmode)
        printf("Proceso %d, libera los marcos -->",idproc);

    for(i=0;i<PROCESSPAGETABLESIZE;i++)
    {
        if(processpagetable[i].presente)
        {
            processpagetable[i].presente=0;
            systemframetable[processpagetable[i].framenumber].assigned=0;
            if(debugmode)
                printf(" %X ",processpagetable[i].framenumber);

            if(processpagetable[i].attached)
            {
                processpagetable[i].attached=0;
                ptr=base+i*PAGESIZE;
                if(shmdt(ptr) == -1)
                    fprintf(stderr,"Error en el shmdt %p, página=%i\n",ptr,i);
            }
        }
    }
    if(debugmode)
        printf("\n");
    return;
}


unsigned long thisinstant()
{
    struct timeval ts;
    gettimeofday(&ts,NULL);
    return(starttime-ts.tv_sec*1000000+ts.tv_usec);
}

void settimer()
{
    struct itimerval itimer, otimer;

    // Inicia el timer
    itimer.it_interval.tv_sec=0;
    itimer.it_interval.tv_usec=10000;
    itimer.it_value.tv_sec=0;
    itimer.it_value.tv_usec=10000;

    if(setitimer(ITIMER_REAL,&itimer,&otimer)<0)
    {
        fprintf(stderr,"Error en el settimer");
        exit(1);
    } 

    return;
}

void exiterror()
{
    freeprocessmem();
    exit(1);
}
