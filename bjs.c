#include <assert.h>
#include <stdio.h>
#include <string.h>

#define NDECKS 6
#define DOUBLE_AFTER_SPLIT 1
#define BLACKJACK_PAYS 1.5

#define CACHE_SIZE (3084)

#define weighted_avg(dest, code) __extension__ ({ \
	cards_left--; long save = dest; double result = 0; \
	for (long i = 1; i < 11; ++deck[i], ++i) { \
		dest = cache[save][i]; result += (double) deck[i]-- * ({code;}); \
	} dest = save; result / ++cards_left; })

#define max(a, b) ((a) > (b) ? (a) : (b))
#define isnan(x) ((x) != (x)) // faster than the one from math.h

/* Returns 1 if a > b, -1 if a < b, and 0 if a == b.
 * Same effect as (a > b) - (a < b), but about 2% faster. */
static inline double cmp(long a, long b) {
	double result;
	__asm__ volatile("cmpq %2, %1; seta %b1; sbb $0, %k1; cvtsi2sd %k1, %0"
			: "=x"(result), "+r"(a) : "r"(b));
	return result;
}

enum { VALUE = 0, CAN_SPLIT = 11, WAS_SPLIT = 12 };
static long cache[CACHE_SIZE][13];
static double hand_cache[10][CACHE_SIZE][5];
static double bank_cache[CACHE_SIZE] = {1};
static long hand = 1;
static long bank = 1;

__extension__ static long deck[11] = { [1 ... 9] = 4 * NDECKS, 4 * 4 * NDECKS};

static int cards_left = 52 * NDECKS;
static long bank_first = 0;

static double eval_bank(void) {
	if (isnan(bank_cache[bank]))
		bank_cache[bank] = cache[bank][VALUE] >= 17
			? cmp(cache[hand][VALUE], cache[bank][VALUE])
			: weighted_avg(bank, eval_bank());
	return bank_cache[bank];
}

static double eval_hand(long moves) {
	double *exp = hand_cache[bank_first - 1][hand];
	if (exp[moves])
		return exp[moves];

	if (!hand)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0] = -1;

	if (cache[hand][VALUE] == 22)
		return BLACKJACK_PAYS * (double) (cards_left -
			deck[(bank_first == 1) ? 10 : bank_first == 10]) / cards_left;

	memset(bank_cache + 1, 255, sizeof(bank_cache) - sizeof(*bank_cache));

	exp[0] = eval_bank();
	if (cache[hand][VALUE] > 18)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0];

	long was_split = cache[hand][WAS_SPLIT];
	long das = 1 + (was_split && DOUBLE_AFTER_SPLIT);
	double hit = weighted_avg(hand, cache[hand][CAN_SPLIT] ? .0 : eval_hand(das));
	double dbl = weighted_avg(hand, cache[hand][CAN_SPLIT] ? .0 : 2 * eval_hand(0));
	double split = -2;
	if (cache[hand][CAN_SPLIT]) {
		long save = hand;
		hand = cache[1][cache[hand][CAN_SPLIT]];
		split = 2 * eval_hand(1);
		hand = save;
	}

	double ratio = was_split ? (cards_left - 2 * (double) deck[was_split]) / cards_left : 1.0;
	exp[1] = max(exp[0], hit / ratio);
	exp[2] = max(exp[1], dbl / ratio);
	exp[3] = max(exp[2], -.5);
	exp[4] = max(exp[2], split);
	return exp[moves];
}

static void print_strat(long hand_first, long hand_snd) {
	char repr[] = "SHDRP";
	hand = 1;
	hand = cache[hand][hand_first];
	hand = cache[hand][hand_snd];
	printf("%2ld %2ld", hand_first, hand_snd);
	for (bank_first = 1; bank_first < 11; ++bank_first) {
		double *exp = hand_cache[bank_first % 10][hand];
		long max = 0;
		for (long i = 1; i < 5; i++)
			if (exp[i] > exp[max])
				max = i;
		printf(" %c", repr[max]);
	}
	printf("\n");
}

static void fill_cache(long prev_hash, long sum, long new_card) {
	static long count = 0;
	static long hand[11] = {0};
	
	long hash = ++count;
	cache[prev_hash][new_card] = hash;
	cache[hash][VALUE] = sum + 10 * (hand[1] && sum <= 11); // soft hand
	cache[hash][VALUE] += hand[1] && hand[10] && sum == 11; // blackjack
	cache[hash][CAN_SPLIT] = sum % 2 == 0 && hand[sum / 2] == 2 ? sum / 2 : 0;
	cache[hash][WAS_SPLIT] = sum < 11 && hand[sum] ? sum : 0;

	long i = 1;
	for (; i < new_card && sum + i <= 21; ++i) {
		long new_hash = 1;
		for (long j = 1; j < 11; ++j)
			for (long k = 0; k < hand[j] + (j == i); ++k)
				new_hash = cache[new_hash][j];
		cache[hash][i] = new_hash;
	}

	for (; i < 11 && sum + i <= 21; ++i) {
		++hand[i];
		fill_cache(hash, sum + i, i);
		--hand[i];
	}
}

int main(void) {
	fill_cache(0, 0, 0);
	double expected_gain = weighted_avg(bank,
		bank_first = i;
		weighted_avg(hand, weighted_avg(hand, eval_hand(4)))
	);
	printf("Expected gain: %f%%\n", expected_gain * 100);
	for (long sum = 5; sum < 20; ++sum)
		print_strat((sum - 1) / 2, sum / 2 + 1);
	for (long i = 2; i < 11; ++i)
		print_strat(1, i);
	for (long i = 1; i < 11; ++i)
		print_strat(i, i);

	return 0;
}
