#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "tiposdatos.h"
#include "lowlvldisk.h"

int secboot_en_memoria = 0;
int blocksmap_en_memoria = 0; 
int inodesmap_en_memoria = 0;
int nodos_i_en_memoria = 0;

unsigned char blocksmap[2048];
char inodesmap[512];
int mapa_bits_bloques;
int mapa_bits_nodos_i;
int inicio_area_datos;
int inicio_nodos_i;

struct SECBOOT secboot;
struct INODE inode[8];

// Tabla de archivos abiertos
int openfiles_inicializada=0;
struct OPENFILES openfiles[16];


// ******************************************************************************
// Funciones para usar sectores logicos
// ******************************************************************************

// Convierte sector logico a parametros fisicos
void slogico2fisico(int *superficie, int *cilindro, int *sector, int sec_log)
{

    *sector = (sec_log % SECTORES_X_PISTA) + 1;
    *superficie = (sec_log / SECTORES_X_PISTA ) % NUM_SUPERFICIES;  
    *cilindro = (sec_log / (SECTORES_X_PISTA * NUM_SUPERFICIES) );

}


// Lectura con sector logico como parametro
int vdreadseclog(int sec_log, char *buffer)
{
    int ncyl,nhead,nsec;
    const int drive = 0;

    slogico2fisico(&nhead, &ncyl, &nsec, sec_log);    
    if(vdreadsector(drive,nhead,ncyl,nsec,1,buffer)==-1)
    {
        fprintf(stderr,"Error al leer de sector %d\n", sec_log);
        return -1;
    }

    return 0;
}

// Escritura con sector logico como parametro
int vdwriteseclog(int sec_log, char *buffer)
{

    int ncyl,nhead,nsec;
    const int drive = 0;

    slogico2fisico(&nhead, &ncyl, &nsec, sec_log);    
    if(vdwritesector(drive,nhead,ncyl,nsec,1,buffer)==-1)
    {
        fprintf(stderr,"Error al escribir en sector %d\n", sec_log);
        return -1;
    }

    return 0;

}



// ******************************************************************************
// Funciones para el manejo de los mapas de bits (area de datos)
// Falta block -1 o no hace caso al primer bit?
// Se cambio lo de sector=(offset/512)*512 a sector=(offset/512); 
// ******************************************************************************

// Determina si esta libre un bloque de datos
int isblockfree(int block)
{
    int offset=block/8;  // Obtener en que byte 
    int shift=block%8;   // y bit, que indica si el bloque está libte
    int result;
    int i;

    // Es importante tenerlo en memoria porque ahí es donde tenemos
    // la información del disco, cuáles son los sectores donde hay qué
    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    // Calculo el sector donde está el mapa de bits para los bloques
    mapa_bits_bloques= secboot.sec_res+secboot.sec_mapa_bits_nodos_i;

        // ¿Está en memoria el mapa de bits de bloques?, si no, cargarlo a memoria
    if(!blocksmap_en_memoria)
    {
        // Leer todos los sectores del mapa de bits a memoria
        for(i=0;i<secboot.sec_mapa_bits_bloques;i++)
            result=vdreadseclog(mapa_bits_bloques+i,blocksmap+i*512);

        blocksmap_en_memoria=1;
    }

    
    if(blocksmap[offset] & (1<<shift))
        return(0);
    else
        return(1);
}    


// Buscar en el mapa de bits cuál es el siguiente bloque de datos libre
int nextfreeblock()
{
    int i,j;
    int result;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    mapa_bits_bloques = secboot.sec_res+secboot.sec_mapa_bits_nodos_i;

    if(!blocksmap_en_memoria)
    {
        for(i=0;i<secboot.sec_mapa_bits_bloques;i++)
            result=vdreadseclog(mapa_bits_bloques+i,blocksmap+i*512);
        blocksmap_en_memoria=1;
    } 

    // Buscar el primer byte en el mapa de bloques donde hay al menos un 
    // bloque libre
    i=0;


    while( blocksmap[i]==0x0FF && i<secboot.sec_mapa_bits_bloques*512)
        i++;

    // Si no llegamos al final
    if(i<secboot.sec_mapa_bits_bloques*512)
    {
        j=0;
        while(blocksmap[i] & (1<<j) && j<8)
            j++;

        return(i*8+j);  // Regresando cual es el primer bloque libre encontrado
    }
    else 
        return(-1);  // Llegamos al final del mapa y no encontramos un bloque libre
        
}


// Poner un bloque como no disponible
int assignblock(int block)
{
    int offset=block/8;
    int shift=block%8;
    int result;
    int i;
    int sector;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    mapa_bits_bloques= secboot.sec_res+secboot.sec_mapa_bits_nodos_i;

    if(!blocksmap_en_memoria)
    {
        for(i=0;i<secboot.sec_mapa_bits_bloques;i++)
            result=vdreadseclog(mapa_bits_bloques+i,blocksmap+i*512);
        blocksmap_en_memoria=1;
    } 

    // Poner en 1 el bit en el byte que corresponde al número de bloque
    blocksmap[offset]|=(1<<shift);

    // Escribir ese sector en el disco
    sector=(offset/512)*512;
    //sector=(offset/512); // Se quito el *512, porque no tiene sentido esa multiplicacion
        // 0 .. 511 = 0
        // 512 .. 1023 = 512
        // 1024 .. 1535 = 1024
        // offset=offset-(offset%512);
    vdwriteseclog(mapa_bits_bloques+sector,blocksmap+sector*512);

    return(1);
}

// Establecer el bloque como libre
int unassignblock(int block)
{
    int offset=block/8;
    int shift=block%8;
    int result;
    char mask;
    int sector;
    int i;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    mapa_bits_bloques= secboot.sec_res+secboot.sec_mapa_bits_nodos_i;

    if(!blocksmap_en_memoria)
    {
        for(i=0;i<secboot.sec_mapa_bits_bloques;i++)
             result=vdreadseclog(mapa_bits_bloques+i,blocksmap+i*512);
        blocksmap_en_memoria=1;
    }

    blocksmap[offset]&=(char) ~(1<<shift);

    sector=(offset/512)*512;
    //sector=(offset/512);
    vdwriteseclog(mapa_bits_bloques+sector,blocksmap+sector*512);

    return(1);
}

 
// *************************************************************************
// Funciones para manipular los mapas de bits de los nodos i
// *************************************************************************

// Determina si esta libre un nodo i
// Return Libre = 1, Ocupado = 0
int isinodefree(int inode)
{
    int offset=inode/8;
    int shift=inode%8;
    int result;

    // Checar si el sector de boot está en memoria
    if(!secboot_en_memoria)
    {
        // Si no está en memoria, cárgalo
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    //Usamos la información del sector de boot para determinar en que sector inicia el 
    // mapa de bits de nodos i 
    mapa_bits_nodos_i= secboot.sec_res;     
                    
    // Está el mapa está en memoria
    if(!inodesmap_en_memoria)
    {
        // Si no está en memoria, hay que leerlo del disco
        result=vdreadseclog(mapa_bits_nodos_i,inodesmap);
        inodesmap_en_memoria=1;
    }


    if(inodesmap[offset] & (1<<shift))
        return(0);
    else
        return(1);
}    

// Buscar en el mapa de bits cuál es el siguiente nodo i libre
int nextfreeinode()
{
    int i,j;
    int result;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }
    mapa_bits_nodos_i= secboot.sec_res;

    if(!inodesmap_en_memoria)
    {
        result=vdreadseclog(mapa_bits_nodos_i,inodesmap);
        inodesmap_en_memoria=1;
    }

    // Recorrer byte por byte mientras sea 0xFF sigo recorriendo
    i=0;
    while(inodesmap[i]==0xFF && i<secboot.sec_mapa_bits_nodos_i)
        i++;

    if(i<secboot.sec_mapa_bits_nodos_i)
    {
        j=0;
        while(inodesmap[i] & (1<<j) && j<8)
            j++;

        return(i*8+j);
    }
    else
        return(-1);

        
}

// Poner un nodo i como no disponible
int assigninode(int inode)
{
    int offset=inode/8;
    int shift=inode%8;
    int result;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    mapa_bits_nodos_i= secboot.sec_res;

    if(!inodesmap_en_memoria)
    {
        result=vdreadseclog(mapa_bits_nodos_i,inodesmap);
        inodesmap_en_memoria=1;
    }

    inodesmap[offset]|=(1<<shift);
    vdwriteseclog(mapa_bits_nodos_i,inodesmap);
    return(1);
}

// Establecer un nodo i como libre
int unassigninode(int inode)
{
    int offset=inode/8;
    int shift=inode%8;
    int result;
    char mask;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    mapa_bits_nodos_i= secboot.sec_res;

    if(!inodesmap_en_memoria)
    {
        result=vdreadseclog(mapa_bits_nodos_i,inodesmap);
        inodesmap_en_memoria=1;
    }


    inodesmap[offset]&=(char) ~(1<<shift);
    vdwriteseclog(mapa_bits_nodos_i,inodesmap);
    return(1);
}


// **********************************************************************************
// Lectura y escritura de bloques
// Nota: el block -1
// **********************************************************************************

int writeblock(int block,char *buffer)
{
    int result;
    int i;

    // Verificar si el sector de boot está en memoria y si no, cárgalo
    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }

    // Calcula cuál es el sector donde inician los bloques 
    inicio_area_datos=secboot.sec_res+secboot.sec_mapa_bits_nodos_i +secboot.sec_mapa_bits_bloques+secboot.sec_tabla_nodos_i;

    // Escribir todos los sectores lógicos que corresponden al bloque
    for(i=0;i<secboot.sec_x_bloque;i++)
        // Escribimos cada uno de los sectores que corresponden al bloque
        // buffer+512*i hacemos que cada iteración del for un subbuffer de 512
        // bytes.
        vdwriteseclog(inicio_area_datos+(block-1)*secboot.sec_x_bloque+i,buffer+512*i);

    return(1);    
}

int readblock(int block,char *buffer)
{
    int result;
    int i;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }
    inicio_area_datos=secboot.sec_res+secboot.sec_mapa_bits_nodos_i+secboot.sec_mapa_bits_bloques+secboot.sec_tabla_nodos_i;

    for(i=0;i<secboot.sec_x_bloque;i++)
        vdreadseclog(inicio_area_datos+(block-1)*secboot.sec_x_bloque+i,buffer+512*i);
    return(1);    
}




// ***********************************************************************************
// Funciones para el manejo de inodos
// Funciones para el empaquetamiento de fecha y hora en los archivos
// ***********************************************************************************

// Convierte la fecha que está en una estructura fecha a un entero de 32 bits
unsigned int datetoint(struct DATE date)
{
    unsigned int val=0;

    val=date.year-1970;
    val<<=4;
    val|=date.month;
    val<<=5;
    val|=date.day;
    val<<=5;
    val|=date.hour;
    val<<=6;
    val|=date.min;
    val<<=6;
    val|=date.sec;
    
    return(val);
}

// Extraer la fecha y hora que está empaquetada en un entero de 32 bits
// Almacena los resultados en una estructura
int inttodate(struct DATE *date,unsigned int val)
{
    date->sec=val&0x3F;
    val>>=6;
    date->min=val&0x3F;
    val>>=6;
    date->hour=val&0x1F;
    val>>=5;
    date->day=val&0x1F;
    val>>=5;
    date->month=val&0x0F;
    val>>=4;
    date->year=(val&0x3F) + 1970;
    return(1);
}

// Fecha y hora actual empaquetada en un entero de 32 bits
// Las funciones de creación y escritura de archivos usan esta función.
unsigned int currdatetimetoint()
{
    struct tm *tm_ptr;
    time_t the_time;
    
    struct DATE now;

    (void) time(&the_time);
    tm_ptr=gmtime(&the_time);

    now.year=tm_ptr->tm_year-70;
    now.month=tm_ptr->tm_mon+1;
    now.day=tm_ptr->tm_mday;
    now.hour=tm_ptr->tm_hour;
    now.min=tm_ptr->tm_min;
    now.sec=tm_ptr->tm_sec;
    return(datetoint(now));
}




// **********************************************************************
// Funciones básicas para la implementación del sistema de archivos
// **********************************************************************


// En un inodo específico escribe el nombre, atributos y usuario y dueño del nuevo archivo.
// Usada por la función vdcreate
int setninode(int num, char *filename,unsigned short atribs, int uid, int gid)
{
    int i;
    int result;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }
    inicio_nodos_i=secboot.sec_res+secboot.sec_mapa_bits_nodos_i+secboot.sec_mapa_bits_bloques;

    if(!nodos_i_en_memoria)
    {
        for(i=0;i<secboot.sec_tabla_nodos_i;i++)
            result=vdreadseclog(inicio_nodos_i+i,(char *) &inode[i*8]);

        nodos_i_en_memoria=1;
    }

    
    strncpy(inode[num].name,filename,20);

    if(strlen(inode[num].name)>19)
         inode[num].name[19]='\0';

    inode[num].datetimecreat=currdatetimetoint();
    inode[num].datetimemodif=currdatetimetoint();
    inode[num].uid=uid;
    inode[num].gid=gid;
    inode[num].perms=atribs;
    inode[num].size=0;
    
    for(i=0;i<10;i++)
        inode[num].blocks[i]=0;

    inode[num].indirect=0;
    inode[num].indirect2=0;

    // Optimizar la escritura escribiendo solo el sector lógico que
    // corresponde al inodo que estamos asignando.
    // i=num/8;
    // result=vdwriteseclog(inicio_nodos_i+i,&inode[i*8]);
    for(i=0;i<secboot.sec_tabla_nodos_i;i++)
        result=vdwriteseclog(inicio_nodos_i+i,(char *) &inode[i*8]);

    return(num);
}

// asigna nodo i, (si es que existe un libre), poniendolo como no disponible e escribe informacion a el
int assignsetinode(char *filename, unsigned short atribs, int uid, int gid)
{

    int result;
    int num;

    // Checar si existe el archivo o no
    // Buscar el nodo i correspondiente al nombre del archivo
    num = searchinode(filename);
    if(num != -1){
        // Buscar si el nodo i esta ocupado, inidicando que existe ya el archivo
        if(isinodefree(num) != 1)
        {   
            printf("Ya existe un archivo con ese nombre\n");
            return -1; // Existe archivo con con ese nombre
        }
    }


    // Obtiene un nodo libre
    num = nextfreeinode();
    if(num == -1){
        printf("No hay nodo i disponible\n");
        return -1;
    }    

    // Asigna nodo y lo pone como no disponible
    result = assigninode(num);   
    //Escribe informacion a nodo i
    result = setninode(num, filename, atribs, uid, gid);
  
    return result;
}


// Buscar a partir del nombre del archivo el número de inodo correspondiente
int searchinode(char *filename)
{
    int i;
    int free;
    int result;

    if(!secboot_en_memoria)
    {
        result=vdreadsector(0,0,0,1,1,(char *) &secboot);
        secboot_en_memoria=1;
    }
    inicio_nodos_i=secboot.sec_res+secboot.sec_mapa_bits_nodos_i+secboot.sec_mapa_bits_bloques;

    if(!nodos_i_en_memoria)
    {
        for(i=0;i<secboot.sec_tabla_nodos_i;i++)
            result=vdreadseclog(inicio_nodos_i+i,(char *) &inode[i*8]);

        nodos_i_en_memoria=1;
    }
    
    if(strlen(filename)>19)
          filename[19]='\0';

    i=0;
    while(strcmp(inode[i].name,filename) && i<8)
        i++;

    if(i>=8)
        return(-1);
    else
        return(i);
}


// Elimina un inodo de la tabla de nodos i
// Util para borrar un archivo
// No falta poner los 10 bloques en 0?
// Se agregan lineas para borrar el contenido de los bloques de datos y el contenido del bloque indirecto
int removeinode(int numinode)
{
    int i;

    unsigned short temp[1024];
    char zeroBuffer[2048] = {0};

    for(i=0;i<10;i++)
    {
        
        if(inode[numinode].blocks[i]!=0)
        {
            unassignblock(inode[numinode].blocks[i]);
            writeblock(inode[numinode].blocks[i], zeroBuffer); //Para borrar el contenido de los bloques de datos
        }

    }

    if(inode[numinode].indirect!=0)
    {
        // Leer el bloque
        readblock(inode[numinode].indirect,(char *) temp);
        for(i=0;i<1024;i++)
        {
            if(temp[i]!=0)
            {
                unassignblock(temp[i]);
                writeblock(temp[i], zeroBuffer); //Para borrar el contenido de los bloques de datos
            }
        }

        unassignblock(inode[numinode].indirect);
        writeblock(inode[numinode].indirect, zeroBuffer);   // Borrar el bloque indirecto de apuntadores   
        inode[numinode].indirect=0;       
    }
    unassigninode(numinode);
    return(1);
}


//-----------------------------------------------------------------------------

// A partir de una posición en el archivo determina la dirección de memoria
// donde está el apuntador en el nodo i que está cargado en memoria.
unsigned short *postoptr(int fd,int pos)
{
    int currinode;
    unsigned short *currptr;
    unsigned short indirect1;

    // El número de inodo actual lo obtenemos de la tabla de archivos abiertos
    currinode=openfiles[fd].inode;

    // Está en los primeros 10 K
    if((pos/2048)<10)
        // Está entre los 10 apuntadores directos
        currptr=&inode[currinode].blocks[pos/2048];
    else if((pos/2048)<1034) //1034 por los 10 directos + los 1024 indirectosl
    {
        // Si el apuntador a bloque indirecto está vacío, asígnale un bloque
        if(inode[currinode].indirect==0)
        {
            // El siguiente bloque que disponible de acuerdo al mapa de bits
            indirect1=nextfreeblock();
            assignblock(indirect1); // Asígnalo
            inode[currinode].indirect=indirect1;
        } 
        // En la tabla de archivos abiertos tenemos el buffer que almacena
        // el bloque de apuntadores     
        currptr=&openfiles[fd].buffindirect[pos/2048-10];
    }
    else
        return(NULL);

    return(currptr);
}

unsigned short *currpostoptr(int fd)
{
    unsigned short *currptr;

    currptr=postoptr(fd,openfiles[fd].currpos);

    return(currptr);
}

// Escribe al apuntador indirecto
int write2indirect(int indirectblock, int block)
{

    int i;
    unsigned short temp[1024];

    // Leer bloque indirecto
    readblock(indirectblock,(char *) temp);
    for(i=0;i<1024;i++)
    {
        // Si el apuntador es 0, se puede escribir en el
        if(temp[i]==0)
        {
            temp[i] = block;
            writeblock(indirectblock, (char *) temp);
            break;
        }
    }

    if(i > 1023)
    {
        printf("Apuntador indirecto lleno");
        return -1;
    }

    return 0;
}





