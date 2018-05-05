
a = {"a": 1, "b": 2}

assert a == {"a": 1, "b": 2}
assert a.size == 2

assert a != {"a": 1, "b": 2, "c": 3}
assert a != {"a": 2, "b": 1}
assert a != {"a": 2, "b": 1, "c": 3}
assert a != {"a": 1}
assert a != {"b": 2}

a["b"] += a["b"] + 8
a["c"] = 3

assert a == {"a": 1, "b": 12, "c": 3}
assert a.size == 3


k = a.keys()
v = a.values()
p = a.pairs()

k.sort((a, b) -> a > b)
v.sort((a, b) -> a > b)
p.sort((a, b) -> a[0] > b[0])

assert k == ["a", "b", "c"]
assert v == [1, 3, 12]
assert p == [("a", 1), ("b", 12), ("c", 3)]


a["b"], a["c"] = a["c"], a["b"]

a["b"] -= 1
a["c"] += 1

assert a == {"a": 1, "b": 2, "c": 13}

a["d"] = 0

assert a == {"a": 1, "b": 2, "c": 13, "d": 0}

assert a["a"] == 1
assert a["d"] == 0
assert a["e"] == null

assert (a["a"] ?? 10) == 1
assert (a["d"] ?? 10) == 0
assert (a["e"] ?? 10) == 10

assert (a["a"] ?: 10) == 1
assert (a["d"] ?: 10) == 10
assert (a["e"] ?: 10) == 10

delete a["d"]
assert a == {"a": 1, "b": 2, "c": 13}


a = {"a": [1], "b": 2}
a["c"] = 3

b = a.copy()

a["d"] = 4
b["e"] = 5

a["a"].add(11)
b["a"].add(111)

assert a == {"a": [1, 11, 111], "b": 2, "c": 3, "d": 4}
assert b == {"a": [1, 11, 111], "b": 2, "c": 3, "e": 5}


a = {"a": 1, "b": 2}

assert a.has("a") == true
assert a.has("b") == true
assert a.has("c") == false

assert a.contains(1) == true
assert a.contains(2) == true
assert a.contains(3) == false


a = {"a": 1, "b": 2}
assert a == {"a": 1, "b": 2}
assert a.pop("a") == 1
assert a.pop("a") == null
assert a.pop("b") == 2
assert a.pop("c") == null
assert a == {}
