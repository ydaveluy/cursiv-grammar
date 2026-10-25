local Registry = record
   Inner = record
      value: number
   end
   items: {string: number}
   list: {number}
   callback: function(number): boolean
   thunk: function(): boolean
end

local Direction = enum
   "north"
   "south"
end

local Comparator = functiontype<T>(T, T): boolean

local function apply<T>(cmp: Comparator<T>, a: T, b: T): boolean
   return cmp(a, b)
end

local function two(): (number, string)
   return 1, "one"
end

local function accept(v: number | string): boolean
   return v is number
end

local function describe(v: any): string
   if v is number then
      return "number"
   end
   if v is string then
      return "string"
   end
   return "other"
end

local wrapped = 10 as (number | string)

local Shape = record<T>
   kind: string
   payload: T
end
