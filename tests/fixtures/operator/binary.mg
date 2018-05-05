
assert 1 + 1 == 2
assert 1 + 1.5 == 2.5
assert 1.5 + 1 == 2.5
assert 1.5 + 1.5 == 3.0

assert "1" + "2" == "12"
assert "1" + 2 == "12"
assert 1 + "2" == "12"
assert 1 + 2 + "3" == "33"
assert 1 + (2 + "3") == "123"
assert "1" + 2 + 3 == "123"
assert "1" + 2 + "3" == "123"

assert "Test" + ["A", "B"] + (1, 2) == "Test[\"A\", \"B\"](1, 2)"

assert (1, 2) + (3, 4) == (1, 2, 3, 4)
assert [1, 2] + [3, 4] == [1, 2, 3, 4]

assert {"a": 1, "b": 2} + {"a": 11, "c": 33} == {"a": 11, "b": 2, "c": 33}

assert [1, 2] * 3 == [1, 2, 1, 2, 1, 2]
assert 3 * [1, 2] == [1, 2, 1, 2, 1, 2]

assert 3 / 2 == 1.5
assert 3 // 2 == 1
assert 4 / 2 == 2.0
assert 4 // 2 == 2
