
import math

func count_mismatches(a, b)
	if (type(a) == "tuple" or type(a) == "list") and (type(b) == "tuple" or type(b) == "list")
		return math.sum(map(count_mismatches, a, b))
	return a != b

test = [
	(count_mismatches(range(5), [0, 1, 2, 3, 4]), 0),
	(count_mismatches(enumerate(range(5)), [(0, 0), (1, 1), (2, 2), (3, 3), (4, 4)]), 0),

	(count_mismatches(consecutive(range(5)), [(0, 1), (1, 2), (2, 3), (3, 4)]), 0),
	(count_mismatches(consecutive(range(5), 3), [(0, 1, 2), (1, 2, 3), (2, 3, 4)]), 0),

	(count_mismatches(zip(range(5), range(10, 15)), [(0, 10), (1, 11), (2, 12), (3, 13), (4, 14)]), 0),

	(count_mismatches(map((func(a) return a * a), range(5)), [0, 1, 4, 9, 16]), 0),
	(count_mismatches(map((func(a, b) return a + b), range(5), range(10, 15)), [10, 12, 14, 16, 18]), 0),

	(count_mismatches(filter(math.even, range(5)), range(0, 5, 2)), 0),
	(count_mismatches(filter(math.odd, range(5)), range(1, 5, 2)), 0),

	(count_mismatches(map(all, [[], [0], [1], [0, 0], [0, 1], [1, 0], [1, 1]]), [1, 0, 1, 0, 0, 0, 1]), 0),
	(count_mismatches(map(any, [[], [0], [1], [0, 0], [0, 1], [1, 0], [1, 1]]), [0, 0, 1, 0, 1, 1, 1]), 0),
]

for i, (actual, expected) in enumerate(test)
	assert type(actual) == type(expected) and actual == expected, i + ": " + actual + " == " + expected

assert (any([]), any([0]), any([1]), any([0, 0]), any([0, 1]), any([1, 0]), any([1, 1])) == (0, 0, 1, 0, 1, 1, 1)
assert (all([]), all([0]), all([1]), all([0, 0]), all([0, 1]), all([1, 0]), all([1, 1])) == (1, 0, 1, 0, 0, 0, 1)
