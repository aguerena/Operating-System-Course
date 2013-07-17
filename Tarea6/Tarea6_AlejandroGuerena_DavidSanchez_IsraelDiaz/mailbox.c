#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#define CICLOS 10
#define KEY_FILE "my_mailbox" //File for mailbox should be defined on this path


struct msg_buf {
  long msg_type;
  char msg_text[256];
};

char *pais[3]={"Peru","Bolivia","Colombia"};

int mailbox_rm(int msqid);
void check_for_send(int idmbq);
void mailbox_send(int idmbq, int typenum);
void mailbox_receive(int idmbq, int typenum);


void proceso(int i, int idmbq)
{
    int k;
    char msg_received[6];

    for(k=0;k<CICLOS;k++)
    {
   
	//WAIT TO RECEIVE AVAIL MESSAGE
	//strcpy(msg_received,rcv_msg(local_msqid,i));
        printf("Llega el pais: %s\n", pais[i]);
        mailbox_receive(idmbq, 1);

        printf("--Entra %s %d\n",pais[i], k);
        fflush(stdout);
        sleep(rand()%3);
        printf("**Sale  %s %d\n",pais[i], k);

        //SEND MSG
        mailbox_send(idmbq, 1);

        // Espera aleatoria fuera de la secciÃ³n crÃ­tica
        sleep(rand()%3);
    }

    exit(0); // Termina el proceso
}


int main()
{
	
    int tpid;
    int status;
    int i;
    int id_mailbox;

    // Generar Mailbox
    id_mailbox = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
 
    //INIT MAILBOX
    mailbox_send(id_mailbox, 1);

    srand(getpid());    

    for(i=0;i<3;i++)
    {
        // Crea un nuevo proceso hijo que ejecuta la funciÃ³n proceso()
        tpid=fork();
        if(tpid==0)
        {
            proceso(i, id_mailbox);
        }
    }

    for(i=0;i<3;i++)
        tpid = wait(&status);
    

    //RM MAILBOX
    printf("Removed = %d\n", mailbox_rm(id_mailbox));

}



void mailbox_send(int idmbq, int typenum)
{

    char msg[] = "OPEN";
    int msg_size;
    struct msg_buf my_msg_snd;

    my_msg_snd.msg_type = typenum;
    msg_size = strlen(msg) + 1;
    strcpy(my_msg_snd.msg_text,msg);

    if(msgsnd(idmbq, &my_msg_snd, msg_size, 0) == -1)
    {
        printf("Error en enviar mail box init(): %d\n", errno);
        exit(0);    
    }
    //printf("Mensaje enviado: %s\n",my_msg_snd.msg_text);
    
}

void mailbox_receive(int idmbq, int typenum)
{

    char msg[] = "OPEN";
    int msg_size;
    struct msg_buf my_msg_rcv;

    msg_size = strlen(msg) + 1;
            
    if(msgrcv(idmbq, &my_msg_rcv, msg_size, typenum, 0) == -1)
    {
        printf("Error en recibir de mail box init(): %d\n", errno);
        exit(0);    
    }
    //printf("Mensaje recibido: %s \n", my_msg_rcv.msg_text);
	
}


int mailbox_rm(int msqid)
{
    int msqid_rm;

    msqid_rm = msgctl(msqid, IPC_RMID, NULL);
    remove("my_mailbox");
    return(msqid_rm);
}








