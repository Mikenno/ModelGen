
func vec3(x, y, z)
	v = {"x": x, "y": y, "z": z}

	v.length_squared = func()
		return v.x * v.x + v.y * v.y + v.z * v.z

	v.zyx = func()
		return vec3(v.z, v.y, v.x)

	return v


a = vec3(1, 2, 3)
b = vec3(4, 5, 6)
c = a.zyx()

print(a.x, a.y, a.z)
print(a.length_squared())
print()

print(b.x, b.y, b.z)
print(b.length_squared())
print()

print(c.x, c.y, c.z)
print(c.length_squared())
print()

a.x = b.x
b.x = -b.x

print(a.x, a.y, a.z)
print(a.length_squared())
print()

print(b.x, b.y, b.z)
print(b.length_squared())
print()

print(c.x, c.y, c.z)
print(c.length_squared())
print()
