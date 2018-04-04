
(a, b, c) = (1, 2, 3)
print(a, b, c)

(a, b, c) = (c, b, a)
print(a, b, c)

(a, c) = (c, a)
print(a, b, c)

func test(a, b)
	return (a * a, b * b)

(a, b) = test(2, 3)
print(a, b)
