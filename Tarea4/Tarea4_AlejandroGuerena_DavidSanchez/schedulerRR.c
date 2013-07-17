#include "virtual_processor.h"
#include <signal.h>

extern struct PROCESO proceso[];
extern struct COLAPROC listos,bloqueados;
extern int tiempo;
extern int pars[];
// =============================================================================
// ESTE ES EL SCHEDULER
// ============================================================================

int scheduler(int evento)
{
    int cambia_proceso =0;
    int prox_proceso_a_ejecutar;

    prox_proceso_a_ejecutar=pars[1]; // pars[1] = proceso en ejecución

    if(evento==PROCESO_NUEVO) // jds: LLega nvo. proc pero evalua si exec o hold
    {
        printf("EV:Llegada de Proceso Nuevo!!!\n");
        proceso[pars[0]].estado=LISTO;
        mete_a_cola(&listos,pars[0]);


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
    }
    if(evento==TERMINA_E_S)
    {
        printf("EV:Termina E/S!\n");
        // Saber cual proceso terminó E/S
        // pars0 es el proceso desbloqueado
        proceso[pars[0]].estado=LISTO;
	mete_a_cola(&listos,pars[0]);
        printf("Termina E/S Proceso desbloqueado %d\n",pars[0]);
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
        if(!cola_vacia(listos)) // jds Hay procesos en cola de listos? 
	  {
	    if (proceso[pars[1]].estado==EJECUCION) {
	      proceso[pars[1]].estado=LISTO;
	      mete_a_cola(&listos,pars[1]);
	      kill(proceso[pars[1]].pid,SIGSTOP);
	    }
            prox_proceso_a_ejecutar=sacar_de_cola(&listos);
	    proceso[prox_proceso_a_ejecutar].estado=EJECUCION;
	    
	  }
        else if (proceso[pars[1]].estado==EJECUCION)
        {
            printf("No hay procesos listos en alguna de las cola, pero hay alguno ejecutandose\n");
            prox_proceso_a_ejecutar=pars[1];
        } else 
          // No hay procesos para ejecutar
	  prox_proceso_a_ejecutar=NINGUNO;
    }
    return(prox_proceso_a_ejecutar);
}

// =================================================================
