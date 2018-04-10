
func counter(n = 0, delta = 1)
	return func(d = delta)
		n += d
		return n

c1 = counter()
c2 = counter(10)
c3 = counter(10, 5)

print(c1())
print(c1())
print(c1(10))
print()

print(c2())
print(c2())
print(c2(100))
print()

print(c3())
print(c3())
print(c3(1))
print()

print(c1())
print(c1())
print()

print(c2())
print(c2())
print()

print(c3())
print(c3())
print()
