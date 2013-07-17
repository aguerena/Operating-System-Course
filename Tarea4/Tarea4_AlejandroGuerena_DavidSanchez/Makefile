## all: fcfs rr proceso
# all: fcfs proceso 
# all: spn proceso
# all: rr proceso
# all: pq proceso
all: rr spn pq proceso

fcfs: virtual_processor.o schedulerFCFS.c
	gcc -o fcfs schedulerFCFS.c virtual_processor.o 

spn: virtual_processor.o schedulerSPN.c
	gcc -o spn schedulerSPN.c virtual_processor.o 

pq: virtual_processor.o schedulerPQ.c
	gcc -o pq schedulerPQ.c virtual_processor.o

proceso: proceso.c
	gcc -o proceso proceso.c

virtual_processor.o: virtual_processor.c virtual_processor.h
	gcc -c virtual_processor.c

rr: virtual_processor.o schedulerRR.c
	gcc -o rr schedulerRR.c virtual_processor.o


clean:
	rm spn
#	rm fcfs
	rm rr
	rm pq

