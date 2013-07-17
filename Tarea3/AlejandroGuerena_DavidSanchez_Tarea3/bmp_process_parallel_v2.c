#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>


// Define and Check CPU Affinity
//#define CHILD_AFFINITY
//#define PARENT_AFFINITY
//#define SETCPU0
//#define SETCPU1
//#define SETCPU2
//#define SETCPU3
//#define CHECK_AFFINITY

#define DIF 16
#define PIXELPERPROCESS 1999
// NOMBRE DEL ARCHIVO A PROCESAR
//char filename[]="/home/agueren1/Documents/SistemasOperativos/Tarea3/IronMan.bmp";
char filename[]="imagen.bmp";


#pragma pack(2) // Empaquetado de 2 bytes
typedef struct {
    unsigned char magic1; // 'B'
    unsigned char magic2; // 'M'
    unsigned int size; // Tamaño
    unsigned short int reserved1, reserved2;
    unsigned int pixelOffset; // offset a la imagen
} HEADER;

#pragma pack() // Empaquetamiento por default
typedef struct {
    unsigned int size; // Tamaño de este encabezado INFOHEADER
    int cols, rows; // Renglones y columnas de la imagen
    unsigned short int planes;
    unsigned short int bitsPerPixel; // Bits por pixel
    unsigned int compression;
    unsigned int cmpSize;
    int xScale, yScale;
    unsigned int numColors;
    unsigned int importantColors;
} INFOHEADER;

typedef struct {
    unsigned char red;  
    unsigned char green;
    unsigned char blue;
} PIXEL;

typedef struct {
    HEADER header;
    INFOHEADER infoheader;
    PIXEL *pixel;
} IMAGE;

typedef struct {
    int imageRows, imageCols; 
    int initCol, endCol;
    int initRow, endRow ; 
    PIXEL *ftepixel; 
    PIXEL *dstpixel;
} PIXIMAGE;	

IMAGE imagenfte,imagendst;

int loadBMP(char *filename, IMAGE *image)
{
    FILE *fin;
    
    int i=0;
    int totpixs=0;

    fin = fopen(filename, "rb+"); //rb+ open file for update(reading or writing)

    // Si el archivo no existe
    if (fin == NULL)
        return(-1);

    // Leer encabezado
    fread(&image->header, sizeof(HEADER), 1, fin);

    // Probar si es un archivo BMP
    if (!((image->header.magic1 == 'B') && (image->header.magic2 == 'M')))
        return(-1);

    fread(&image->infoheader, sizeof(INFOHEADER), 1, fin);

    // Probar si es un archivo BMP 24 bits no compactado
    if (!((image->infoheader.bitsPerPixel == 24) && !image->infoheader.compression))
        return(-1);

    image->pixel=(PIXEL *)malloc(sizeof(PIXEL)*image->infoheader.cols*image->infoheader.rows); // parametro, numero de byts a reservar
    // regresa apuntador * //regresa la localizacion "0" del espacio de memoria reservadolll

    totpixs=image->infoheader.rows*image->infoheader.cols;

    while(i<totpixs)
    {
        fread(image->pixel+i, sizeof(PIXEL),512, fin); // porque el 512
        i+=512;
    }

    fclose(fin);

}


int saveBMP(char *filename, IMAGE *image)
{
    FILE *fout;
    int i,totpixs;

    fout=fopen(filename,"wb");
    if (fout == NULL)
        return(-1); // Error

    // Escribe encabezado
    fwrite(&image->header, sizeof(HEADER), 1, fout);


    // Escribe información del encabezado
    fwrite(&image->infoheader, sizeof(INFOHEADER), 1, fout);

    i=0;
    totpixs=image->infoheader.rows*image->infoheader.cols;
    

    while(i<totpixs)
    {
        fwrite(image->pixel+i,sizeof(PIXEL),512,fout);
        i+=512;
    }

    fclose(fout);
}

unsigned char blackandwhite(PIXEL p)
{
    return((unsigned char) (0.3*((float)p.red)+ 0.59*((float)p.green)+0.11*((float)p.blue)));
}



// Process Image by a giving range
static int processBMPrangetest(void *arg)
{
    int i, j;
    PIXEL *ftepixel;
    PIXEL *dstpixel;
    PIXEL *pfte, *pdst;
    PIXEL *v0,*v1,*v2,*v3,*v4,*v5,*v6,*v7;
    int imageRows, imageCols;
    int initrow, endrow, initcol, endcol;
    
    // CPU Affinity is defined for Child
    #ifdef CHILD_AFFINITY 
    int cpu0, cpu1, cpu2, cpu3;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    #ifdef SETCPU0 
    CPU_SET(0, &mask);
    #endif
    #ifdef SETCPU1
    CPU_SET(1, &mask);
    #endif
    #ifdef SETCPU2
    CPU_SET(2, &mask);
    #endif
    #ifdef SETCPU3
    CPU_SET(3, &mask);
    #endif
    
    if(sched_setaffinity(getpid(), sizeof(cpu_set_t), &mask) == -1){
        fprintf(stderr,"Error Affinity\n");
        exit(1);    
    }

    #ifdef CHECK_AFFINITY
    if(sched_getaffinity(getpid(), sizeof(cpu_set_t), &mask) == -1){
        fprintf(stderr,"Error Affinity\n");
        exit(1);    
    }
  
    cpu0 = CPU_ISSET(0, &mask);
    cpu1 = CPU_ISSET(1, &mask);
    cpu2 = CPU_ISSET(2, &mask);
    cpu3 = CPU_ISSET(3, &mask);
    printf("Child PID=%ld CPU 0: %s, CPU 1: %s, CPU 2: %s, CPU 3: %s\n", (long) getpid(), cpu0 ? "set" : "unset", cpu1 ? "set" : "unset", cpu2 ? "set" : "unset", cpu3 ? "set" : "unset");
    #endif
    #endif
 
    PIXIMAGE *pixelimage;

    // Cast to true form
    pixelimage = (PIXIMAGE *) arg;  
    
    imageRows = pixelimage->imageRows;
    imageCols = pixelimage->imageCols;
    initrow = pixelimage->initRow;
    initcol = pixelimage->initCol; 
    endrow = pixelimage->endRow;
    endcol = pixelimage->endCol;
    ftepixel = pixelimage->ftepixel;
    dstpixel = pixelimage->dstpixel;
   

    for(i=initrow;i<endrow;i++){
        for(j=initcol;j<endcol;j++){ 
            pfte=ftepixel+imageCols*i+j;
            v0=pfte-imageCols-1;
            v1=pfte-imageCols;
            v2=pfte-imageCols+1;
            v3=pfte-1;
            v4=pfte+1;
            v5=pfte+imageCols-1;
            v6=pfte+imageCols;
            v7=pfte+imageCols+1;

            pdst=dstpixel+imageCols*i+j;    
    
            if(abs(blackandwhite(*pfte)-blackandwhite(*v0))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v1))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v2))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v3))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v4))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v5))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v6))>DIF ||
               abs(blackandwhite(*pfte)-blackandwhite(*v7))>DIF)
            {             

                if(initrow == 1) {
                pdst->red=0;
                pdst->green=255;
                pdst->blue=0;
                }
                else if(initrow == 750){
                pdst->red=0;
                pdst->green=0;
                pdst->blue=255;
                }
                else if(initrow == 1500){
                pdst->red=255;
                pdst->green=0;
                pdst->blue=0;
                }
                else{
                pdst->red=0;
                pdst->green=0;
                pdst->blue=0;
                }
              
            }
            else
            {
                pdst->red=255;
                pdst->green=255;
                pdst->blue=255;
            }
          }
        }

    //write(1, "Finish\n", 7);
    return 0;  

}


void processBMP(IMAGE *imagefte, IMAGE *imagedst)
{
    int j;
    int imageRows,imageCols;
    
    int id_child[4];
    char *tmp_stack[4];

    int i_row[4];
    int f_row[4];

    const int STACK_SIZE = 65535;
    char *stack;
    char *stackTop;
    PIXIMAGE pixelimage[4];
    

    i_row[0]= 1;
    i_row[1]= 750;
    i_row[2]= 1500;
    i_row[3]= 2250;
    f_row[0]= 750;
    f_row[1]= 1500;
    f_row[2]= 2250;
    f_row[3]= 3000;

    memcpy(imagedst,imagefte,sizeof(IMAGE)-sizeof(PIXEL *));

    imageRows = imagefte->infoheader.rows;
    imageCols = imagefte->infoheader.cols;

    imagedst->pixel=(PIXEL *)malloc(sizeof(PIXEL)*imageRows*imageCols);


    
        for(j=0;j<4;j=j+1)
        {

            pixelimage[j].imageRows = imageRows;
            pixelimage[j].imageCols = imageCols;
            pixelimage[j].ftepixel  = imagefte->pixel;
            pixelimage[j].dstpixel  = imagedst->pixel;
            pixelimage[j].initCol   = 1;
            pixelimage[j].endCol    = imageCols-1;
            pixelimage[j].initRow   = i_row[j];
            pixelimage[j].endRow    = f_row[j];

            // Allocate stack for child
            stack = malloc(STACK_SIZE);
            if(stack == NULL){
                fprintf(stderr,"Error NULL Malloc\n");
                exit(1);
            }
            stackTop = stack + STACK_SIZE;

            // Create Child Process using Clone
            id_child[j] = clone(processBMPrangetest, stackTop, CLONE_FILES | CLONE_FS| CLONE_VM | CLONE_SIGHAND, &pixelimage[j]);

            if(id_child[j] == -1){
                fprintf(stderr,"Error Clone\n");
                exit(1);
            }

            tmp_stack[j] = stack;            

            //printf("CHILD SENT TO EXECUTE: %d \n", id_child[j]);

        }

        for(j=0;j<4;j=j+1)
        {
             //printf("WAITING CHILD: %d \n", id_child[j]);
             //Wait for childs to terminate
             if (waitpid(id_child[j], NULL, __WALL) == -1){
                fprintf(stderr,"waitpid\n");
                exit(1);
             }
             //printf("CHILD TERMINATE: %d \n", id_child[j]);
             // Free stack
             free(tmp_stack[j]);
        }
    
 
}



int main()
{
    int res;
    clock_t t_inicial,t_final;
    clock_t t_inicial2,t_final2;
    char namedest[80];

    // CPU Affinity is defined for Parent
    #ifdef PARENT_AFFINITY
    int cpu0, cpu1, cpu2, cpu3;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    #ifdef SETCPU0 
    CPU_SET(0, &mask);
    #endif
    #ifdef SETCPU1
    CPU_SET(1, &mask);
    #endif
    #ifdef SETCPU2
    CPU_SET(2, &mask);
    #endif
    #ifdef SETCPU3
    CPU_SET(3, &mask);
    #endif
   
    if(sched_setaffinity(getpid(), sizeof(cpu_set_t), &mask) == -1){
        fprintf(stderr,"Error Affinity\n");
        exit(1);    
    }

    #ifdef CHECK_AFFINITY
    if(sched_getaffinity(getpid(), sizeof(cpu_set_t), &mask) == -1){
        fprintf(stderr,"Error Affinity\n");
        exit(1);    
    }

    cpu0 = CPU_ISSET(0, &mask);
    cpu1 = CPU_ISSET(1, &mask);
    cpu2 = CPU_ISSET(2, &mask);
    cpu3 = CPU_ISSET(3, &mask);
    printf("Parent CPU 0: %s, CPU 1: %s, CPU 2: %s, CPU 3: %s\n", cpu0 ? "set" : "unset", cpu1 ? "set" : "unset", cpu2 ? "set" : "unset", cpu3 ? "set" : "unset");
    #endif
    #endif

    t_inicial=clock();
    
    strcpy(namedest,strtok(filename,"."));
    strcat(filename,".bmp"); 
    strcat(namedest,"_P.bmp");
    printf("Archivo fuente %s\n",filename);
    printf("Archivo destino %s\n",namedest);

    res=loadBMP(filename,&imagenfte);
    if(res==-1)
    {
        fprintf(stderr,"Error al abrir imagen\n");
        exit(1);
    }

    printf("Procesando imagen de: Renglones = %d, Columnas = %d\n",imagenfte.infoheader.rows,imagenfte.infoheader.cols);

    processBMP(&imagenfte,&imagendst);
    
    res=saveBMP(namedest,&imagendst);
    if(res==-1)
    {
        fprintf(stderr,"Error al escribir imagen\n");
        exit(1);
    }
    t_final=clock();

    printf("Tiempo %3.6f segundos\n",((float) t_final- (float)t_inicial)/ CLOCKS_PER_SEC);
}

