global a b c
clear x y

for i = 1:5
  a = i + 1;
end

for (j = 1:2:9)
  b = j;
end

k = 0;
while k < 3
  k = k + 1;
  if k == 1
    a = 10;
  elseif k == 2
    a = 20;
  else
    a = 30;
  end
end

if a > 0 & b < 100 | c ~= 5
  return;
else
  break;
end

v = -a;
w = ~a;
y2 = +a;
t = a';
u = a .';
