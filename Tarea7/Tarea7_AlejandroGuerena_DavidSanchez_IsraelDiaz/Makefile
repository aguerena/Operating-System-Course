all: procesos procesosVM procesosVM3 swap

procesos: procesos.c mmu.o pagefault.o
	cc -o procesos procesos.c mmu.o pagefault.o

procesosVM: procesos.c mmu.o pagefaultVM.o
	cc -o procesosVM procesos.c mmu.o pagefaultVM.o

procesosVM3: procesos.c mmu.o pagefaultVM3.o
	cc -o procesosVM3 procesos.c mmu.o pagefaultVM3.o

pagefault.o: pagefault.c mmu.h
	cc -c pagefault.c

pagefaultVM.o: pagefaultVM.c mmu.h
	cc -c pagefaultVM.c

pagefaultVM3.o: pagefaultVM3.c mmu.h
	cc -c pagefaultVM3.c

mmu.o: mmu.c semaphores.h mmu.h
	cc -c mmu.c

swap: 
	cc -o createswap createswap.c
	./createswap
