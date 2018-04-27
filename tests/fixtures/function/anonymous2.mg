
print(map((func(a) return a * a), range(5)))
print(map(a -> a * a, range(5)))
print()

print(map((func(a, b) return a * b), range(5), range(10, 15)))
print(map((a, b) -> a * b, range(5), range(10, 15)))
print()

f = () -> 1
print(f(), (() -> 1)())

f = (a) -> print(a)
f(123)
((a) -> print(a))(456)
print()

add, square = (a: float, b: float) -> a + b, a: float -> a * a

print(add(1, 1), add(1, 2), add(2, 1), add(2, 2))
print(square(1), square(2), square(3), square(4))
