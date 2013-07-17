exec: \
compile
	./init

compile: \
init.c sh.c getty.c
	gcc -o init init.c
	gcc -o sh sh.c
	gcc -o getty getty.c
	echo "Compiling init.c sh.c getty.c"
	touch compile

