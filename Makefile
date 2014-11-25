CC = gcc -Wall -Wextra --std=gnu99 -march=native -Ofast

.PHONY: run
run: bjs
	./$<

bjs: bjs.c Makefile
	$(CC) -fprofile-generate $< -o $@
	./$@
	$(CC) -fprofile-use $< -o $@

bjs.s: bjs.c Makefile
	$(CC) -fprofile-use -S $<

.PHONY: stat
stat: bjs
	perf stat -d -etask-clock -epage-faults -ecycles -einstructions -erc8 -erc9 -ealignment-faults -er47 ./$<

.PHONY: report
report: bjs
	perf record -e LLC-load-misses ./$<
	perf report

.PHONY: debug
debug: a.out
	gdb ./$<

a.out: bjs.c Makefile
	$(CC) -Og -ggdb $<
