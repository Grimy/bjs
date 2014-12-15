CCFLAGS = -Wall -Wextra -Werror -std=gnu99 -march=native
OPTI = -fno-asynchronous-unwind-tables -Ofast
DEBUG =  -Og -ggdb -fsanitize=address,leak,undefined

.PHONY: run
run: bjs
	./$<

bjs: bjs.c Makefile
	$(CC) $(CCFLAGS) $(OPTI) -fprofile-generate $< -o $@
	./$@
	$(CC) $(CCFLAGS) $(OPTI) -fprofile-use $< -o $@

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
	$(CC) $(CCFLAGS) $(DEBUG) $<

bjs.s: bjs.c Makefile
	$(CC) $(CCFLAGS) $(OPTI) -S $<
# yasm -pgas -felf64 fibo.asm
