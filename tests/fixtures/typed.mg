
i: int = 1
f: float = 2.5
m: map<string, int> = {"a": 1, "b": 2, "c": 1 + 2}

func add(a: int, b: int): int
	return a + b

func test(): map<string, int>
	return m
