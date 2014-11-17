#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define mv(a, b, i) --(a)[i]; ++(b)[i]
#define max(a, b) ((a) > (b) ? (a) : (b))

// < 0% : 5vA 6vA 14v10 12v4
// < .5% : 16v9 11v10 A7v2 7vA A2v5
// < 2% : 13v2 12v3 12v6 12vA 9v2 8v6 A6v2 A5v3 A3v4

static int deck[11] = { [1 ... 9] = 24, [10] = 96 };
static int hand[11] = {0};
static int bank[11] = {0};

static int hand_hash, hand_sum, hand_value, cards_left;
/* static double cache[33023][5] = {[0 ... 33022] = {[0 ... 4] = .0d / .0d}}; */

void update_hand() {
	int pos = 1 + hand[1] / 2;
	int card = 2;
	hand_hash = (hand[1] & 1) ^ ~(~0 << pos);
	hand_sum = hand[1];

	for (int i = 2; i < 11; i++) {
		hand_sum += i * hand[i];
		for (int j = 1; j < hand[i]; j++) {
			hand_hash |= 1 << (pos += i + 1 - card);
			card = i;
		}
	}
	hand_value = hand_sum + 10 * (hand[1] && hand_sum <= 11);
}

double eval_bank() {
	int sum = 0;
	for (int i = 1; i < 11; ++i)
		sum += i * bank[i];
	int val = sum + 10 * (bank[1] && sum <= 11);
	int blackjack = bank[1] && bank[10] && sum == 11;

	if (val > 21)
		return 1;
	if (val >= 17)
		return (hand_value > val) - (hand_value < val + blackjack);

	double result, omg = 42E10;
	cards_left--;
	for (int i = 1; i < 11; ++i) {
		mv(deck, bank, i);
		printf("", omg);
		result += (deck[i] + 1) * eval_bank();
		mv(bank, deck, i);
	}
	cards_left++;

	return result / cards_left;
}

double eval_hand(int moves) {
	update_hand();
	if (hand_value > 21)
		return -1;
	if (hand_value == 21 && hand_sum == 11) // Blackjack!
		return 1.5 * (cards_left - hand[bank[11] ? 10 : bank[10]]) / cards_left;

	/* double *exp = cache[hand_hash]; */
	double exp[5] = {[0 ... 4] = .0d / .0d};
	if (hand_hash > 33022) {
		printf("Hash too big: %d\n", hand_hash);
		exit(1);
	}
	if (exp[moves] == exp[moves])
		return exp[moves];
	// hand_sum % 2 == 0 && hand[hand_sum / 2] == 2

	exp[0] = eval_bank();
	if (hand_value > 18 || hand_sum > 17)
		return exp[3] = exp[2] = exp[1] = exp[0];

	cards_left--;
	double hit, dbl;
	for (int i = 1; i < 11; ++i) {
		mv(deck, hand, i);
		hit += (deck[i] + 1) * eval_hand(1);
		dbl += (deck[i] + 1) * 2 * eval_hand(0);
		mv(hand, deck, i);
	}
	cards_left++;

	exp[1] = max(exp[0], hit / cards_left);
	exp[2] = max(exp[1], dbl / cards_left);
	exp[3] = max(exp[2], -.5);
	return exp[moves];
}

int main(void) {
	mv(deck, hand, 10);
	mv(deck, hand, 8);
	mv(deck, bank, 1);
	cards_left = 309;
	printf("%f\n", eval_hand(3));
	return 0;
}
