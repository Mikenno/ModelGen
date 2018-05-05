
a, b = [], []

assert [].add != [].add
assert a.add != [].add
assert b.add != [].add

assert a.add == a.add
assert b.add == b.add
assert a.add != b.add

a.add(1, 2)
b.add(3, 4)

a_add = a.add
b_add = b.add

assert a == a_add.bound
assert b == b_add.bound

a_add(5, 6)
b_add(7, 8)

map(a_add, map(x -> x * x, range(10, 16)))
map(b_add, map(x -> x * x * x, range(10, 16)))

assert a.size == 10
assert b.size == 10

assert a == [1, 2, 5, 6, 100, 121, 144, 169, 196, 225]
assert b == [3, 4, 7, 8, 1000, 1331, 1728, 2197, 2744, 3375]
