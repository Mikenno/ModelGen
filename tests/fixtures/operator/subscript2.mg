
a = [null]
assert a == [null]
a[0] = [null]
assert a == [[null]]
a[0][0] = [null]
assert a == [[[null]]]
a[0][0][0] = []
assert a == [[[[]]]]
