#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "vdisk.h"
#include "lowlvldisk.h"

#define LINESIZE 16
#define SECSIZE 512
#define MAX_SEC 54400

int main(int argc,char *argv[])
{
    const int drive = 0;
    int ncyl,nhead,nsec, sec_log;
    int fd;
    unsigned char buffer[SECSIZE];
    int offset;
    int i,j,r;
    unsigned char c;

    int value;

    if(argc!=2)
    {
        fprintf(stderr,"Debe indicar el número de sector logico\n");
        exit(2);
    }

    sec_log = atoi(argv[1]);

    if( ( sec_log < 0) || ( sec_log > (MAX_SEC -1) ) )
    {
        fprintf(stderr,"Numero de sector logico invalido\n");
        exit(2);
    }

    printf("Desplegando de disco0.vd Sector Logico=%d\n", sec_log);

    /* Usando sectores fisicos*/
    /*slogico2fisico(&nhead, &ncyl, &nsec, sec_log);    

    printf("Desplegando de disco0.vd Cil=%d, Sup=%d, Sec=%d\n", ncyl,nhead,nsec);
		
    if(vdreadsector(drive,nhead,ncyl,nsec,1,buffer)==-1)
    {
        fprintf(stderr,"Error al abrir disco virtual\n");
        exit(1);
    }*/

    /* Usando sectores logicos*/
    if(vdreadseclog(sec_log, buffer)==-1)
    {
        fprintf(stderr,"Error al abrir disco virtual\n");
        exit(1);
    }


    for(i=0;i<SECSIZE/LINESIZE;i++)
    {
        printf("\n %3X -->",i*LINESIZE);
        for(j=0;j<LINESIZE;j++)
        {
	    c=buffer[i*LINESIZE+j];
     	    printf("%2X ",c);
	}
	printf("  |  ");
	for(j=0;j<LINESIZE;j++)
	{
	    c=buffer[i*LINESIZE+j]%256;
	    if(c>0x1F && c<127)
	        printf("%c",c);
	    else
	        printf(".");
	}
    }
	printf("\n");
}

