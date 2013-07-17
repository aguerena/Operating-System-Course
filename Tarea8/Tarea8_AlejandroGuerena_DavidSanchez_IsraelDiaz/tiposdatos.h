//#include <iostream>
//#include <cstdlib>
#include "vdisk.h"

#define NUM_CILINDROS 160
#define NUM_SUPERFICIES 20
#define SECTORES_X_PISTA 17

struct SECBOOT {
	char jump[4];
	const char* nombre_disco;
	unsigned char sec_res;		// 1 sector de arranque
	unsigned char sec_mapa_bits_nodos_i;  // 1 sector
	unsigned char sec_mapa_bits_bloques;  // 4 sectores
	unsigned short sec_tabla_nodos_i;	// 1 sector
	unsigned short sec_log_unidad;	// 54400 sectores
	unsigned char sec_x_bloque;		// 4 sectores por bloque
	unsigned char heads;			// 20 superficies
	unsigned char cyls;			// 160 cilindros
	unsigned char secfis;			// 17 sectores
	//char restante[487];
        char restante[489];
};


struct INODE {
	char name[20];
	unsigned short uid;
	unsigned short gid;
	unsigned short perms;
	unsigned int datetimecreat;
	unsigned int datetimemodif;
	unsigned int size;
	unsigned short blocks[10];
	unsigned short indirect;
	unsigned short indirect2;
};


struct OPENFILES {
	int inuse;
	unsigned short inode;
	int currpos;
	int currbloqueenmemoria;
	char buffer[2048];
	unsigned short buffindirect[1024];
};

struct DATE {
    unsigned int sec   : 6;
    unsigned int min   : 6;
    unsigned int hour  : 5;
    unsigned int day   : 5;
    unsigned int month : 4;
    unsigned int year  : 6;    
};


/*************************************************************
/* Tipos de datos para el manejo de directorios
 *************************************************************/

typedef int VDDIR;
	

struct vddirent 
{
	char *d_name;
};

struct vddirent *vdreaddir(VDDIR *dirdesc);
VDDIR *vdopendir(char *path);







