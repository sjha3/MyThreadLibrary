ptrmake:
	gcc -c -o mythread.o mythread.c
	ar rcs mythread.a mythread.o
