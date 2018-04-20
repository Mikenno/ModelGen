#!/usr/bin/env modelgen

from math import cos, sin
from vec import _cast, mul as vec_mul, dot, length, normalize

func vec_type(x)
	t = type(x)
	if t == "Tuple"
		if type(x[0]) == "Tuple"
			return "Matrix"
		return "Vector"
	return t

func mat4(diagonal = 1)
	return (
		(diagonal, 0, 0, 0),
		(0, diagonal, 0, 0),
		(0, 0, diagonal, 0),
		(0, 0, 0, diagonal))

func column(m, index)
	c = (0,) * len(m[0])
	for i in range(len(m[0]))
		c[i] = m[index][i]
	return c

func row(m, index)
	r = (0,) * len(m)
	for i in range(len(m))
		r[i] = m[i][index]
	return r

func scaling(scaling)
	scaling = _cast(scaling, 3)
	assert len(scaling) == 3
	m = mat4(1)
	for i in range(3)
		m[i][i] = scaling[i]
	return m

func translation(xyz)
	xyz = _cast(xyz, 3)
	assert len(xyz) == 3
	m = mat4(1)
	for i in range(3)
		m[3][i] = xyz[i]
	return m

func rotation(radians, axis)
	axis = _cast(axis, 3)
	s, c = sin(radians), cos(radians)
	oc = 1 - c
	x, y, z = normalize(axis)
	return (
		((x * x * oc + c), (x * y * oc - z * s), (x * z * oc + y * s), 0),
		((y * x * oc + z * s), (y * y * oc + c), (y * z * oc - x * s), 0),
		((x * z * oc - y * s), (y * z * oc + x * s), (z * z * oc + c), 0),
		(0, 0, 0, 1))

func mul(a, b)
	ta, tb = vec_type(a), vec_type(b)
	if ta == "Matrix" and tb == "Matrix"
		result = mat4()
		for i in range(len(a[0]))
			for j in range(len(a))
				result[i][j] = dot(row(a, j), column(b, i))
		return result
	else if ta == "Vector" and tb == "Matrix"
		print()
		if len(a) == 3
			a = a[0], a[1], a[2], 1
			return (dot(a, b[0]), dot(a, b[1]), dot(a, b[2]))
		return (dot(a, b[0]), dot(a, b[1]), dot(a, b[2]), dot(a, b[3]))
	else if ta == "Matrix" and tb == "Vector"
		if len(b) == 3
			b = b[0], b[1], b[2], 1
			return (
				(b[0] * a[0][0]) + (b[1] * a[1][0]) + (b[2] * a[2][0]) + (b[3] * a[3][0]),
				(b[0] * a[0][1]) + (b[1] * a[1][1]) + (b[2] * a[2][1]) + (b[3] * a[3][1]),
				(b[0] * a[0][2]) + (b[1] * a[1][2]) + (b[2] * a[2][2]) + (b[3] * a[3][2]))
		return (
			(b[0] * a[0][0]) + (b[1] * a[1][0]) + (b[2] * a[2][0]) + (b[3] * a[3][0]),
			(b[0] * a[0][1]) + (b[1] * a[1][1]) + (b[2] * a[2][1]) + (b[3] * a[3][1]),
			(b[0] * a[0][2]) + (b[1] * a[1][2]) + (b[2] * a[2][2]) + (b[3] * a[3][2]),
			(b[0] * a[0][3]) + (b[1] * a[1][3]) + (b[2] * a[2][3]) + (b[3] * a[3][3]))
	else if ta == "Vector" and tb == "Vector"
		return vec_mul(a, b)
	print(a)
	print(b)
	assert false

func scale(m, xyz)
	return mul(m, scaling(xyz))

func translate(m, xyz)
	return mul(m, translation(xyz))

func rotate(m, radians, axis)
	return mul(m, rotation(radians, axis))

func get_translation(m)
	xyzw = column(m, 3)
	return xyzw[0], xyzw[1], xyzw[2]

func get_scaling(m)
	sx, sy, sz = column(m, 0), column(m, 1), column(m, 2)
	sx = length((sx[0], sx[1], sx[2]))
	sy = length((sy[0], sy[1], sy[2]))
	sz = length((sz[0], sz[1], sz[2]))
	return sx, sy, sz

func get_rotation(m)
	sx, sy, sz = get_scaling(m)
	return (
		(m[0][0] / sx, m[0][1] / sy, m[0][2] / sz, 0),
		(m[1][0] / sx, m[1][1] / sy, m[1][2] / sz, 0),
		(m[2][0] / sx, m[2][1] / sy, m[2][2] / sz, 0),
		(0, 0, 0, 1))
