
import math

test = [
	(null, false),

	(0, false),
	(1, true),
	(2, true),
	(-2, true),

	(0.0, false),
	(1.0, true),
	(2.0, true),
	(-2.0, true),
	(0.01, true),
	(-0.01, true),

	("", false),
	(" ", true),
	("Test", true),

	((), false),
	((1,), true),
	((1, 2), true),

	([], false),
	([1], true),
	([1, 2], true),

	({}, false),
	({"a": 1}, true),
	({"a": 1, "b": 2}, true),

	(print, true),
	(bool, true),

	((func()), true),
	((proc()), true),

	(math, true),
]

for i, (actual, expected) in enumerate(test)
	assert bool(actual) == expected, i + ": " + bool(actual) + " == " + expected
	assert not bool(actual) == not expected, i + ": " + not bool(actual) + " == " + not expected
	assert not actual == not expected, i + ": " + not actual + " == " + not expected
