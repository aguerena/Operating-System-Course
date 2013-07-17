#include <fcntl.h>

main()
{
    int fd;
    char buffer[16*4096];

    fd=creat("swap",0666); 
    write(fd,buffer,16*4096);
    close(fd);
}

    //fd=creat("a.txt",0640);
    //fd=creat("a.txt", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
