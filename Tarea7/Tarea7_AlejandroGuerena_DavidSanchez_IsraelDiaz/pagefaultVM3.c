#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "mmu.h"

#define RESIDENTSETSIZE 24

extern char *base;
extern int framesbegin;
extern int idproc;
extern int systemframetablesize;
extern int processpagetablesize;

extern struct SYSTEMFRAMETABLE *systemframetable;
extern struct PROCESSPAGETABLE processpagetable[];

void write2binfile(int framenumber, int pagina);
void readbinfile(int framenumber);
void printtimes();
int paginaexpulsar();
int getfreevirtualframe();
int getfreeframe();
int checkfreeframe();

// Rutina de fallos de página
// Checa si hay memoria fisica disponible, para conjunto residente arriba de 2

int pagefault(char *vaddress)
{

    const int conjunto_residente = 3;
    int i;
    int frame, framevirtual;
    int pag_a_expulsar;
    int pag_del_proceso;

    // Calcula la página del proceso
    pag_del_proceso=(int) vaddress>>12;
    // Cuenta los marcos asignados al proceso
    i=countframesassigned();
   
   
    // Determina si el proceso ha alcanzado su conjunto residente de marcos
    printf("Conjunto residente : %d, getfreeframe = %d\n", i, checkfreeframe());
    // Es menor el numero de marcos asignados al proceso que su conjunto residente?
    if( (i < conjunto_residente) && (checkfreeframe() != (-1)))
    {    
        printf("------------------- Numero de Marcos asignados del proceso %d es MENOR al conjunto residente, asignarle marco a pagina : %d\n", idproc, pag_del_proceso);
        // Busca un marco libre en el sistema
        frame=getfreeframe();
        
        if(frame==-1)
        {
            printf("------------------- ERROR FATAL, Memoria fisica insuficiente\n");
            return(-1); // Regresar indicando error de memoria insuficiente
        }

        // Actualiza campos de pagina asignada a memoria fisica
        processpagetable[pag_del_proceso].presente=1;
        processpagetable[pag_del_proceso].framenumber=frame;

        // Escribir pagina en archivo swap
        write2binfile(frame, pag_del_proceso);
        //readbinfile(frame);
        
        return(1); // Regresar todo bien  
    }
    // Es menor el numero de marcos asignados al proceso que su conjunto residente?
    else if( (i == conjunto_residente) || ( (i < conjunto_residente) && (checkfreeframe() == -1) ) )
    {


        // Determinar pagina a expulsar
        pag_a_expulsar = paginaexpulsar();
        if(pag_a_expulsar == -1){
            printf("------------------- ERROR FATAL, Marcos virtuales no disponibles para el proceso %d\n", idproc);
            return -1;    
        }

        if(i == conjunto_residente)
            printf("------------------- Numero de Marcos asignados del proceso %d es IGUAL al conjunto residente, asignarle marco a pagina : %d y expulsar pagina: %d\n", idproc, pag_del_proceso, pag_a_expulsar);
        else
            printf("------------------- Numero de Marcos asignados del proceso %d es MENOR al conjunto residente, pero hay que asignarle marco a pagina : %d y expulsar pagina: %d\n", idproc, pag_del_proceso, pag_a_expulsar); 

        // Pagina expulsada es escrita en disco si esta modificada
        if(processpagetable[pag_a_expulsar].modificado){
            processpagetable[pag_a_expulsar].modificado = 0; 
            write2binfile(processpagetable[pag_a_expulsar].framenumber, pag_a_expulsar);    
        }

        // Se realiza swapping 
        // Pagina que genera el fallo, esta en memoria virtual?
      
        // Esta en memoria virtual
        if(processpagetable[pag_del_proceso].framenumber != -1) 
        {
            // Se obtiene el marco fisico y el virtual 
            frame = processpagetable[pag_a_expulsar].framenumber;
            framevirtual = processpagetable[pag_del_proceso].framenumber;         
            // Se asignan paginas al marco fisico y virtual
            systemframetable[frame].assigned=1;  // Este ya tiene previamente asignado 1, bit assigned = 1
            systemframetable[framevirtual].assigned=1; // Este ya tiene previamente asignado 1, bit assigned = 1 
        }
        // No esta en memoria virtual 
        else
        {
            // Se obtiene el marco fisico y el virtual 
            frame = processpagetable[pag_a_expulsar].framenumber;
            framevirtual = getfreevirtualframe();
            
            if(framevirtual == -1)
            {
                printf("------------------- ERROR FATAL, Memoria virtual insuficiente\n");
                return(-1); // Regresar indicando error de memoria virtual insuficiente              
            }
            
            // Se asignan paginas al marco fisico y virtual
            systemframetable[frame].assigned=1;  // Este ya tiene previamente asignado 1, bit assigned = 1
            systemframetable[framevirtual].assigned=1;           
        }

        // Actualiza campos de pagina asignada a memoria fisica
        processpagetable[pag_del_proceso].presente=1;
        processpagetable[pag_del_proceso].framenumber=frame;

        // Actualiza campos de pagina expulsada y asignada con marco virtual
        processpagetable[pag_a_expulsar].presente=0;
        processpagetable[pag_a_expulsar].framenumber=framevirtual;     

        // Se escriben en archivo swap
        write2binfile(frame, pag_del_proceso);
        write2binfile(framevirtual, pag_a_expulsar);

        return(1);
    }

    //Es mayor el numero de marcos asignados al proceso que su conjunto residente?
    else
    {
        printf("------------------- Numero de Marcos asignados del proceso %d es MAYOR al conjunto residente, ERROR FATAL\n", idproc);
        return -1;    
    }

    return 1; // Default Return
}


void write2binfile(int framenumber, int pagina)
{

    FILE *f;
    int offset = framenumber - framesbegin;    

    // Open swap file for read
    f=fopen("swap","w");    
    
    // Point to the correct frame
    fseek(f,sizeof(processpagetable[pagina])*offset,SEEK_SET);    

    // Write on swap file  
    fwrite(&processpagetable[pagina],sizeof(processpagetable[pagina]),1,f);       

    // Close swap file
    fclose(f); 

}



void readbinfile(int framenumber)
{

    struct PROCESSPAGETABLE s_processpagetable;
    FILE *f;
    int offset = framenumber - framesbegin;    

/*
    // Open swap file for read
    f=fopen("swap","r");    
    
    // Point to the correct frame
    fseek(f,sizeof(systemframetable[framenumber])*offset,SEEK_SET);
    
    // Read from swap file  
    fread(&systemframetable[framenumber],sizeof(systemframetable[framenumber]),1,f);       

    printf("Valor leido del frame virtual : %X Assigned : %d pAdders : %p  \n", framenumber, systemframetable[framenumber].assigned, systemframetable[framenumber].paddress);

    // Close swap file
    fclose(f);  
*/
// -----------------------------------

    // Open swap file for read
    f=fopen("swap","r");    
    
    // Point to the correct frame
    fseek(f,sizeof(s_processpagetable)*offset,SEEK_SET);
    
    // Read from swap file  
    fread(&s_processpagetable,sizeof(s_processpagetable),1,f);       

    printf("Valor leido del frame virtual: %X %X Presente : %d Modificado : %d\n", framenumber, s_processpagetable.framenumber, s_processpagetable.presente, s_processpagetable.modificado);

    // Close swap file
    fclose(f);  

}


int getfreeframe()
{
    int i;
    // Busca un marco libre en el sistema (memoria fisica)
    for(i=framesbegin;i<systemframetablesize+framesbegin;i++)
        if(!systemframetable[i].assigned)
        {
            systemframetable[i].assigned=1;
            break;
        }
    if(i<systemframetablesize+framesbegin)
        systemframetable[i].assigned=1;
    else
        i=-1;
    return(i);
}


int checkfreeframe()
{

    int i;
    // Busca un marco libre en el sistema (memoria fisica)
    for(i=framesbegin;i<systemframetablesize+framesbegin;i++)
        if(!systemframetable[i].assigned)
        {
            return 1;
        }

   return -1;

}

int getfreevirtualframe()
{
    int i;
    // Busca un marco libre en el sistema (memoria virtual)
    for(i=systemframetablesize+framesbegin;i<((2*systemframetablesize)+framesbegin);i++)
        if(!systemframetable[i].assigned)
        {
            return i;
        }
    return -1;
}


int paginaexpulsar()
{
 
    int pag_a_expulsar = -1;
    int i,j;
    
    // LRU
    
    for(i=0;i<3;i++){
        for(j=i+1;j<4;j++){
            if(processpagetable[i].presente && processpagetable[j].presente){       
                if(processpagetable[i].tlastaccess < processpagetable[j].tlastaccess)  
                    pag_a_expulsar = i;
                else
                    pag_a_expulsar = j;                     
            }
        }
    }

    // FIFO
    /*
    for(i=0;i<3;i++){
        for(j=i+1;j<4;j++){
            if(processpagetable[i].presente && processpagetable[j].presente){       
                if(processpagetable[i].tarrived < processpagetable[j].tarrived)  
                    pag_a_expulsar = i;
                else
                    pag_a_expulsar = j;                     
            }
        }
    }
    */
    return pag_a_expulsar;
}



void printtimes()
{
    printf("------------------- Proceso %d\n", idproc);
    printf("-----Proceso %d Pagina 0 tarrived: %lu tlastaccess: %lu\n", idproc, processpagetable[0].tarrived, processpagetable[0].tlastaccess);
    printf("-----Proceso %d Pagina 1 tarrived: %lu tlastaccess: %lu\n", idproc, processpagetable[1].tarrived, processpagetable[1].tlastaccess);  
    printf("-----Proceso %d Pagina 2 tarrived: %lu tlastaccess: %lu\n", idproc, processpagetable[2].tarrived, processpagetable[2].tlastaccess);  
    printf("-----Proceso %d Pagina 3 tarrived: %lu tlastaccess: %lu\n", idproc, processpagetable[3].tarrived, processpagetable[3].tlastaccess);
}


  

