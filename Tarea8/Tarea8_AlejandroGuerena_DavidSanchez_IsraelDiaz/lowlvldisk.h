
struct DATE;
struct INODE;


void slogico2fisico(int *superficie, int *cilindro, int *sector, int sec_log); 
int vdreadseclog(int sec_log, char *buffer); 
int vdwriteseclog(int sec_log, char *buffer); 

// Determina si esta libre un bloque de datos
int isblockfree(int block); 
// Buscar en el mapa de bits cuál es el siguiente bloque de datos libre
int nextfreeblock(); 
// Poner un bloque como no disponible
int assignblock(int block); 
// Establecer el bloque como libre
int unassignblock(int block); 

// Determina si esta libre un nodo i
// Return Libre = 1, Ocupado = 0
int isinodefree(int inode); 
// Buscar en el mapa de bits cuál es el siguiente nodo i libre
int nextfreeinode(); 
// Poner un nodo i como no disponible
int assigninode(int inode); 
// Establecer un nodo i como libre
int unassigninode(int inode); 

// Escritura de bloque
int writeblock(int block,char *buffer); 
// Lectura de bloque
int readblock(int block,char *buffer); 

// Convierte la fecha que está en una estructura fecha a un entero de 32 bits
unsigned int datetoint(struct DATE date); 
// Extraer la fecha y hora que está empaquetada en un entero de 32 bits
// Almacena los resultados en una estructura
int inttodate(struct DATE *date,unsigned int val); 
// Fecha y hora actual empaquetada en un entero de 32 bits
// Las funciones de creación y escritura de archivos usan esta función.
unsigned int currdatetimetoint(); 

// En un inodo específico escribe el nombre, atributos y usuario y dueño del nuevo archivo.
// Usada por la función vdcreate
int setninode(int num, char *filename,unsigned short atribs, int uid, int gid);
// asigna nodo i, (si es que existe uno libre), poniendolo como no disponible e escribe informacion a el
int assignsetinode(char *filename, unsigned short atribs, int uid, int gid);
// Buscar a partir del nombre del archivo el número de inodo correspondiente
int searchinode(char *filename); 
// Elimina un inodo de la tabla de nodos i
// Util para borrar un archivo
int removeinode(int numinode);
// Escribe al apuntador indirecto
int write2indirect(int indirectblock, int block);

// A partir de una posición en el archivo determina la dirección de memoria
// donde está el apuntador en el nodo i que está cargado en memoria.
unsigned short *postoptr(int fd,int pos);
unsigned short *currpostoptr(int fd);


