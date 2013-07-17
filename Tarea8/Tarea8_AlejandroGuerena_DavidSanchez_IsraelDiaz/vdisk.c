#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include "vdisk.h" 

int currentcyl[4]={0,0,0,0};
int currentsec[4]={0,0,0,0};

int vdwritesector(int drive, int head, int cylinder, int sector, int nsecs, char *buffer)
{
    char filename[20];
    int fp;
    int timecyl,timesec;
    int sl,offset;
    sprintf(filename,"disco%c.vd",(char) drive+'0');
    fp=open(filename,O_WRONLY);
    if(fp==-1)
        return(-1);

    // Valida parámetros
    if(drive<0 || drive>3)
        return(-1);

    if(head<0 || head>=HEADS)
        return(-1);

    if(cylinder<0 || cylinder>=CYLINDERS)
        return(-1);

    if(sector<1 || sector>SECTORS)
        return(-1);

    if(sector+nsecs-1>SECTORS)
        return(-1);

    // Hace el retardo
    timesec=sector-currentsec[drive];
    if(timesec<0)
        timesec+=SECTORS;
    usleep(timesec*1000);
    currentsec[drive]=sector;

    timecyl=abs(currentcyl[drive]-cylinder);
    usleep(timecyl*1000);    
    currentcyl[drive]=cylinder;

    // Calcula la posición en el archivo
    sl=cylinder*SECTORS*HEADS+head*SECTORS+(sector-1);
    offset=sl*512;
    lseek(fp,offset,SEEK_SET);
    write(fp,buffer,512*nsecs);
    close(fp);
    return(nsecs);
}

int vdreadsector(int drive, int head, int cylinder, int sector, int nsecs, char *buffer)
{
    char filename[20];
    int fp;
    int timecyl,timesec;
    int sl,offset;
    sprintf(filename,"disco%c.vd",(char) drive+'0');
    fp=open(filename,O_RDONLY);
    if(fp==-1)
        return(-1);
                                                                                
    // Valida parámetros
    if(drive<0 || drive>3)
        return(-1);
                                                                                
    if(head<0 || head>=HEADS)
        return(-1);
                                                                                
    if(cylinder<0 || cylinder>=CYLINDERS)
        return(-1);
                                                                                
    if(sector<1 || sector>SECTORS)
        return(-1);
                                                                                
    if(sector+nsecs-1>SECTORS)
        return(-1);
                                                                                
    // Hace el retardo
    timesec=sector-currentsec[drive];
    if(timesec<0)
        timesec+=SECTORS;
    usleep(timesec*1000);
    currentsec[drive]=sector;
                                                                                
    timecyl=abs(currentcyl[drive]-cylinder);
    usleep(timecyl*1000);
    currentcyl[drive]=cylinder;
                                                                                
    // Calcula la posición en el archivo
    sl=cylinder*SECTORS*HEADS+head*SECTORS+(sector-1);
    offset=sl*512;
    lseek(fp,offset,SEEK_SET);
    read(fp,buffer,512*nsecs);
    close(fp);
    return(nsecs);
}
                                                                                

