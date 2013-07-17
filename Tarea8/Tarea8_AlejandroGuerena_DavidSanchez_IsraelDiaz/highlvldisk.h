
int vdseek(int fd, int offset, int whence);
int vdcreat(char *filename, int permission);
int vdopen(char *filename, int mode);
int vdclose(int fd);
int vdunlink(char *path);
int vdread(int fd, char *buffer, int bytes);
int vdwrite(int fd, char *buffer, int bytes);

/*
int vdclosedir(VDDIR *dirdesc);
struct vddirent *vdreaddir(VDDIR *dirdesc);
VDDIR *vdopendir(char *path);
*/
