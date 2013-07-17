#include "virtual_processor.h"
#include <signal.h>
#include <stdio.h>

#define MAXCOL 10
#define EXTRAALTA 0
#define ALTA 1
#define NORMAL 2
#define BAJA 3

extern struct PROCESO proceso[];
extern struct COLAPROC listos ,bloqueados;
extern int tiempo;
extern int pars[];
// =============================================================================
// ESTE ES EL SCHEDULER
// ============================================================================
// Determina el nivel actual de cola
int nivelcola[MAXPROC];
struct COLAPROC listos2[MAXCOL];

void meter_a_cola_prioridad(int, int);
int sacar_de_cola_prioridad();
void disminuir_nivel_cola(int);
int checar_colas();

int scheduler(int evento)
{
    int cambia_proceso =0;
    int prox_proceso_a_ejecutar;

    prox_proceso_a_ejecutar=pars[1]; // pars[1] = proceso en ejecución

    if(evento==PROCESO_NUEVO) // jds: LLega nvo. proc pero evalua si exec o hold
    {
        printf("EV:Llegada de Proceso Nuevo!!!\n");
        proceso[pars[0]].estado=LISTO;
        meter_a_cola_prioridad(pars[0], 1);
    }      

    if(evento==TIMER)
    {
        printf("EV:Timer!\n");
        printf("Llega interrupcion del Timer\n");
	cambia_proceso=1;
    }
                                 
    if(evento==SOLICITA_E_S)
    {
        printf("EV:Solicita E/S!\n");
        proceso[pars[1]].estado=BLOQUEADO;
        printf("Solicita E/S Proceso %d\n",pars[1]);
        //cambia_proceso=1; // Indíca que puede poner un proceso nuevo en ejecucion
    }
    if(evento==TERMINA_E_S)
    {
        printf("EV:Termina E/S!\n");
        printf("Termina E/S Proceso desbloqueado %d\n",pars[0]);
        // Saber cual proceso terminó E/S
        // pars0 es el proceso desbloqueado
        proceso[pars[0]].estado=LISTO;
	meter_a_cola_prioridad(pars[0], 1);
        
    }
    if(evento==PROCESO_TERMINADO)
    {
        printf("EV:Proceso Terminado!\n");
        // pars0 = proceso terminado
        proceso[pars[0]].estado=TERMINADO;
        //cambia_proceso=1; // Indíca que puede poner un proceso nuevo en ejecucion
    }
                                        
    if(cambia_proceso)
    {
        printf("FL:Cambia Proceso!!!\n");
        // Si la cola no esta vacia obtener de la cola el siguiente proceso listo
        if(checar_colas()) // jds Hay minimo un proceso en las colas de listos? 
	  {
	    if (proceso[pars[1]].estado==EJECUCION) {
	      proceso[pars[1]].estado=LISTO;
	      meter_a_cola_prioridad(pars[1], 0);
	      kill(proceso[pars[1]].pid,SIGSTOP);
	    }
            prox_proceso_a_ejecutar=sacar_de_cola_prioridad();
	    proceso[prox_proceso_a_ejecutar].estado=EJECUCION;
            disminuir_nivel_cola(prox_proceso_a_ejecutar); // Disminuir su nivel de cola 
	    
	  }
        else if (proceso[pars[1]].estado==EJECUCION)
        {
            printf("No hay procesos listos en alguna de las cola, pero hay alguno ejecutandose\n");
            prox_proceso_a_ejecutar=pars[1];         
            disminuir_nivel_cola(pars[1]); // Disminuir su nivel de cola  
        } else 
            // No hay procesos para ejecutar
	    prox_proceso_a_ejecutar=NINGUNO;
    }
    return(prox_proceso_a_ejecutar);
}

// =================================================================

int checar_colas()
{

    int hay_cola_lista = 0;
    int i;    

    // Verificar que las colas no esten vacias
    for(i=0;i<MAXCOL;i++){
        hay_cola_lista |= (!cola_vacia(listos2[i]));
        //printf("Cola %d: %d\n", i, (!cola_vacia(listos2[i]))); 
    }

    if(hay_cola_lista == 0)
        printf("No hay procesos listos en las colas\n");

    return hay_cola_lista;
    
}

void meter_a_cola_prioridad(int procnumber, int nivelalto)
{

    //Para meter a la cola de prioridad mas alta que le corresponde a su nivel
    if(nivelalto == 1) {
        switch(proceso[procnumber].prioridad)
        {
            case EXTRAALTA:
                nivelcola[procnumber] = 0;               
                break; 
            case ALTA:
                nivelcola[procnumber] = 3;
                break;
            case NORMAL:
                nivelcola[procnumber] = 5;
                break;
            case BAJA:
                nivelcola[procnumber] = 7;
                break;
            default: 
                printf("Error: %d es una Prioridad Invalida\n", proceso[procnumber].prioridad);
                exit(1);
                break;
        }
    }

    //Mete a cola apropiada
    printf("Mete a cola proceso: %d a la cola numero: %d\n", procnumber, nivelcola[procnumber]);
    mete_a_cola(&listos2[nivelcola[procnumber]], procnumber);    
}


int sacar_de_cola_prioridad()
{

    int i; 
    //checar cual es la cola de mayor prioridad con procesos listos
    for(i=0;i<MAXCOL;i++){
        if(cola_vacia(listos2[i]) == 0)
            break; 
    }
    //printf("Saca elemento de la cola %d proceso %d\n", i, sacar_de_cola(&listos2[i]));
    printf("Saca elemento de la cola %d \n", i);
    return sacar_de_cola(&listos2[i]);
}

void disminuir_nivel_cola(int procnumber)
{
    
    if(proceso[procnumber].prioridad == EXTRAALTA){
        if(nivelcola[procnumber] < 2)
            nivelcola[procnumber]++;    
    }    
    else if(proceso[procnumber].prioridad == ALTA){
        if(nivelcola[procnumber] < 5)
            nivelcola[procnumber]++;
    }
    else if(proceso[procnumber].prioridad == NORMAL){
        if(nivelcola[procnumber] < 7)
            nivelcola[procnumber]++;
    }
    else{
        if(nivelcola[procnumber] < 9)
            nivelcola[procnumber]++;
    }

}


