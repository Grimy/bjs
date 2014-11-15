#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DECK_SIZE 312
#define value(x) ((x) & 31)
#define softness(x) ((x) & ~31)

// < 0% : 5vA 6vA 14v10 12v4
// < .5% : 16v9 11v10 A7v2 7vA A2v5
// < 2% : 13v2 12v3 12v4 12v6 12vA 9v2 8v6 7v2 A5v3 A3v4
static int esp[3];
static bool picked[DECK_SIZE];
static bool should_hit[12][96] = {{0}};
static int hand, bank;
static int hand_fst, hand_snd, bank_fst;

static void add_card(int *hand, int card) {
	picked[card] = true;
	card = card >= 216 ? 10 : 1 + card / 24;
	*hand += card + 42 * (card == 1);
	if (value(*hand) > 21)
		*hand = softness(*hand) ? *hand - 42 : 0;
}

static void hit(int *hand) {
	static int card;
	while (picked[card = (int) (drand48() * DECK_SIZE)]);
	add_card(hand, card);
}

static void end_round(int multiplier) {
	while (bank && value(bank) < 17)
		hit(&bank);
	esp[multiplier] += (multiplier == 2 ? 2 : 1) * (value(hand) > value(bank));
	esp[multiplier] -= (multiplier == 2 ? 2 : 1) * (value(hand) < value(bank) || !hand);
	memset(picked, 0, sizeof(picked));
	hand = bank = 0;
	add_card(&hand, hand_fst * 24 - 1);
	add_card(&hand, hand_snd * 24 - 2);
	add_card(&bank, bank_fst * 24 - 3);
}

static void play() {
	int i;
	end_round(0);
	memset(esp, 0, sizeof(esp));
	for (i = 0; i < 1E7; ++i) {
		end_round(0);
		hit(&hand);
		while (should_hit[value(bank)][hand])
			hit(&hand);
		end_round(1);
		hit(&hand);
		end_round(2);
	}
	should_hit[value(bank)][hand] = esp[1] > esp[0];
	printf("%2d%3d%6d\tS: %-11dH: %-11dD: %d\n", hand_fst, hand_snd, bank_fst,
		esp[0], esp[1], esp[2]);
}

int main(void) {
	srand48(time(NULL));
	int sum;
	for (sum = 20; sum > 1; --sum)
		for (hand_fst = sum / 2; hand_fst && hand_fst > sum - 11; --hand_fst)
			for (bank_fst = 1; bank_fst < 11; ++bank_fst)
				hand_snd = sum - hand_fst, play();
	return 0;
}
