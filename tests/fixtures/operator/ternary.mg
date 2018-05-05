
a, b = 1, 1
assert (b < a ? b : a, a < b ? b : a) == (1, 1)
a, b = 1, 2
assert (b < a ? b : a, a < b ? b : a) == (1, 2)
a, b = 2, 1
assert (b < a ? b : a, a < b ? b : a) == (1, 2)

a = 0
assert a == 0
a == 0 ? (a += 1) : (a += 2)
assert a == 1
a == 0 ? (a += 1) : (a += 2)
assert a == 3

a = [10, 20, 30]
assert a[a[0] > 10 ? 1 : 0] == 10
assert (a[a[0] > 10 ? 1 : 0] = 15) == 15
assert a == [15, 20, 30]
assert a[a[0] > 10 ? 1 : 0] == 20
assert (a[a[0] > 10 ? 1 : 0] = 25) == 25
assert a == [15, 25, 30]

a = 1
assert a == 1
assert (a += 1) == 2 # Prefix increment
assert (a, a += 1)[0] == 2 # Postfix increment
assert a == 3

a = [1, 2, 3]
assert a == [1, 2, 3]
assert (a, a[(a[0] == 1 ? 0 : 1)] += 10)[0] == [11, 2, 3]
assert (a, a[(a[0] == 1 ? 0 : 1)] += 10)[0] == [11, 12, 3]
assert a == [11, 12, 3]
