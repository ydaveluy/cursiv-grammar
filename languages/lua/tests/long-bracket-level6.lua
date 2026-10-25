--[======[
a comment at level six -- the closer must echo exactly six `=`, not five,
not seven, and nothing bounds how high a level Lua source may choose
]======]

local sql = [========[
SELECT * FROM t WHERE note = "contains ]] and ]=] and even ]======]"
]========]

print(sql)
