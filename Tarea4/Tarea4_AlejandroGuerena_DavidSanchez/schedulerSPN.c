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
    int elmejor;

    prox_proceso_a_ejecutar=pars[1]; // pars[1] = proceso en ejecución

    if(evento==PROCESO_NUEVO) // jds: LLega nvo. proc pero evalua si exec o hold
    {
        printf("EV:Llegada de Proceso Nuevo!!!\n");
        proceso[pars[0]].estado=LISTO;
    }      

    if(evento==TIMER)
    {
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
        // Saber cual proceso terminó E/S
        // pars0 es el proceso desbloqueado
        proceso[pars[0]].estado=LISTO;
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
	elmejor = mejor_proceso_listo(); // obtiene el de menos t ejecucion listo
	if (elmejor != -1) {
	  if( (pars[1]== NINGUNO)|| (proceso[pars[1]].estado==BLOQUEADO) || (proceso[pars[1]].estado==TERMINADO) ) { 
	    if (proceso[elmejor].trestante > 0 ) {
	      proceso[elmejor].trestante--;
	    }
	    prox_proceso_a_ejecutar=elmejor;
	    proceso[prox_proceso_a_ejecutar].estado=EJECUCION;
	  }
          // Hay en ejecucion y compara quien tiene menor t. ejecucion
	  else if (proceso[elmejor].trestante < proceso[pars[1]].trestante) {
	    prox_proceso_a_ejecutar=elmejor;
	    proceso[prox_proceso_a_ejecutar].estado=EJECUCION;
	    if (proceso[elmejor].trestante > 0 ) {
	      proceso[elmejor].trestante--;
	    }
	    proceso[pars[1]].estado=LISTO;
	    kill(proceso[pars[1]].pid,SIGSTOP);
	  }
          // El de menor t. ejecucion es el que se esta ejecutando
	  else {
	    if (proceso[pars[1]].trestante > 0 ) {
	      proceso[pars[1]].trestante--;
	    }
	  
	  }
	  
	} else {
          // No hay procesos listos, pero hay uno en ejecucion
	  if (proceso[pars[1]].estado==EJECUCION) {
	    if (proceso[pars[1]].trestante > 0 ) {
	      proceso[pars[1]].trestante--;
	    }
	  } 
          // No hay procesos para ejecutar
          else {
	    prox_proceso_a_ejecutar=NINGUNO;
	  }
	}
	
        
      }
    return(prox_proceso_a_ejecutar);
}

// =================================================================

int mejor_proceso_listo()
{
    int i, hayproceso;
    int proceso_a_ejecutar;

    hayproceso = 0;
    proceso_a_ejecutar = 0;

    for(i=0;i<MAXPROC;i++)
    {
        if((proceso[i].estado==LISTO) && (hayproceso == 0))
        {
            hayproceso = 1;
            proceso_a_ejecutar = i;
        }
        else if((proceso[i].estado==LISTO) && (hayproceso == 1))
        {
            // Si el tiempo es igual o menor al proceso_a_ejecutar
            if(!(proceso[i].trestante > proceso[proceso_a_ejecutar].trestante))
            {
                // Es menor
                if(proceso[i].trestante < proceso[proceso_a_ejecutar].trestante)
                {
                    proceso_a_ejecutar = i;                  
                }  
                // Es igual
                else
                {
                    // Checar tiempos de llegada (El de menor tiempo de inicio tiene prioridad)
                    if(proceso[i].tinicio < proceso[proceso_a_ejecutar].tinicio)
                    {
                        proceso_a_ejecutar = i;
                    }
                }
            }     
        }
    }
   
    if(hayproceso == 0)
        // No hay procesos listos para ejecucion
        return(-1);
    else
        return(proceso_a_ejecutar);
}
