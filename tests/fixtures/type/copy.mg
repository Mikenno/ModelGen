
a = [1, [2, 3], 4]

b = copy(a)
c = deep_copy(a)

assert a == b
assert a == c

a.add(5)
a[1][1] = 33
b[1].add(3.5)
c[1].add(3.75)
c.add(6)

assert a == [1, [2, 33, 3.5], 4, 5]
assert b == [1, [2, 33, 3.5], 4]
assert c == [1, [2, 3, 3.75], 4, 6]
