
assert -2 as float == -2.0
assert type(-2 as float) == "float"

assert 2.5 as int == 2
assert type(2.5 as int) == "int"

assert (-2) as string == "-2"
assert type((-2) as string) == "string"

assert [1, (2, 3), 4] as string == "[1, (2, 3), 4]"
assert type([1, (2, 3), 4] as string) == "string"
