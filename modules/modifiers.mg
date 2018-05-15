#!/usr/bin/env modelgen

from math import cos, sin
import vec
import mat


_modifiers = []

func push_modifier(modifier)
	_modifiers.add(modifier)

func pop_modifier()
	assert len(_modifiers) > 0
	delete _modifiers[-1]


func _twist(angle, axis = (0, 1, 0))
	axis = vec.normalize(axis)
	func apply(position, normal)
		a = vec.dot(position, axis) * angle

		t = mat.translation(position)
		r = mat.rotation(a, axis)
		m = mat.mul(r, t)

		position = mat.get_translation(m)
		normal = mat.mul(r, normal)

		return position, normal
	return apply

func twist(angle, axis = (0, 1, 0))
	push_modifier(_twist(angle, axis))
