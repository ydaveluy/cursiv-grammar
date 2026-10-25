=begin
A block comment that must be skipped like a single line one.
=end

flag = true
other = false
empty = nil

a = 5
b = a ** 2
c = ~a
d = not a
e = !a
f = a & 3
g = a | 3
h = a ^ 3
i = a << 1
j = a >> 1

if a != 3
  puts "not three"
elsif a <= 10
  puts "small"
end

k = 0
while k < 3
  k += 1
  if k == 2
    retry
  end
end

require "helper"

$G = 10
m = $G

arr = []
arr = [1, 2, 3]
arr[0] = 9

def greet(name, greeting)
  return greeting + name
end

n = greet(name="World", greeting="Hi")
