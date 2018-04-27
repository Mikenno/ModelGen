
import math

test = [
	(reduce((func(a, b) return (b, a)[a > b]), range(1, 6)), 5),
	(reduce((func(a, b) return (b, a)[a < b]), range(1, 6)), 1),
	(reduce((func(a, b) return a + b), range(1, 6)), 15),

	(math.max(1, 2), 2),
	(math.max(1, -2), 1),
	(math.max(1, 2, 3), 3),
	(math.max(1, 2, -2, 3), 3),
	(math.max([1, 2, 3, 4, 5]), 5),
	(math.max([1, 2, 3, 4, -4, 5]), 5),

	(math.max(1, 2.0), 2.0),
	(math.max(1, -2.0), 1.0),
	(math.max(1, 2, 3.0), 3.0),
	(math.max(1, 2, -2.0, 3), 3.0),
	(math.max([1, 2, 3, 4.0, 5]), 5.0),
	(math.max([1, 2, 3, 4, -4.0, 5]), 5.0),

	(math.min(1, 2), 1),
	(math.min(1, -2), -2),
	(math.min(1, 2, 3), 1),
	(math.min(1, 2, -2, 3), -2),
	(math.min([1, 2, 3, 4, 5]), 1),
	(math.min([1, 2, 3, 4, -4, 5]), -4),

	(math.min(1, 2.0), 1.0),
	(math.min(1, -2.0), -2.0),
	(math.min(1, 2, 3.0), 1.0),
	(math.min(1, 2, -2.0, 3), -2.0),
	(math.min([1, 2, 3, 4.0, 5]), 1.0),
	(math.min([1, 2, 3, 4, -4.0, 5]), -4.0),

	(math.sum([]), 0),
	(math.sum([0]), 0),
	(math.sum([1]), 1),
	(math.sum([1, 2]), 3),
	(math.sum([1, 2, 3]), 6),
	(math.sum([1, 2, 3, 4]), 10),
	(math.sum([1, 2, 3, 4, 5]), 15),

	(math.clamp(-10, -5, 5), -5),
	(math.clamp(0, -5, 5), 0),
	(math.clamp(10, -5, 5), 5),

	(math.clamp(-10.0, -5, 5), -5.0),
	(math.clamp(0.0, -5, 5), 0.0),
	(math.clamp(10.0, -5, 5), 5.0),

	(math.clamp(-10, -5.0, 5), -5.0),
	(math.clamp(0, -5.0, 5), 0.0),
	(math.clamp(10, -5.0, 5), 5.0),

	(math.clamp(-10, -5, 5.0), -5.0),
	(math.clamp(0, -5, 5.0), 0.0),
	(math.clamp(10, -5, 5.0), 5.0),

	(math.normalize(50, 255), 0.196078),
	(math.normalize(50, 0, 255), 0.196078),
	(math.normalize(50, 100), 0.5),
	(math.normalize(50, 0, 100), 0.5),
	(math.normalize(0, -100, 100), 0.5),

	(math.lerp(0, 100, 0.5), 50.0),
	(math.lerp(0.0, 100, 0.5), 50.0),
	(math.lerp(0, 100.0, 0.5), 50.0),
	(math.lerp(0.0, 100.0, 0.5), 50.0),

	(math.lerp(0, 100, 0), 0.0),
	(math.lerp(0, 100, 1), 100.0),

	(math.map(50, 0, 100, 0, 1), 0.5),

	(math.nearest(0, 0, 5), 0),
	(math.nearest(1, 0, 5), 0),
	(math.nearest(2, 0, 5), 0),
	(math.nearest(3, 0, 5), 5),
	(math.nearest(4, 0, 5), 5),
	(math.nearest(5, 0, 5), 5),

	(math.nearest(0.0, 0, 5), 0.0),
	(math.nearest(1.0, 0, 5), 0.0),
	(math.nearest(2.0, 0, 5), 0.0),
	(math.nearest(3.0, 0, 5), 5.0),
	(math.nearest(4.0, 0, 5), 5.0),
	(math.nearest(5.0, 0, 5), 5.0),

	(math.nearest(0, 0.0, 5), 0.0),
	(math.nearest(1, 0.0, 5), 0.0),
	(math.nearest(2, 0.0, 5), 0.0),
	(math.nearest(3, 0.0, 5), 5.0),
	(math.nearest(4, 0.0, 5), 5.0),
	(math.nearest(5, 0.0, 5), 5.0),

	(math.nearest(0, 0, 5.0), 0.0),
	(math.nearest(1, 0, 5.0), 0.0),
	(math.nearest(2, 0, 5.0), 0.0),
	(math.nearest(3, 0, 5.0), 5.0),
	(math.nearest(4, 0, 5.0), 5.0),
	(math.nearest(5, 0, 5.0), 5.0),

	(math.snap(0.0, 3), 0.0),
	(math.snap(0.25, 3), 0.0),
	(math.snap(0.5, 3), 0.0),
	(math.snap(0.75, 3), 0.0),
	(math.snap(1.0, 3), 0.0),
	(math.snap(1.25, 3), 0.0),
	(math.snap(1.5, 3), 3.0),
	(math.snap(1.75, 3), 3.0),
	(math.snap(2.0, 3), 3.0),
	(math.snap(2.25, 3), 3.0),
	(math.snap(2.5, 3), 3.0),
	(math.snap(2.75, 3), 3.0),
	(math.snap(3.0, 3), 3.0),
	(math.snap(3.25, 3), 3.0),
	(math.snap(3.5, 3), 3.0),
	(math.snap(3.75, 3), 3.0),
	(math.snap(4.0, 3), 3.0),
	(math.snap(4.25, 3), 3.0),
	(math.snap(4.5, 3), 6.0),
	(math.snap(4.75, 3), 6.0),
	(math.snap(5.0, 3), 6.0),
	(math.snap(5.25, 3), 6.0),
	(math.snap(5.5, 3), 6.0),
	(math.snap(5.75, 3), 6.0),

	(math.snap_ceil(0.0, 3), 0.0),
	(math.snap_ceil(0.25, 3), 3.0),
	(math.snap_ceil(0.5, 3), 3.0),
	(math.snap_ceil(0.75, 3), 3.0),
	(math.snap_ceil(1.0, 3), 3.0),
	(math.snap_ceil(1.25, 3), 3.0),
	(math.snap_ceil(1.5, 3), 3.0),
	(math.snap_ceil(1.75, 3), 3.0),
	(math.snap_ceil(2.0, 3), 3.0),
	(math.snap_ceil(2.25, 3), 3.0),
	(math.snap_ceil(2.5, 3), 3.0),
	(math.snap_ceil(2.75, 3), 3.0),
	(math.snap_ceil(3.0, 3), 3.0),
	(math.snap_ceil(3.25, 3), 6.0),
	(math.snap_ceil(3.5, 3), 6.0),
	(math.snap_ceil(3.75, 3), 6.0),
	(math.snap_ceil(4.0, 3), 6.0),
	(math.snap_ceil(4.25, 3), 6.0),
	(math.snap_ceil(4.5, 3), 6.0),
	(math.snap_ceil(4.75, 3), 6.0),
	(math.snap_ceil(5.0, 3), 6.0),
	(math.snap_ceil(5.25, 3), 6.0),
	(math.snap_ceil(5.5, 3), 6.0),
	(math.snap_ceil(5.75, 3), 6.0),

	(math.snap_floor(0.0, 3), 0.0),
	(math.snap_floor(0.25, 3), 0.0),
	(math.snap_floor(0.5, 3), 0.0),
	(math.snap_floor(0.75, 3), 0.0),
	(math.snap_floor(1.0, 3), 0.0),
	(math.snap_floor(1.25, 3), 0.0),
	(math.snap_floor(1.5, 3), 0.0),
	(math.snap_floor(1.75, 3), 0.0),
	(math.snap_floor(2.0, 3), 0.0),
	(math.snap_floor(2.25, 3), 0.0),
	(math.snap_floor(2.5, 3), 0.0),
	(math.snap_floor(2.75, 3), 0.0),
	(math.snap_floor(3.0, 3), 3.0),
	(math.snap_floor(3.25, 3), 3.0),
	(math.snap_floor(3.5, 3), 3.0),
	(math.snap_floor(3.75, 3), 3.0),
	(math.snap_floor(4.0, 3), 3.0),
	(math.snap_floor(4.25, 3), 3.0),
	(math.snap_floor(4.5, 3), 3.0),
	(math.snap_floor(4.75, 3), 3.0),
	(math.snap_floor(5.0, 3), 3.0),
	(math.snap_floor(5.25, 3), 3.0),
	(math.snap_floor(5.5, 3), 3.0),
	(math.snap_floor(5.75, 3), 3.0),

	(math.snap(0.0, 3, 1), 1.0),
	(math.snap(0.25, 3, 1), 1.0),
	(math.snap(0.5, 3, 1), 1.0),
	(math.snap(0.75, 3, 1), 1.0),
	(math.snap(1.0, 3, 1), 1.0),
	(math.snap(1.25, 3, 1), 1.0),
	(math.snap(1.5, 3, 1), 1.0),
	(math.snap(1.75, 3, 1), 1.0),
	(math.snap(2.0, 3, 1), 1.0),
	(math.snap(2.25, 3, 1), 1.0),
	(math.snap(2.5, 3, 1), 4.0),
	(math.snap(2.75, 3, 1), 4.0),
	(math.snap(3.0, 3, 1), 4.0),
	(math.snap(3.25, 3, 1), 4.0),
	(math.snap(3.5, 3, 1), 4.0),
	(math.snap(3.75, 3, 1), 4.0),
	(math.snap(4.0, 3, 1), 4.0),
	(math.snap(4.25, 3, 1), 4.0),
	(math.snap(4.5, 3, 1), 4.0),
	(math.snap(4.75, 3, 1), 4.0),
	(math.snap(5.0, 3, 1), 4.0),
	(math.snap(5.25, 3, 1), 4.0),
	(math.snap(5.5, 3, 1), 7.0),
	(math.snap(5.75, 3, 1), 7.0),

	(math.snap_ceil(0.0, 3, 1), 1.0),
	(math.snap_ceil(0.25, 3, 1), 1.0),
	(math.snap_ceil(0.5, 3, 1), 1.0),
	(math.snap_ceil(0.75, 3, 1), 1.0),
	(math.snap_ceil(1.0, 3, 1), 1.0),
	(math.snap_ceil(1.25, 3, 1), 4.0),
	(math.snap_ceil(1.5, 3, 1), 4.0),
	(math.snap_ceil(1.75, 3, 1), 4.0),
	(math.snap_ceil(2.0, 3, 1), 4.0),
	(math.snap_ceil(2.25, 3, 1), 4.0),
	(math.snap_ceil(2.5, 3, 1), 4.0),
	(math.snap_ceil(2.75, 3, 1), 4.0),
	(math.snap_ceil(3.0, 3, 1), 4.0),
	(math.snap_ceil(3.25, 3, 1), 4.0),
	(math.snap_ceil(3.5, 3, 1), 4.0),
	(math.snap_ceil(3.75, 3, 1), 4.0),
	(math.snap_ceil(4.0, 3, 1), 4.0),
	(math.snap_ceil(4.25, 3, 1), 7.0),
	(math.snap_ceil(4.5, 3, 1), 7.0),
	(math.snap_ceil(4.75, 3, 1), 7.0),
	(math.snap_ceil(5.0, 3, 1), 7.0),
	(math.snap_ceil(5.25, 3, 1), 7.0),
	(math.snap_ceil(5.5, 3, 1), 7.0),
	(math.snap_ceil(5.75, 3, 1), 7.0),

	(math.snap_floor(0.0, 3, 1), -2.0),
	(math.snap_floor(0.25, 3, 1), -2.0),
	(math.snap_floor(0.5, 3, 1), -2.0),
	(math.snap_floor(0.75, 3, 1), -2.0),
	(math.snap_floor(1.0, 3, 1), 1.0),
	(math.snap_floor(1.25, 3, 1), 1.0),
	(math.snap_floor(1.5, 3, 1), 1.0),
	(math.snap_floor(1.75, 3, 1), 1.0),
	(math.snap_floor(2.0, 3, 1), 1.0),
	(math.snap_floor(2.25, 3, 1), 1.0),
	(math.snap_floor(2.5, 3, 1), 1.0),
	(math.snap_floor(2.75, 3, 1), 1.0),
	(math.snap_floor(3.0, 3, 1), 1.0),
	(math.snap_floor(3.25, 3, 1), 1.0),
	(math.snap_floor(3.5, 3, 1), 1.0),
	(math.snap_floor(3.75, 3, 1), 1.0),
	(math.snap_floor(4.0, 3, 1), 4.0),
	(math.snap_floor(4.25, 3, 1), 4.0),
	(math.snap_floor(4.5, 3, 1), 4.0),
	(math.snap_floor(4.75, 3, 1), 4.0),
	(math.snap_floor(5.0, 3, 1), 4.0),
	(math.snap_floor(5.25, 3, 1), 4.0),
	(math.snap_floor(5.5, 3, 1), 4.0),
	(math.snap_floor(5.75, 3, 1), 4.0),

	(math.snap_within(0, 5, 1), 0.0),
	(math.snap_within(1, 5, 1), 0.0),
	(math.snap_within(2, 5, 1), 2.0),
	(math.snap_within(3, 5, 1), 3.0),
	(math.snap_within(4, 5, 1), 5.0),
	(math.snap_within(5, 5, 1), 5.0),

	(math.snap_within(0, 5, 1, 1), 1.0),
	(math.snap_within(1, 5, 1, 1), 1.0),
	(math.snap_within(2, 5, 1, 1), 1.0),
	(math.snap_within(3, 5, 1, 1), 3.0),
	(math.snap_within(4, 5, 1, 1), 4.0),
	(math.snap_within(5, 5, 1, 1), 6.0),

	(math.wrap(-5, 2), 1),
	(math.wrap(-4, 2), 0),
	(math.wrap(-3, 2), 1),
	(math.wrap(-2, 2), 0),
	(math.wrap(-1, 2), 1),
	(math.wrap(0, 2), 0),
	(math.wrap(1, 2), 1),
	(math.wrap(2, 2), 0),
	(math.wrap(3, 2), 1),
	(math.wrap(4, 2), 0),
	(math.wrap(5, 2), 1),

	(math.wrap(-5, -1, 2), 0),
	(math.wrap(-4, -1, 2), 1),
	(math.wrap(-3, -1, 2), -1),
	(math.wrap(-2, -1, 2), 0),
	(math.wrap(-1, -1, 2), 1),
	(math.wrap(0, -1, 2), -1),
	(math.wrap(1, -1, 2), 0),
	(math.wrap(2, -1, 2), 1),
	(math.wrap(3, -1, 2), -1),
	(math.wrap(4, -1, 2), 0),
	(math.wrap(5, -1, 2), 1),

	(math.ping_pong(-5, 2), 1),
	(math.ping_pong(-4, 2), 0),
	(math.ping_pong(-3, 2), 1),
	(math.ping_pong(-2, 2), 2),
	(math.ping_pong(-1, 2), 1),
	(math.ping_pong(0, 2), 0),
	(math.ping_pong(1, 2), 1),
	(math.ping_pong(2, 2), 2),
	(math.ping_pong(3, 2), 1),
	(math.ping_pong(4, 2), 0),
	(math.ping_pong(5, 2), 1),

	(math.ping_pong(-5, 2, 4), 3),
	(math.ping_pong(-4, 2, 4), 2),
	(math.ping_pong(-3, 2, 4), 3),
	(math.ping_pong(-2, 2, 4), 4),
	(math.ping_pong(-1, 2, 4), 3),
	(math.ping_pong(0, 2, 4), 2),
	(math.ping_pong(1, 2, 4), 3),
	(math.ping_pong(2, 2, 4), 4),
	(math.ping_pong(3, 2, 4), 3),
	(math.ping_pong(4, 2, 4), 2),
	(math.ping_pong(5, 2, 4), 3),

	(math.ping_pong(-5, -12, -10), -11),
	(math.ping_pong(-4, -12, -10), -12),
	(math.ping_pong(-3, -12, -10), -11),
	(math.ping_pong(-2, -12, -10), -10),
	(math.ping_pong(-1, -12, -10), -11),
	(math.ping_pong(0, -12, -10), -12),
	(math.ping_pong(1, -12, -10), -11),
	(math.ping_pong(2, -12, -10), -10),
	(math.ping_pong(3, -12, -10), -11),
	(math.ping_pong(4, -12, -10), -12),
	(math.ping_pong(5, -12, -10), -11),
]

for i, (actual, expected) in enumerate(test)
	assert type(actual) == type(expected) and actual == expected, i + ": " + actual + " == " + expected