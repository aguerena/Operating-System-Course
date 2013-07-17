#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "vdisk.h"
#include "lowlvldisk.h"
#include "tiposdatos.h"

int formatDisk(const char *diskname, int disknumber);

int main(int argc,char **argv)
{
    int fp;
    int i;
    int disknumber;
    char buffer[512];
    const char *diskname = "disk";

    if(argc!=2)
    {
        fprintf(stderr,"Debe indicar el número de disco virtual a formatear\n\n");
        exit(2);
    }
    if(argv[1][0]< '0' || argv[1][0]> '3')
    {
        fprintf(stderr,"Los números válidos para formatear un disco son entre 0 y 3\n\n");
        exit(2);
    }

    disknumber = atoi(argv[1]);
    printf("disknumber is: %d\n", disknumber);

    if(formatDisk(diskname, disknumber) == -1)
    {
        fprintf(stderr,"Error al formatear el disco\n");
        exit(2);
    }       

    return 1;
}


int formatDisk(const char *diskname, int disknumber)
{

    char zeroBuffer[512] = {0};
    int i, j;

    struct SECBOOT bootEdd;
    struct INODE inodeEdd[8];

    struct INODE inoderead[8];

    //printf("Size of SECBOOT is:%d\n", sizeof(struct SECBOOT));
    //printf("Size of INODE is:%d\n", sizeof(struct INODE)); 
     
    /* Escribir el sector de arranque con los parametros del disco en sector fisico 1
     cilindro 0, superficie 0 */
    bootEdd.nombre_disco           = diskname;
    bootEdd.sec_res                = 1;	
    bootEdd.sec_mapa_bits_nodos_i  = 1; 
    bootEdd.sec_mapa_bits_bloques  = 4; 
    bootEdd.sec_tabla_nodos_i      = 1;	
    bootEdd.sec_log_unidad         = 54400; 
    bootEdd.sec_x_bloque           = 4;	
    bootEdd.heads                  = 20; 
    bootEdd.cyls                   = 160; 
    bootEdd.secfis                 = 17;

    // Escribir a sector de arranque
    if( vdwritesector (disknumber, 0, 0, 1, 1, (char*) &bootEdd) == -1)
        return -1;

    /* Reiniciar los mapas de bits (todos en 0 = libres) */
    // Escribir a sector que contiene el mapa de bits de nodos i y de areas de datos
    for(i=0; i<5; i++)
    {
        if( vdwritesector (disknumber, 0, 0, 2+i, 1, zeroBuffer) == -1)
            return -1;
    }

    /* Reiniciar tablas de nodos i */
    for(i=0; i<8; i++)
    {
        //inodeEdd[i].name;
        inodeEdd[i].uid            = 0;
        inodeEdd[i].gid            = 0;
        inodeEdd[i].perms          = 0;   
        inodeEdd[i].datetimecreat  = 0;
        inodeEdd[i].datetimemodif  = 0;
        inodeEdd[i].size           = 0;
        for(j=0; j<10; j++)
            inodeEdd[i].blocks[j]  = 0;
        inodeEdd[i].indirect       = 0;
        inodeEdd[i].indirect2      = 0;
    }
    
    // Escribir a sector que contiene las tablas de nodos i
    if( vdwritesector (disknumber, 0, 0, 7, 1, (char*)&inodeEdd ) == -1)
    //if( vdwritesector (disknumber, 0, 0, 7, 1, (char*) &inodeEdd[0] ) == -1)
        return -1;

    // asignar bloque 0, para que no pueda ser utilizado despues
    assignblock(0);

    return 0;

}
