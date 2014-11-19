#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NDECKS 6
#define DOUBLE_AFTER_SPLIT 1

#define FOREACH(dest, code) \
	cards_left--; \
	for (int i = 1; i < 11; --dest[i], ++deck[i], ++i) { \
		--deck[i]; ++dest[i]; code; \
	} cards_left++;

#define max(a, b) ((a) > (b) ? (a) : (b))
#define CACHE_SIZE 9219

// < 0% : 5vA 6vA 14v10 12v4
// < .5% : 16v9 11v10 A7v2 7vA A2v5
// < 2% : 13v2 12v3 12v6 12vA 9v2 8v6 A6v2 A5v3 A3v4

enum { HASH = 0, ACE = 1, SUM = 11, VALUE = 12 };
static int deck[11] = {[1 ... 9] = 4 * NDECKS, [10] = 4 * 4 * NDECKS};
static int hand[13] = {0};
static int bank[13] = {0};

static int cards_left = 52 * NDECKS;
static int bank_first;
static double cache[10][CACHE_SIZE][5] = {{{0}}};
static double bank_cache[CACHE_SIZE] = {0};

void update_hand(int hand[13]) {
	int pos = 1 + hand[1] / 2;
	int card = 2;
	hand[HASH] = (hand[1] & 1) ^ ~(~0 << pos);
	hand[SUM] = hand[1];

	for (int i = 2; i < 11; i++) {
		hand[SUM] += i * hand[i];
		for (int j = 0; j < hand[i]; j++) {
			hand[HASH] |= 1 << (pos += i + 1 - card);
			card = i;
		}
	}
	hand[HASH] %= CACHE_SIZE;
	hand[VALUE] = hand[SUM] + 10 * (hand[1] && hand[SUM] <= 11);
}

double eval_bank() {
	update_hand(bank);
	if (bank[VALUE] > 21)
		return 1;
	if (bank_cache[bank[HASH]])
		return bank_cache[bank[HASH]];
	int blackjack = bank[ACE] && bank[10] && bank[SUM] == 11;
	if (bank[VALUE] >= 17)
		return (hand[VALUE] > bank[VALUE]) - (hand[VALUE] < bank[VALUE] + blackjack);

	double result = 0;
	FOREACH(bank, result += (deck[i] + 1) * eval_bank());
	return bank_cache[bank[HASH]] = result / cards_left;
}

double eval_hand(int moves) {
	update_hand(hand);
	if (hand[VALUE] > 21)
		return -1;
	if (hand[ACE] && hand[10] && hand[SUM] == 11) // Blackjack!
		return 1.5 * (cards_left - deck[bank[1] ? 10 : bank[10]]) / cards_left;

	double *exp = cache[bank_first - 1][hand[HASH]];
	if (exp[moves])
		return exp[moves];

	memset(bank_cache, 0, sizeof(bank_cache));
	exp[0] = eval_bank();
	if (hand[VALUE] > 18 || hand[SUM] > 17)
		return exp[4] = exp[3] = exp[2] = exp[1] = exp[0];

	int can_split = hand[SUM] % 2 == 0 && hand[hand[SUM] / 2] == 2 ? hand[SUM] / 2 : 0;
	int was_split = hand[SUM] < 11 && hand[hand[SUM]] ? hand[SUM] : 0;

	double hit = 0, dbl = 0, split = -2;
	FOREACH(hand,
		if (i == was_split)
			continue;
		hit += (deck[i] + 1) * eval_hand(1);
		dbl += (deck[i] + 1) * 2 * eval_hand(0);
	)
	if (can_split) {
		--hand[can_split];
		split = eval_hand(2);
		++hand[can_split];
	}

	exp[1] = max(exp[0], hit / (cards_left - 2 * deck[was_split]));
	exp[2] = max(exp[1], dbl / (cards_left - 2 * deck[was_split]));
	exp[3] = max(exp[2], -.5);
	exp[4] = max(exp[3], split);

	return exp[moves];
}

void print_strat(int hand_first, int hand_snd) {
	char repr[] = "SHDRP";
	++hand[hand_first];
	++hand[hand_snd];
	printf("\n%2d %2d", hand_first, hand_snd);

	update_hand(hand);
	for (int bank_first = 2; bank_first < 12; ++bank_first) {
		double *exp = cache[bank_first == 11 ? 0 : bank_first - 1][hand[HASH]];
		int max = 0;
		for (int i = 1; i < 5; i++)
			if (exp[i] > exp[max])
				max = i;
		printf(" %c", repr[max]);
	}

	--hand[hand_first];
	--hand[hand_snd];
}

int main(void) {
	int fst;
	FOREACH(bank,
		bank_first = i;
		FOREACH(hand, eval_hand(4));
		FOREACH(hand, fst = i; FOREACH(hand,
			printf("%d %d %d %f\n", bank_first, fst, i, eval_hand(4));
		))
	)

	/* for (int sum = 5; sum < 20; ++sum) */
		/* print_strat((sum - 1) / 2, sum / 2 + 1); */
	/* for (int i = 2; i < 11; ++i) */
		/* print_strat(1, i); */
	/* for (int i = 1; i < 11; ++i) */
		/* print_strat(i, i); */

	return 0;
}
