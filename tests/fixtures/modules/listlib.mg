
import list

a = []
list.add_from(a, range(5))

print(a)
print()

print(list.reverse(a))
print(list.reverse(a))
print()

print(list.sort(a, (func(a, b) return a < b)))
print(list.sort(a, (func(a, b) return a > b)))
print()

print(a)
print()

print(list.slice(a))
print(list.slice(a, 2))
print(list.slice(a, -2))
print(list.slice(a, 1, -1))
print(list.slice(a, 0, len(a), 2))
print(list.slice(a, 1, -1, 2))
