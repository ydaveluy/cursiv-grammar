local type Comparator = function<T>(T, T): boolean

local function mysort<A>(arr: {A}, cmp: Comparator<A>)
   -- ...
end