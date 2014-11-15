run: bjs
	./bjs

bjs: bjs.c
	gcc -Wall bjs.c -o bjs

prof: gmon.out
	gprof --brief

gmon.out: a.out
	./a.out

a.out: bjs.c
	gcc -pg -Wall bjs.c
