
a = 1
assert a == 1
a = a + 1
assert a == 2
a += 1
assert a == 3
a *= 3
assert a == 9
a += a * 2
assert a == 27
a /= 5
assert a == 5.4
a //= 2
assert a == 2

a = [1, 2, 3]
assert a == [1, 2, 3]
assert (a[1] += a[1] * 10) == 22
assert a == [1, 22, 3]
