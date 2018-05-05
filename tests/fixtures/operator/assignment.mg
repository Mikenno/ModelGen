
a = 1
assert a == 1

a = 2
assert a == 2

a, b = 1 + 2, 3 + 4
assert a == 3
assert b == 7

(a, b), c, (d, e) = (1 + 2, 3 + 4), 5 + 6, (7 + 8, 9 + 10)
assert a == 3
assert b == 7
assert c == 11
assert d == 15
assert e == 19

a = [1, 2, 3]
assert a == [1, 2, 3]
assert (a[1] = 22) == 22
assert a == [1, 22, 3]

a = [1, 2, 3]
assert a == [1, 2, 3]
assert (a[a[0] == 1 ? 0 : 1] += 10) == 11
assert a == [11, 2, 3]
assert (a[a[0] == 1 ? 0 : 1] += 10) == 12
assert a == [11, 12, 3]

a = [0, 0, 0]
assert a == [0, 0, 0]
a[a[0]] += 1
assert a == [1, 0, 0]
a[a[0]] += 1
assert a == [1, 1, 0]
a[a[0]] += 1
assert a == [1, 2, 0]

a = {"a": 1}
assert a == {"a": 1}
assert (a["b"] = 2) == 2
assert a == {"a": 1, "b": 2}

a = {"a": 1, "b": 2}
assert a == {"a": 1, "b": 2}
assert (a["a"] += 10) == 11
assert a == {"a": 11, "b": 2}
assert (a.c = 3) == 3
assert a == {"a": 11, "b": 2, "c": 3}
assert a[a["b"] > 10 ? "c" : "b"] == 2
assert ((a[a["b"] > 10 ? "c" : "b"]) += 10) == 12
assert (a[a["b"] > 10 ? "c" : "b"]) == 3
assert a == {"a": 11, "b": 12, "c": 3}
assert ((a[a["b"] > 10 ? "c" : "b"]) += 10) == 13
assert (a[a["b"] > 10 ? "c" : "b"]) == 13
assert a == {"a": 11, "b": 12, "c": 13}
