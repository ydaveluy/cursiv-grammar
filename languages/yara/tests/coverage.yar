rule coverage
{
 strings:
  $re = /abc[0-9]+/i
  $re2 = /literal/is
 condition:
  not filesize == 0
  and (true or false)
  and (1 + 2 * 3 - 4 \ 2 % 5) == 5
  and (1 << 2) == 4
  and (8 >> 1) == 4
  and (1 & 2 | 4 ^ 1) == 5
  and filesize >= 1
  and filesize <= 1000000
  and filesize != 0
  and "name" icontains "x"
  and "name" istartswith "y"
  and "name" iendswith "z"
  and "name" iequals "w"
  and "name" endswith "v"
  and "name" startswith "u"
  and #re in (0..filesize)
  and for any i in (1, 2, 3) : (i > 0)
  and for 1 x in (0..10) : (x >= 0)
  and for any of them : ($ and # >= 0 and @[1] >= 0 and ![1] >= 0)
  and $re
}
