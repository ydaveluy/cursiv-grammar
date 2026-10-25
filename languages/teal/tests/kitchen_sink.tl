local Point = record
   x: number
   y: number
end

local function distance(a: Point, b: Point): number
   local dx: number = a.x - b.x
   local dy: number = b.y - a.y
   return (dx * dx + dy * dy) ^ 0.5
end

function Point:magnitude(): number
   return (self.x * self.x + self.y * self.y) ^ 0.5
end

local origin: Point = { x = 0, y = 0 }
local p: Point = { x = 3, y = 4 }

global Debug: boolean = true
global function log(msg: string)
   if Debug then
      print(msg)
   end
end
global Version: string
global Limit <const>: number = 10
global Handler = functiontype(string): boolean

local K <const> = 100

local function classify(n: number): string
   if n < 0 then
      return "negative"
   elseif n == 0 then
      return "zero"
   else
      return "positive"
   end
end

local sum = 0
for i = 1, 10 do
   sum = sum + i
end

local names = {"a", "b", "c"}
for i, name in ipairs(names) do
   if name == "b" then
      goto continue
   end
   print(i, name)
   ::continue::
end

local i = 0
while i < 3 do
   i = i + 1
end

repeat
   i = i - 1
until i <= 0

local x, y = 1, 2
x, y = y, x

local flags: number = (1 & 2) | (4 ~ 8)
local shifted = flags << 1
local text = "a" .. "b" .. "c"
local negated = -sum
local notb = not Debug
local sz = #names

local function variadic(...: number): number
   local total = 0
   for _, v in ipairs({...}) do
      total = total + v
   end
   return total
end

local total = variadic(1, 2, 3)
