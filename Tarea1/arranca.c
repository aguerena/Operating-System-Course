#include <stdio.h>
#include <sys/time.h>


int main()
{
    long long start_ts;
    long long stop_ts;
    long long elapsed_time;
    long lElapsedTime;
    struct timeval ts;

    int pid;
    int i,procs,pcpu;
    char spcpu[5];
    int status;
    float tasa;

    printf("Procesos ............ :");
    scanf("%d",&procs);

    printf("Porcentaje uso de CPU :"); 
    scanf("%d",&pcpu);
    sprintf(spcpu,"%d",pcpu);

    gettimeofday(&ts, NULL);
    start_ts = ts.tv_sec; // Tiempo inicial

    for(i=0;i<procs;i++)
    {
        pid=fork();
        if(pid==0)
            execlp("nice","nice","--adjustment=0","./cpuyes",spcpu,0);
    }

    for(i=0;i<procs;i++)
        wait(&status);

    gettimeofday(&ts, NULL);

    stop_ts = ts.tv_sec; // Tiempo final
    elapsed_time = stop_ts - start_ts;

    printf("------------------------------\n");
    printf("TIEMPO TOTAL, %d segundos\n",elapsed_time);
    tasa=(float) procs / (float) elapsed_time;
    printf("Tasa de trabajos = %1.4f\n",tasa);

}
