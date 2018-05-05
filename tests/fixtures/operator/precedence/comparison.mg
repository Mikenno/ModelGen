
f, t = 0, 1

func xor(a, b)
	return a == not b

print(f, t)
print(not f, not t)

print(f and f, f and t, t and f, t and t)
print(f or f, f or t, t or f, t or t)
print(xor(f, f), xor(f, t), xor(t, f), xor(t, t))

print(1 + 2 > 3 + 4 or 5 + 6 < 7 + 8)
print(1 == 2 or 3 != 4)

print([] and [], [] and [1, 2], [1, 2] and [], [1, 2] and [1, 2])
print([] or [], [] or [1, 2], [1, 2] or [], [1, 2] or [1, 2])
