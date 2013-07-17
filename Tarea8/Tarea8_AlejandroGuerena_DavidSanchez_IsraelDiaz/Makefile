all: createvd dumpsec dumpseclog vdformat shell1

createvd: createvd.c vdisk.o
	cc -o createvd createvd.c vdisk.o

dumpsec: dumpsec.c vdisk.o 
	cc -o dumpsec dumpsec.c vdisk.o

dumpseclog: dumpseclog.c lowlvldisk.o vdisk.o 
	cc -o dumpseclog dumpseclog.c lowlvldisk.o vdisk.o 

vdformat: vdformat.c vdisk.o lowlvldisk.o 
	cc -o vdformat vdformat.c vdisk.o lowlvldisk.o

shell1: shell1.c highlvldisk.o lowlvldisk.o vdisk.o 
	cc -o shell1 shell1.c highlvldisk.o lowlvldisk.o vdisk.o     

vdisk.o: vdisk.c vdisk.h
	cc -c vdisk.c	

lowlvldisk.o: lowlvldisk.c lowlvldisk.h
	cc -c lowlvldisk.c

highlvldisk.o: highlvldisk.c highlvldisk.h
	cc -c highlvldisk.c	 
