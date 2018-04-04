
print([1, 2, 3][1])
print([[1, 2, 3], [4, 5, 6], [7, 8, 9]][1])
print([[1, 2, 3], [4, 5, 6], [7, 8, 9]][1][1])
print()

a = [1, (2, 2, 2), 3]
b = a[1]

print(a)
print(b)
print()

a[1] = 22

print(a)
print(b)
print()

b = a[1]
a[1] = (2, 22, 2)

print(a)
print(b)
print()

a = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
b = a[1]
c = b[1]

print(a)
print(b)
print(c)
print()

a[1][1] = 55
b[-1] = 444

print(a)
print(b)
print(c)
print()

a[1] = (44, 5, 66)

print(a)
print(b)
print(c)
print()

m = {"a": 1, "b": 2}
l = [m, m, m]

print(l[0]["a"], l[0]["b"])
print(l[1]["a"], l[1]["b"])
print(l[2]["a"], l[2]["b"])
print()

l[1] = (1, 2)
m["b"] = 22
m["c"] = 3

print(l[2]["a"], l[2]["b"], l[2]["c"])
print(l[1])
print(l[2]["a"], l[2]["b"], l[2]["c"])
print()
