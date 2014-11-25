#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NDECKS 6
#define DOUBLE_AFTER_SPLIT 1
#define BLACKJACK_PAYS 1.5

#define FOREACH(dest, code) \
	cards_left--; \
	long save = dest; \
	for (long i = 1; i < 11; ++deck[i], ++i) { \
		--deck[i]; dest = cache[save][i]; code; \
	} dest = save; cards_left++;

#define max(a, b) ((a) > (b) ? (a) : (b))

// < 0% : 5vA 6vA 14v10 12v4
// < .5% : 16v9 11v10 A7v2 7vA A2v5
// < 2% : 13v2 12v3 12v6 12vA 9v2 8v6 A6v2 A5v3 A3v4

#define CACHE_SIZE 3084
#define BLACKJACK_HASH 2430

static long cache[CACHE_SIZE][12];
static double hand_cache[10][CACHE_SIZE][5] = {{{0}}};
static double bank_cache[CACHE_SIZE] = {0};
static long hand = 1;
static long bank = 1;

enum { SUM = 0, VALUE = 11 };
static long deck[11] = {[1 ... 9] = 4 * NDECKS, [10] = 4 * 4 * NDECKS};

static long cards_left = 52 * NDECKS;
static long bank_first;

static double eval_bank() {
	double *exp = &bank_cache[bank];
	if (*exp)
		return *exp;

	if (cache[bank][VALUE] >= 17)
		return *exp = (cache[hand][VALUE] > cache[bank][VALUE]) -
			(cache[hand][VALUE] < cache[bank][VALUE] + (bank == BLACKJACK_HASH));

	if (!bank)
		return *exp = 1;

	FOREACH(bank, *exp += (deck[i] + 1) * eval_bank());
	return *exp /= cards_left;
}

static double eval_hand(long moves) {
	double *exp = hand_cache[bank_first - 1][hand];
	if (exp[moves])
		return exp[moves];

	if (!hand)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0] = -1;

	if (hand == BLACKJACK_HASH)
		return BLACKJACK_PAYS * (cards_left -
			deck[(bank_first == 1) ? 10 : bank_first == 10]) / cards_left;

	memset(bank_cache, 0, sizeof(bank_cache));
	exp[0] = eval_bank();
	if (cache[hand][VALUE] > 18)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0];

	/* long can_split = hand[SUM] % 2 == 0 && hand[hand[SUM] / 2] == 2 ? hand[SUM] / 2 : 0; */
	/* long was_split = hand[SUM] < 11 && hand[hand[SUM]] ? hand[SUM] : 0; */

	double hit = 0, dbl = 0, split = -2;
	FOREACH(hand,
		/* if (i != was_split) { */
		hit += (deck[i] + 1) * eval_hand(1);
		dbl += (deck[i] + 1) * 2 * eval_hand(0);
		/* } */
	)
	printf("%*s%ld\n", 312 - (int) cards_left, "", hand);

	/* if (can_split) { */
		/* --hand[can_split]; */
		/* split = 2 * eval_hand(2); */
		/* ++hand[can_split]; */
	/* } */

	/* double ratio = was_split ? (cards_left + 2 * deck[was_split]) : cards_left; */
	double ratio = cards_left;
	exp[1] = max(exp[0], hit / ratio);
	exp[2] = max(exp[1], dbl / ratio);
	exp[3] = max(exp[2], -.5);
	exp[4] = max(exp[3], split);
	/* printf("%*s%ld Hit: %f, Double: %f, Split: %f\n", 312 - cards_left, "", sum, */
			/* hit / (cards_left - 2 * deck[was_split]), dbl / (cards_left - 2 * deck[was_split]), split); */

	return exp[moves];
}

static void print_strat(long hand_first, long hand_snd) {
	char repr[] = "SHDRP";
	long hand = 1;
	hand = cache[hand][hand_first];
	hand = cache[hand][hand_snd];
	printf("\n%2ld %2ld", hand_first, hand_snd);

	for (long bank_first = 2; bank_first < 12; ++bank_first) {
		double *exp = hand_cache[bank_first == 11 ? 0 : bank_first - 1][hand];
		long max = 0;
		for (long i = 1; i < 5; i++)
			if (exp[i] > exp[max])
				max = i;
		printf(" %c", repr[max]);
	}
}

static long count = 1;
long fill_tree(long card, long sum) {
	if (sum > 21)
		return 0;
	long hash = count++;
	cache[hash][SUM] = sum;
	cache[hash][VALUE] = sum + 10 * (hash < 2441 && sum <= 11); // soft hand
	for (; card < 11; card++) {
		cache[hash][card] = fill_tree(card, sum + card);
	}
	return hash;
}

int main(void) {
	// 22v2, 22v3, 33v2, 33v3, 44v5, 44v6, 66v2
	/* bank_first = 2; */
	/* int fst = 6; */
	/* --deck[fst], ++hand[fst], --cards_left; */
	/* --deck[fst], ++hand[fst], --cards_left; */
	/* --deck[bank_first], ++bank[bank_first], --cards_left; */
	/* eval_hand(4); */
	/* return 0; */
	fill_tree(1, 0);

	FOREACH(bank,
		bank_first = i;
		FOREACH(hand, FOREACH(hand, eval_hand(4)))
	)
	for (long sum = 5; sum < 20; ++sum)
		print_strat((sum - 1) / 2, sum / 2 + 1);
	for (long i = 2; i < 11; ++i)
		print_strat(1, i);
	for (long i = 1; i < 11; ++i)
		print_strat(i, i);

	return 0;
}
