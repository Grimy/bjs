#include <assert.h>
#include <stdio.h>
#include <string.h>

#define NDECKS 6
#define DOUBLE_AFTER_SPLIT 1
#define BLACKJACK_PAYS 1.5

#define CACHE_SIZE (3083)

#define mean(dest, code) __extension__ ({ \
	cards_left--; double result = 0; long *p = cache[dest].next; \
	for (long i = 1; i < 11; ++deck[i], ++i) { \
		long dest = p[i]; result += (double) deck[i]-- * ({code;}); \
	} result / ++cards_left; })

#define max(a, b) ((a) > (b) ? (a) : (b))
#define isnan(x) ((x) != (x)) // faster than the one from math.h
#define eval_bank() _eval_bank(hand, bank)
#define eval_hand(moves) _eval_hand(hand, bank, moves)

/* Returns 1 if a > b, -1 if a < b, and 0 if a == b.
 * Same effect as (a > b) - (a < b), but about 2% faster. */
static inline double cmp(long a, long b) {
	double result;
	__asm__ volatile("cmpq %2, %1; seta %b1; sbb $0, %k1; cvtsi2sd %k1, %0"
			: "=x"(result), "+r"(a) : "r"(b));
	return result;
}

struct hand {
	long next[11];
	long value;
	long can_split;
};
static struct hand cache[CACHE_SIZE];
static double hand_cache[CACHE_SIZE][5];
static double bank_cache[CACHE_SIZE] = {1};

__extension__ static long deck[11] = {[1 ... 9] = 4 * NDECKS, 4 * 4 * NDECKS};

static int cards_left = 52 * NDECKS;

static double _eval_bank(long hand, long bank) {
	if (isnan(bank_cache[bank]))
		bank_cache[bank] = cache[bank].value >= 17
			? cmp(cache[hand].value, cache[bank].value)
			: mean(bank, eval_bank());
	return bank_cache[bank];
}

static double _eval_hand(long hand, long bank, long moves) {
	// Look in the cache
	double *exp = hand_cache[hand];
	if (exp[moves])
		return exp[moves];

	// Busted!
	if (!hand)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0] = -1;

	// Stay
	memset(bank_cache + 1, 255, sizeof(bank_cache) - sizeof(*bank_cache));
	exp[0] = eval_bank() * (cache[hand].value == 22 ? BLACKJACK_PAYS : 1);
	if (moves == 0)
		return exp[0];
	if (cache[hand].value > 18)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0];

	// Hit
	long das = hand < 11 && DOUBLE_AFTER_SPLIT;
	double hit = mean(hand, cache[hand].can_split ? .0 : eval_hand(1 + das));
	double dbl = mean(hand, cache[hand].can_split ? .0 : 2 * eval_hand(0));
	double split = 2 * hand_cache[cache[hand].can_split][1];

	double ratio = (cards_left - (hand < 11 ? 2.0 * deck[hand] : 0.0)) / cards_left;
	exp[1] = max(exp[0], hit / ratio);
	exp[2] = max(exp[1], dbl / ratio);
	exp[3] = max(exp[2], -.5);
	exp[4] = max(exp[3], split);
	return exp[moves];
}

static void fill_cache(long hash, long sum, long new_card) {
	static long count;
	static long hand[11];
	
	long soft = hand[1] && sum <= 11;
	cache[hash].value = sum + 10 * soft + (soft && hand[10]);
	cache[hash].can_split = sum % 2 == 0 && hand[sum / 2] == 2 ? sum / 2 : 0;

	for (long i = 1; i < new_card && sum + i <= 21; ++i)
		for (long j = 1; j < 11; ++j)
			for (long k = 0; k < hand[j] + (j == i); ++k)
				cache[hash].next[i] = cache[cache[hash].next[i]].next[j];

	for (long i = new_card; i < 11 && sum + i <= 21; ++i)
		cache[hash].next[i] = ++count;

	for (long i = new_card; i < 11 && sum + i <= 21; ++i) {
		++hand[i];
		fill_cache(cache[hash].next[i], sum + i, i);
		--hand[i];
	}
}

static void print_strat(long bank, long hand_first, long hand_snd) {
	char repr[] = "SHDRP";
	double *exp = hand_cache[cache[hand_first].next[hand_snd]];
	long max = 0;
	for (long i = 1; i < 5; i++)
		if (exp[i] > exp[max])
			max = i;
	printf("\033[%ldG%c\n", bank == 1 ? 22 : bank * 2, repr[max]);
}

static double halfmain(long bank) {
	long hand = 0;
	memset(hand_cache, 0, sizeof(hand_cache));
	mean(hand, eval_hand(1));
	double result = mean(hand, mean(hand, eval_hand(4)));
	for (long sum = 5; sum < 20; ++sum)
		print_strat(bank, (sum - 1) / 2, sum / 2 + 1);
	for (long i = 2; i < 11; ++i)
		print_strat(bank, 1, i);
	for (long i = 1; i < 11; ++i)
		print_strat(bank, i, i);
	printf("\033[34A");
	return result;
}

int main(void) {
	fill_cache(0, 0, 1);
	long bank = 0;
	double expected_gain = mean(bank, halfmain(bank));
	printf("\033[34BExpected gain: %f%%\n", expected_gain * 100);
	return 0;
}
