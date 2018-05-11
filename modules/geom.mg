#!/usr/bin/env modelgen

import math
import vec
import mat


matrix_stack = [mat.mat4(1)]

func get_matrix()
	return matrix_stack[-1]

func set_matrix(matrix)
	matrix_stack[-1] = matrix

func push()
	matrix_stack.add(get_matrix())

func pop()
	assert len(matrix_stack) > 1
	delete matrix_stack[-1]

func scale(x, y, z)
	set_matrix(mat.mul(get_matrix(), mat.scaling((x, y, z))))

func translate(x, y, z)
	set_matrix(mat.mul(get_matrix(), mat.translation((x, y, z))))

func rotate(radians, x, y, z)
	set_matrix(mat.mul(get_matrix(), mat.rotation(radians, vec.normalize((x, y, z)))))


proc vertex(position, normal)
	m = get_matrix()
	position = mat.mul(m, position)
	normal = mat.mul(mat.get_rotation(m), normal)
	emit position[0], position[1], position[2], normal[0], normal[1], normal[2]


func flip_triangle(p1, p2, p3)
	return p1, p3, p2

func get_triangle_normal(p1, p2, p3)
	return vec.cross(vec.sub(p2, p1), vec.sub(p3, p1))

func triangles_to_points(triangles)
	points = []
	for triangle in triangles
		assert len(triangle) == 3
		points.extend(triangle)
	return points

func quads_to_triangles(quads)
	_triangles = []
	for p1, p2, p3, p4 in quads
		_triangles.extend(make_quad(p1, p2, p3, p4))
	return _triangles


func translate_polygon(polygon, xyz)
	for i in range(len(polygon))
		polygon[i] = vec.add(polygon[i], xyz)
	return polygon

func translate_triangles(triangles, xyz)
	for triangle in triangles
		assert len(triangle) == 3
		for i in range(3)
			triangle[i] = vec.add(triangle[i], xyz)
	return triangles


func get_bounds(polygon)
	assert len(polygon) > 0
	min, max = polygon[0], polygon[0]
	for p in polygon
		min, max = vec.min(min, p), vec.max(max, p)
	return vec.sub(max, min)

func get_bounds_triangles(triangles)
	assert len(triangles) > 0
	min, max = triangles[0][0], triangles[0][0]
	for triangle in triangles
		assert len(triangle) == 3
		for p in triangle
			min, max = vec.min(min, p), vec.max(max, p)
	return vec.sub(max, min)

func get_center_point(polygon)
	assert len(polygon) > 0
	center = (0,) * len(polygon[0])
	for p in polygon
		center = vec.add(center, p)
	center = vec.div(center, len(polygon))
	return center

func is_convex(polygon)
	assert len(polygon) > 2
	for i in range(len(polygon))
		p1 = polygon[i]
		p2 = polygon[(i + 1) % len(polygon)]
		p3 = polygon[(i + 2) % len(polygon)]
		d1, d2 = vec.sub(p3, p2), vec.sub(p1, p2)
		assert len(d1) == 2 and len(d2) == 2
		z = d1[0] * d2[1] - d1[1] * d2[0]
		if i == 0
			sign = z > 0
		else if sign != (z > 0)
			return false
	return true


func triangle_inside(a, b, c, p)
	ax, ay = c[0] - b[0], c[1] - b[1]
	bx, by = a[0] - c[0], a[1] - c[1]
	cx, cy = b[0] - a[0], b[1] - a[1]
	apx, apy = p[0] - a[0], p[1] - a[1]

	bpx, bpy = p[0] - b[0], p[1] - b[1]
	cpx, cpy = p[0] - c[0], p[1] - c[1]

	acbp = ax * bpy - ay * bpx
	ccap = cx * apy - cy * apx
	bccp = bx * cpy - by * cpx

	return acbp >= 0.0 and bccp >= 0.0 and ccap >= 0.0

func _area(polygon)
	n = len(polygon)
	a, p = 0, n - 1
	for q in range(n)
		a += polygon[p][0] * polygon[q][1] - polygon[q][0] * polygon[p][1]
		p = q
	return a * 0.5

func ear_clipping(polygon)
	n = len(polygon)
	assert n > 2

	func snip(u, v, w, n, vertices)
		a, b, c = polygon[vertices[u]], polygon[vertices[v]], polygon[vertices[w]]
		if math.epsilon > (((b[0] - a[0]) * (c[1] - a[1])) - ((b[1] - a[1]) * (c[0] - a[0])))
			return false
		for p in range(n)
			if p == u or p == v or p == w
				continue
			p = polygon[vertices[p]]
			if triangle_inside(a, b, c, p)
				return false
		return true

	vertices = null
	if _area(polygon) > 0
		vertices = range(n)
	else
		vertices = range(n - 1, -1)

	indices = []

	nv = n
	count = nv * 2
	m, v = 0, nv - 1

	while nv > 2
		if (count -= 1) <= 0
			break

		u = v
		u = nv <= u ? 0 : u
		v = u + 1
		v = nv <= v ? 0 : v
		w = v + 1
		w = nv <= w ? 0 : w

		if snip(u, v, w, nv, vertices)
			indices.add(vertices[u], vertices[v], vertices[w])
			m += 1
			s = v
			t = v + 1
			while t < nv
				vertices[s] = vertices[t]
				s += 1
				t += 1
			nv -= 1
			count = nv * 2

	return map(i -> (
		polygon[indices[i]],
		polygon[indices[i + 1]],
		polygon[indices[i + 2]]),
		range(0, len(indices), 3))

func triangulate(polygon, center = (0, 0, 0), clockwise = false)
	assert len(polygon) > 2
	_triangles = []
	if len(polygon) == 3
		p1, p2, p3 = polygon
		_triangles.add((p1, p2, p3))
	else if len(polygon) == 4
		p1, p2, p3, p4 = polygon
		_triangles.add((p1, p2, p4), (p2, p3, p4))
	else if is_convex(polygon)
		p1 = polygon[0]
		for i in range(len(polygon))
			p2, p3 = polygon[(i + 1) % len(polygon)], polygon[(i + 2) % len(polygon)]
			_triangles.add((p1, p2, p3))
	else
		_triangles = ear_clipping(polygon)
	for i in range(len(_triangles))
		assert len(_triangles[i]) == 3
		for j in range(3)
			_triangles[i][j] = vec.add((_triangles[i][j][0], 0, _triangles[i][j][1]), center)
		if clockwise
			_triangles[i] = flip_triangle(_triangles[i][0], _triangles[i][1], _triangles[i][2])
	assert len(_triangles) > 0
	return _triangles


# 1 +--+ 4
#   | /|
#   |/ |
# 2 +--+ 3
func make_quad(p1, p2, p3, p4, center = (0, 0, 0))
	p1, p2 = vec.add(p1, center), vec.add(p2, center)
	p3, p4 = vec.add(p3, center), vec.add(p4, center)
	return (p1, p2, p4), (p2, p3, p4)


func make_oblique_pyramid(polygon, translation = (0, 1, 0), center = (0, 0, 0))
	assert len(polygon) > 2
	_triangles = []
	half_translation = vec.div(translation, 2)
	top = get_center_point(polygon)
	top = vec.add((top[0], 0, top[1]), half_translation)
	top = vec.add(top, center)
	bottom = triangulate(polygon, center, true)
	for i in range(len(bottom))
		assert len(bottom[i]) == 3
		for j in range(3)
			bottom[i][j] = vec.sub(bottom[i][j], half_translation)
		_triangles.add(bottom[i])
	for i in range(len(polygon))
		p2, p3 = polygon[(i + 1) % len(polygon)], polygon[(i + 2) % len(polygon)]
		p2 = p2[0], 0, p2[1]
		p3 = p3[0], 0, p3[1]
		p2, p3 = vec.sub(p2, half_translation), vec.sub(p3, half_translation)
		p2, p3 = vec.add(p2, center), vec.add(p3, center)
		_triangles.add((top, p2, p3))
	return _triangles

func make_pyramid(polygon, height, center = (0, 0, 0))
	return make_oblique_pyramid(polygon, (0, height, 0), center)


func make_oblique_frustum(top, bottom, translation = (0, 1, 0), center = (0, 0, 0))
	assert len(top) > 2 and len(bottom) > 2 and len(top) == len(bottom)
	_triangles = []
	half_translation = vec.div(translation, 2)
	_top, _bottom = triangulate(top, center), triangulate(bottom, center, true)
	for i in range(len(_top))
		assert len(_top[i]) == 3
		for j in range(3)
			_top[i][j] = vec.add(_top[i][j], half_translation)
		_triangles.add(_top[i])
	for i in range(len(_bottom))
		assert len(_bottom[i]) == 3
		for j in range(3)
			_bottom[i][j] = vec.sub(_bottom[i][j], half_translation)
		_triangles.add(_bottom[i])
	for i in range(len(top))
		p1, p4 = top[i], top[(i + 1) % len(top)]
		p2, p3 = bottom[i], bottom[(i + 1) % len(bottom)]
		p1 = p1[0], 0, p1[1]
		p2 = p2[0], 0, p2[1]
		p3 = p3[0], 0, p3[1]
		p4 = p4[0], 0, p4[1]
		p1, p4 = vec.add(p1, half_translation), vec.add(p4, half_translation)
		p2, p3 = vec.sub(p2, half_translation), vec.sub(p3, half_translation)
		_triangles.extend(make_quad(p1, p2, p3, p4, center))
	return _triangles

func make_frustum(top, bottom, height = 1, center = (0, 0, 0))
	return make_oblique_frustum(top, bottom, (0, height, 0), center)


func make_oblique_prism(polygon, translation = (0, 1, 0), center = (0, 0, 0))
	assert len(polygon) > 2
	_triangles = []
	half_translation = vec.div(translation, 2)
	bottom, top = triangulate(polygon, center, true), triangulate(polygon, center)
	for i in range(len(bottom))
		assert len(bottom[i]) == 3
		for j in range(3)
			bottom[i][j] = vec.sub(bottom[i][j], half_translation)
		_triangles.add(bottom[i])
	for i in range(len(top))
		assert len(top[i]) == 3
		for j in range(3)
			top[i][j] = vec.add(top[i][j], half_translation)
		_triangles.add(top[i])
	for i in range(len(polygon))
		p2, p3 = polygon[i], polygon[(i + 1) % len(polygon)]
		p2 = p2[0], 0, p2[1]
		p3 = p3[0], 0, p3[1]
		p2, p3 = vec.sub(p2, half_translation), vec.sub(p3, half_translation)
		p1, p4 = vec.add(p2, translation), vec.add(p3, translation)
		_triangles.extend(make_quad(p1, p2, p3, p4, center))
	return _triangles

func make_prism(polygon, height = 1, center = (0, 0, 0))
	return make_oblique_prism(polygon, (0, height, 0), center)

func make_cylinder(diameter, height, center = (0, 0, 0), segments = 8)
	return make_prism(make_circle(diameter, (0, 0), segments), height, center)


func make_triangle(size = (1, 1), center = (0, 0))
	(hw, hh), (x, y) = vec.div(size, 2), center
	return [
		(x - hw, y - hh),
		(x - hw, y + hh),
		(x + hw, y - hh)]


func make_rectangle(size = (1, 1), center = (0, 0))
	(hw, hh), (x, y) = vec.div(size, 2), center
	return [
		(x - hw, y - hh),
		(x - hw, y + hh),
		(x + hw, y + hh),
		(x + hw, y - hh)]


func _make_rectangle_x(size = (1, 1), center = (0, 0))
	(hw, hh), (x, y) = vec.div(size, 2), center
	return [
		(0, x - hw, y - hh),
		(0, x - hw, y + hh),
		(0, x + hw, y + hh),
		(0, x + hw, y - hh)]


func _make_rectangle_y(size = (1, 1), center = (0, 0))
	(hw, hh), (x, y) = vec.div(size, 2), center
	return [
		(x - hw, 0, y - hh),
		(x - hw, 0, y + hh),
		(x + hw, 0, y + hh),
		(x + hw, 0, y - hh)]


func _make_rectangle_z(size = (1, 1), center = (0, 0))
	(hw, hh), (x, y) = vec.div(size, 2), center
	return [
		(x - hw, y - hh, 0),
		(x - hw, y + hh, 0),
		(x + hw, y + hh, 0),
		(x + hw, y - hh, 0)]


func make_circle(diameter = 1, center = (0, 0), segments = 8)
	r, (x, y) = diameter / 2, center
	assert segments > 2
	angle = math.rad(360 / segments)
	polygon = []
	for i in range(segments)
		polygon.add((x - math.cos(angle * i) * r, y + math.sin(angle * i) * r))
	return polygon


func make_ellipse(size = (1, 1), center = (0, 0), segments = 8)
	(w, h), (x, y) = vec.div(size, 2), center
	assert segments > 2
	angle = math.rad(360 / segments)
	polygon = []
	for i in range(segments)
		polygon.add((x - math.cos(angle * i) * w, y + math.sin(angle * i) * h))
	return polygon


func make_cube(size = (1, 1, 1), center = (0, 0, 0))
	hw, hh, hl = vec.div(size, 2)
	return translate_triangles(quads_to_triangles([
		((-hw, hh, -hl), (-hw, hh, hl), (hw, hh, hl), (hw, hh, -hl)), # Top
		((-hw, -hh, hl), (-hw, -hh, -hl), (hw, -hh, -hl), (hw, -hh, hl)), # Bottom
		((-hw, hh, hl), (-hw, -hh, hl), (hw, -hh, hl), (hw, hh, hl)), # Front
		((hw, hh, -hl), (hw, -hh, -hl), (-hw, -hh, -hl), (-hw, hh, -hl)), # Back
		((-hw, hh, -hl), (-hw, -hh, -hl), (-hw, -hh, hl), (-hw, hh, hl)), # Left
		((hw, hh, hl), (hw, -hh, hl), (hw, -hh, -hl), (hw, hh, -hl))]), # Right
		center)

func make_square_pyramid(size = (1, 1, 1), center = (0, 0, 0))
	hw, hh, hl = vec.div(size, 2)
	_triangles = []
	_triangles.extend(quads_to_triangles([
		((-hw, -hh, hl), (-hw, -hh, -hl), (hw, -hh, -hl), (hw, -hh, hl))])) # Bottom
	_triangles.extend([
		((-hw, -hh, hl), (hw, -hh, hl), (0, hh, 0)), # Front
		((hw, -hh, -hl), (-hw, -hh, -hl), (0, hh, 0)), # Back
		((hw, -hh, hl), (hw, -hh, -hl), (0, hh, 0)), # Left
		((-hw, -hh, -hl), (-hw, -hh, hl), (0, hh, 0))]) # Right
	return translate_triangles(_triangles, center)


func make_sphere(diameter = 1, center = (0, 0, 0), horizontal_segments = 24, vertical_segments = 24)
	return make_ellipsoid((diameter, diameter, diameter), center, horizontal_segments, vertical_segments)

func make_ellipsoid(size = (1, 1, 1), center = (0, 0, 0), horizontal_segments = 24, vertical_segments = 24)
	hw, hh, hl = vec.div(size, 2)
	cx, cy, cz = center
	s, r = 1 / horizontal_segments, 1 / vertical_segments
	_triangles = []
	for ir in range(vertical_segments)
		rs0 = math.sin(math.pi * (ir + 0) * r)
		rs1 = math.sin(math.pi * (ir + 1) * r)
		rns0 = math.sin(-math.pi * 0.5 + math.pi * (ir + 0) * r)
		rns1 = math.sin(-math.pi * 0.5 + math.pi * (ir + 1) * r)
		for is in range(horizontal_segments)
			sc0 = math.cos(math.tau * (is + 0) * s)
			sc1 = math.cos(math.tau * (is + 1) * s)
			ss0 = math.sin(math.tau * (is + 0) * s)
			ss1 = math.sin(math.tau * (is + 1) * s)
			x1, y1, z1 = sc0 * rs0, rns0, ss0 * rs0
			x2, y2, z2 = sc1 * rs0, rns0, ss1 * rs0
			x3, y3, z3 = sc1 * rs1, rns1, ss1 * rs1
			x4, y4, z4 = sc0 * rs1, rns1, ss0 * rs1
			_triangles.add((
				(cx + x1 * hw, cy + y1 * hh, cz + z1 * hl),
				(cx + x3 * hw, cy + y3 * hh, cz + z3 * hl),
				(cx + x2 * hw, cy + y2 * hh, cz + z2 * hl)))
			_triangles.add((
				(cx + x3 * hw, cy + y3 * hh, cz + z3 * hl),
				(cx + x1 * hw, cy + y1 * hh, cz + z1 * hl),
				(cx + x4 * hw, cy + y4 * hh, cz + z4 * hl)))
	return _triangles

func make_semisphere(diameter = 1, center = (0, 0, 0), horizontal_segments = 24 / 2, vertical_segments = 24)
	return make_semiellipsoid((diameter, diameter / 2, diameter), center, horizontal_segments, vertical_segments)

make_hemisphere = make_semisphere

func make_semiellipsoid(size = (1, 0.5, 1), center = (0, 0, 0), horizontal_segments = 24 / 2, vertical_segments = 24)
	hw, hh, hl = size
	hw, hl = hw / 2, hl / 2
	cx, cy, cz = center
	cy -= hh / 2
	s, r = 1 / horizontal_segments, 1 / vertical_segments
	hr = r / 2
	_triangles = []
	for ir in range(vertical_segments)
		rs0 = math.sin(math.pi / 2 * (ir + 0) * r)
		rs1 = math.sin(math.pi / 2 * (ir + 1) * r)
		rns0 = math.sin(math.pi * 0.5 + math.pi / 2 * (ir + 0) * r)
		rns1 = math.sin(math.pi * 0.5 + math.pi / 2 * (ir + 1) * r)
		for is in range(horizontal_segments)
			sc0 = math.cos(math.tau * (is + 0) * s)
			sc1 = math.cos(math.tau * (is + 1) * s)
			ss0 = math.sin(math.tau * (is + 0) * s)
			ss1 = math.sin(math.tau * (is + 1) * s)
			x1, y1, z1 = sc0 * rs0, rns0, ss0 * rs0
			x2, y2, z2 = sc1 * rs0, rns0, ss1 * rs0
			x3, y3, z3 = sc1 * rs1, rns1, ss1 * rs1
			x4, y4, z4 = sc0 * rs1, rns1, ss0 * rs1
			_triangles.add((
				(cx + x1 * hw, cy + y1 * hh, cz + z1 * hl),
				(cx + x3 * hw, cy + y3 * hh, cz + z3 * hl),
				(cx + x2 * hw, cy + y2 * hh, cz + z2 * hl)))
			_triangles.add((
				(cx + x3 * hw, cy + y3 * hh, cz + z3 * hl),
				(cx + x1 * hw, cy + y1 * hh, cz + z1 * hl),
				(cx + x4 * hw, cy + y4 * hh, cz + z4 * hl)))
	return _triangles


proc triangle(p1, p2, p3, center = (0, 0, 0), clockwise = false)
	if clockwise
		p1, p2, p3 = flip_triangle(p1, p2, p3)
	normal = get_triangle_normal(p1, p2, p3)
	for position in p1, p2, p3
		vertex(vec.add(position, center), normal)

proc triangles(triangles, center = (0, 0, 0), clockwise = false)
	for p1, p2, p3 in triangles
		triangle(p1, p2, p3, center, clockwise)


proc quad(p1, p2, p3, p4, clockwise = false)
	triangles(make_quad(p1, p2, p3, p4), clockwise)


proc cube(size = (1, 1, 1), center = (0, 0, 0), inverted = false)
	triangles(make_cube(size, center), inverted)

proc square_pyramid(size = (1, 1, 1), center = (0, 0, 0), inverted = false)
	triangles(make_square_pyramid(size, center), inverted)


proc oblique_pyramid(polygon, translation = (0, 1, 0), center = (0, 0, 0))
	triangles(make_oblique_pyramid(polygon, translation, center))

proc pyramid(polygon, height = 1, center = (0, 0, 0))
	triangles(make_pyramid(polygon, height, center))


proc oblique_frustum(top, bottom, translation = (0, 1, 0), center = (0, 0, 0))
	triangles(make_oblique_frustum(top, bottom, translation, center))

proc frustum(top, bottom, height = 1, center = (0, 0, 0))
	triangles(make_frustum(top, bottom, height, center))


proc oblique_prism(polygon, translation = (0, 1, 0), center = (0, 0, 0))
	triangles(make_oblique_prism(polygon, translation, center))

proc prism(polygon, height = 1, center = (0, 0, 0))
	triangles(make_prism(polygon, height, center))

proc cylinder(diameter, height, center = (0, 0, 0), segments = 8)
	triangles(make_cylinder(diameter, height, center, segments))


proc sphere(diameter = 1, center = (0, 0, 0), horizontal_segments = 24, vertical_segments = 24)
	triangles(make_sphere(diameter, center, horizontal_segments, vertical_segments))

proc ellipsoid(size = (1, 1, 1), center = (0, 0, 0), horizontal_segments = 24, vertical_segments = 24)
	triangles(make_ellipsoid(size, center, horizontal_segments, vertical_segments))

proc semisphere(diameter = 1, center = (0, 0, 0), horizontal_segments = 24 / 2, vertical_segments = 24)
	triangles(make_semisphere(diameter, center, horizontal_segments, vertical_segments))

hemisphere = semisphere

proc semiellipsoid(size = (1, 0.5, 1), center = (0, 0, 0), horizontal_segments = 24 / 2, vertical_segments = 24)
	triangles(make_semiellipsoid(size, center, horizontal_segments, vertical_segments))
