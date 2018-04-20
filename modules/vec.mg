#!/usr/bin/env modelgen

from math import max as _max, min as _min, sqrt

# Check if y is a multiple of x
func is_multiple(a, b)
	return b % a == 0

func sum(iterable)
	result = 0
	for x in iterable
		result += x
	return result

func op_add(a, b) return a + b
func op_sub(a, b) return a - b
func op_mul(a, b) return a * b
func op_div(a, b) return a / b
func op_mod(a, b) return a % b

func _cast(v, n = 1)
	if type(v) != "Tuple"
		return (v,) * n
	return v

func op_vec(a, b, op = op_add)
	a, b = _cast(a), _cast(b)
	assert len(a) == len(b) or is_multiple(len(a), len(b)) or is_multiple(len(b), len(a)), "vec" + len(a) + " and vec" + len(b) + " either is not a multiple of the other"
	v = (0,) * _max(len(a), len(b))
	for i in range(_max(len(a), len(b)))
		v[i] = op(a[i % len(a)], b[i % len(b)])
	return v

func add(a, b) return op_vec(a, b, op_add)
func sub(a, b) return op_vec(a, b, op_sub)
func mul(a, b) return op_vec(a, b, op_mul)
func div(a, b) return op_vec(a, b, op_div)
func mod(a, b) return op_vec(a, b, op_mod)

func max(a, b) return op_vec(a, b, _max)
func min(a, b) return op_vec(a, b, _min)

func dot(a, b) return sum(mul(a, b))
func length_squared(a) return dot(a, a)
func length(a) return sqrt(length_squared(a))
func distance_squared(a, b) return length_squared(sub(a, b))
func distance(a, b) return length(sub(a, b))
func normalize(a, to = 1) return mul(a, (to / length(a),) * len(a))

func cross(a, b)
	a, b = _cast(a), _cast(b)
	assert len(a) == 3 and len(b) == 3
	return (
		(a[1] * b[2]) - (a[2] * b[1]),
		(a[2] * b[0]) - (a[0] * b[2]),
		(a[0] * b[1]) - (a[1] * b[0]))

func vec2(x, y) return x, y
func vec3(x, y, z) return x, y, z
func vec4(x, y, z, w = 1) return x, y, z, w
