#define HEADS 20
#define SECTORS 17
#define CYLINDERS 160

int vdwritesector(int drive, int head, int cylinder, int sector, int nsecs, char *buffer);
int vdreadsector(int drive, int head, int cylinder, int sector, int nsecs, char *buffer);
 
