CC = gcc -Wall -Wextra -std=gnu99 -march=native -Ofast -ggdb

.PHONY: run
run: bjs
	./$<

bjs: bjs.c Makefile
	$(CC) -fprofile-generate $< -o $@
	./$@
	$(CC) -fprofile-use $< -o $@

.PHONY: stat
stat: bjs
	perf stat -d -etask-clock -epage-faults -ecycles -einstructions -erc8 -erc9 -ealignment-faults -er47 ./$<

.PHONY: report
report: bjs
	perf record ./$<
	perf report

.PHONY: debug
debug: a.out
	gdb ./$<

a.out: bjs.c Makefile
	$(CC) -Og $<

# yasm -pgas -felf64 fibo.asm
