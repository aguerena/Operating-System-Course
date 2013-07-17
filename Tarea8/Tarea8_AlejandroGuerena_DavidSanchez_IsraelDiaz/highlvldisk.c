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


//Funciones de alto nivel.
//Posicionamiento del apuntador del archivo.
extern int inicio_nodos_i;
extern int nodos_i_en_memoria;
extern int secboot_en_memoria; 

extern struct OPENFILES openfiles[16];
extern struct SECBOOT secboot;
extern struct INODE inode[8];

// fd = Descriptor del archivo
// Offset = Cuanto hay que mover el apuntador
// Whence = A partir de dond
//    0 = A partir del inicio.
//    1 = A partir de la posición actual del puntero.
//    2 = A partir del final del archivo.
//
// devuelve la posición donde queda el puntero del archivo después de moverlo.

int vdseek(int fd, int offset, int whence)
{
    unsigned short oldblock,newblock;

    // Si no está abierto regresa error
    if(openfiles[fd].inuse==0)
        return(-1);

    oldblock=*currpostoptr(fd);
        // A partir de la posición actual del archivo, me regresa un apuntador
        // a una dirección de memoria que contiene el bloque actual.
        //     Puede ser el inodo o un bloque de apuntadores.
        

    if(whence==0) // A partir del inicio
    {
        // Si el offset es negativo o excede el tamaño del archivo regresa
        // error
        if(offset<0 || 
           openfiles[fd].currpos+offset>inode[openfiles[fd].inode].size)
            return(-1);
        openfiles[fd].currpos=offset;

    } else if(whence==1) // A partir de la posición actual
    {
        // Validar que posición actual - offset no vaya antes del principio
        // también que posición actual + offset no exceda el tamaño del archivo
        if(openfiles[fd].currpos+offset>inode[openfiles[fd].inode].size ||
           openfiles[fd].currpos+offset<0)
            return(-1);
        openfiles[fd].currpos+=offset;

    } else if(whence==2) // A partir del final
    {
        // Validar que no sea positivo
        // Si es negativo, que el valor absoluto no exceda el 
// tamaño del archivo
        if(offset>inode[openfiles[fd].inode].size ||
           openfiles[fd].currpos-offset<0)
            return(-1);
        openfiles[fd].currpos=inode[openfiles[fd].inode].size-offset;
    } else
        return(-1);
    // Una vez cambiada la posición, obtenemos el nuevo bloque actual
    newblock=*currpostoptr(fd);
    
    // Si después del movimiento hay un cambio de bloque
    if(newblock!=oldblock)
    {
        // Escribir el bloque que ya no estoy usando
        // Si quieren en la tabla de archivos abiertos poner una bandera que
        // indica si el bloque fue modificado y si es así escríbelo
        // Una vez que se escribe, poner esa bandera en 0.
        writeblock(oldblock,openfiles[fd].buffer);

        // Cargar el nuevo bloque a memoria
        readblock(newblock,openfiles[fd].buffer);
        openfiles[fd].currbloqueenmemoria=newblock;
    }

    return(openfiles[fd].currpos);
}


// Create
// Crea un archivo nuevo en el disco virtual, el archivo queda abierto
// filename = nombre del archivo a crear
// permission = permisos del archivo
int vdcreat(char *filename, int permission)
{
    int fd;
    int inodenum;

    // Checar que se pueda crear un archivo
    // La funcion checa que haya nodo i disponible para nuevo archivo y que no exista un archivo con ese nombre
    inodenum = assignsetinode(filename, permission, getuid(), getgid());

    if(inodenum  == -1){
        printf("No es posible crear archivo nuevo \n");
        return -1;
    }

    // Se abre archivo
    fd = vdopen(filename, 0);      

    if(fd == -1){
        printf("No es posible crear archivo nuevo \n");
        return -1;
    }

    return fd;
}


// Abrir
// filename = nombre del archivo a abrir
// mode = modo en que lo vamos a abrir(read only, read/write)
int vdopen(char *filename, int mode)
{  
    int fd;
    int inodenum;

    // Se obtiene el numero de nodo i del archivo
    inodenum = searchinode(filename);
    if(inodenum  == -1)
    {
        printf("No es posible abrir archivo\n");
        return -1;
    }

    // Buscar si el nodo i esta libre, indicando que el archivo fue borrado anteriormente, pero el nombre
    // se mantiene en las tablas de nodo i
    if(isinodefree(inodenum) == 1)
    {
        printf("No es posible abrir archivo\n");
        return -1;
    }

    // Si no se encuentra cerrado, entonces error, porque ya se abrio anteriormente
    if(openfiles[inodenum].inuse != 0){
        printf("El archivo se encuentra ya abierto\n");
        return inodenum;
    }
    

    //Asignar el nodo i al descriptor
    fd = inodenum;

    // Asigna valores a la estructura openfiles
    openfiles[fd].inode = inodenum;
    openfiles[fd].inuse = 1;
    openfiles[fd].currpos = 0;
    openfiles[fd].currbloqueenmemoria = 0;
    // Cargar bloque indirectos a buffer indirecto (si es que existe)    
    if (inode[inodenum].indirect != 0)
    {   
        readblock(inode[inodenum].indirect, (char *) openfiles[fd].buffindirect);
    }   

    return fd;
}

// Close
// Cierra un archivo y lo elimina de la tabla de archivos abiertos.
// Solo va a recibir como parametro el descriptor del archivo a cerrar
int vdclose(int fd)
{
    // Si no está abierto, regresa error
    if(openfiles[fd].inuse==0)
    {
        printf("Error cerrar archivo\n");
        return(-1);
    }

    // Poner el archivo como cerrado
    openfiles[fd].inuse =0;
    openfiles[fd].currpos = 0; 
    openfiles[fd].currbloqueenmemoria = 0;
    //openfiles[fd].buffer = {0};
    //openfiles[fd].buffindirect = {0};       
 
    return 0;
}


// Unlink
// Borra un archivo del directorio raiz del disco virtual
// Path es el nombre del archivo a borrar
int vdunlink(char *path)
{
    int inodenum;

    // Buscar el nodo i correspondiente al nombre del archivo
    inodenum = searchinode(path);
    if(inodenum == -1)
    {
        printf("Error al borrar archivo. No hay archivo con ese nombre \n");
        return -1; // No hay archivo con ese nombre
    }

    // Buscar si el nodo i esta libre, indicando que el archivo fue borrado anteriormente, pero el nombre
    // se mantiene en las tablas de nodo i
    if(isinodefree(inodenum) == 1)
    {
        printf("Error al borrar archivo. No hay archivo con ese nombre \n");
        return -1; // El archivo no tiene nodo i asignado, no se puede borrar  
    }  

    // Se borra inodo y los bloques asociados a ese nodo i  
    if(removeinode(inodenum) != -1)
        return 0;
    else
    {
        printf("Error al borrar archivo\n");
        return -1;
    }

}


// Read.
// fd es el identificador en la tabla de archivos abiertos
// buffer un apuntador al area de memoria donde está lo que vamos a leer
// bytes cuántos bytes vamos a leer
int vdread(int fd, char *buffer, int bytes)
{
    int currblock;
    int currinode;
    int cont=0;
    int sector;
    int i;
    int result;
    unsigned short *currptr;

    // Si no está abierto, regresa error
    if(openfiles[fd].inuse==0)
        return(-1);

    // Determinar cuál es el inodo de la tabla de nodos i del archivo que vamos a leer
    currinode=openfiles[fd].inode;

    // Ciclo para insertar byte por byte en el buffer a leer
    while(cont<bytes)
    {
        // Obtener la dirección de donde está el bloque que corresponde
        // a la posición actual, si no hay bloque asignado a la posición
        // actual del archivo, regresamos error
        currptr=currpostoptr(fd);
        if(currptr==NULL)
            return(-1);
    
        // Obtener el número de bloque actual
        currblock=*currptr;

        // Si el bloque está en 0, no hay bloque para leer
        if(currblock==0)
        {
            return cont;
        }

        // Si el bloque de la posición actual no está en memoria
        // Lee el bloque al buffer del archivo
        if(openfiles[fd].currbloqueenmemoria!=currblock)
        {
            // Leer el bloque actual hacia el buffer que
            // está en la tabla de archivos abiertos
            readblock(currblock,openfiles[fd].buffer);            
            openfiles[fd].currbloqueenmemoria=currblock;
        }

        // Copia el buffer donde tenemos el bloque actual al buffer que recibe la función
        buffer[cont] = openfiles[fd].buffer[openfiles[fd].currpos%2048];

        // Incrementa posición actual del archivo
        openfiles[fd].currpos++;

        // Termina de leer si ya no hay mas caracteres que leer
        if(buffer[cont] == '\0'){
            return (cont);
        }

        // Incrementa el contador
        cont++;

    }
    return(cont);
}



// Escritura.
// fd es el identificador en la tabla de archivos abiertos
// buffer un apuntador al area de memoria donde está lo que vamos a escribir
// bytes cuántos bytes vamos a escribir
int vdwrite(int fd, char *buffer, int bytes)
{
    int currblock;
    int currinode;
    int cont=0;
    int sector;
    int i;
    int result;
    unsigned short *currptr;

    char bufferindirect[2048] = {0};
    int countindirect = 0;
    int indi = 0;

    // Si no está abierto, regresa error
    if(openfiles[fd].inuse==0)
        return(-1);

    // Determinar cuál es el inodo de la tabla de nodos i del archivo que
    // vamos a escribir
    currinode=openfiles[fd].inode;

    // Ciclo para recorrer byte por byte del buffer a escribir
    while(cont<bytes)
    {
        // Obtener la dirección de donde está el bloque que corresponde
        // a la posición actual, si no hay bloque asignado a la posición
        // actual del archivo, regresamos error
        currptr=currpostoptr(fd);
        if(currptr==NULL)
            return(-1);
    
        // Obtener el número de bloque actual
        currblock=*currptr;

        // Si el bloque está en 0, aún no hay bloque para 
        // escribir este carácter del archivo
        // hay que darle uno
        if(currblock==0)
        {
            // Busca un bloque libre en el mapa de bits
            // si no hay bloques libres, regresa -1
            currblock=nextfreeblock();
            if(currblock==-1)
                return(-1);

            // El bloque encontrado ponerlo en donde
            // apunta el apuntador al bloque actual
            *currptr=currblock; // aquí ya lo estoy escribiendo en el nodo i.
            assignblock(currblock); // Poner ese bloque como asignado     

            // Logica para escribir los valores de apuntadores en el bloque indirecto
            if(inode[currinode].indirect != 0)
            {
                if(write2indirect(inode[currinode].indirect, currblock) == -1)
                    return -1;
            }
             
            // Los cambios se hicieron en el nodo i en memoria, ahora hay que escribirlos en el disco
            // Escribir el sector de la tabla de nodos i en el disco
            // Recordar que los nodos i son de 64 bytes y caben 8 en un sector
            sector=(currinode/8)*8;
            result=vdwriteseclog(inicio_nodos_i+sector,(char *) &inode[sector*8]);
        }

        // Si el bloque de la posición actual no está en memoria
        // Lee el bloque al buffer del archivo
        if(openfiles[fd].currbloqueenmemoria!=currblock)
        {
            // Leer el bloque actual hacia el buffer que
            // está en la tabla de archivos abiertos
            readblock(currblock,openfiles[fd].buffer);            
            openfiles[fd].currbloqueenmemoria=currblock;
        }

        // Copia el caracter del buffer que recibe la función
        // vdwrite, al buffer donde tenemos el bloque actual
        openfiles[fd].buffer[openfiles[fd].currpos%2048]=buffer[cont];

        // Incrementa posición actual del archivo
        openfiles[fd].currpos++;

        // Si la posición es mayor que el tamaño, modifica el tamaño
        if(openfiles[fd].currpos>inode[currinode].size)
            inode[openfiles[fd].inode].size=openfiles[fd].currpos;

        // Incrementa el contador
        cont++;
    
        // Si se llena el buffer, escríbelo
        // Determinar si se quiere escribir byte por byte o por bloque o
        // byte == cont

        // Byte por Byte
        /*writeblock(currblock,openfiles[fd].buffer);*/
        
        // Por bloque
        /*if(openfiles[fd].currpos%2048==0){
            writeblock(currblock,openfiles[fd].buffer);
        }*/
  
        // cont == byte
        if(cont == bytes){
            writeblock(currblock,openfiles[fd].buffer);
        }

    }
    return(cont);
}

/*************************************************************
/* Funciones para el manejo de directorios
 *************************************************************/
VDDIR dirs[2]={-1,-1};
struct vddirent current;


VDDIR *vdopendir(char *path)
{
	int i=0;
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

	if(strcmp(path,".")!=0)
		return(NULL);

	i=0;
	while(dirs[i]!=-1 && i<2)
		i++;

	if(i==2)
		return(NULL);

	dirs[i]=0;

	return(&dirs[i]);	
} 

struct vddirent *vdreaddir(VDDIR *dirdesc)
{
	int i;

	int result;
	if(!nodos_i_en_memoria)
	{
		for(i=0;i<secboot.sec_tabla_nodos_i;i++)
			result=vdreadseclog(inicio_nodos_i+i,(char *) &inode[i*8]);

		nodos_i_en_memoria=1;
	}

	// Mientras no haya nodo i, avanza
	while(isinodefree(*dirdesc) && *dirdesc<4096)
		(*dirdesc)++;


	// Apunta a donde está el nombre en el inodo	
	current.d_name=inode[*dirdesc].name;

	(*dirdesc)++;

	if(*dirdesc>=4096)
		return(NULL);
	return( &current);	
}


int vdclosedir(VDDIR *dirdesc)
{
	(*dirdesc)=-1;
}





