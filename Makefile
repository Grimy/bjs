CC = gcc --std=c99

.PHONY: run
run: bjs
	./$<

bjs: bjs.c
	$(CC) -Wall $< -o $@

.PHONY: prof
prof: gmon.out
	gprof --brief

gmon.out: a.out
	./$<

a.out: bjs.c
	$(CC) -pg -Wall $<

.PHONY: debug
debug: bjs-debug
	gdb bjs-debug

bjs-debug: bjs.c
	$(CC) -g -Wall $< -o $@

