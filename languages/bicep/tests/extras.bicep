// extras.bicep: constructs no example/ file reaches. Lives under tests/,
// never counted in the campaign's example corpus.

#disable-next-line no-unused-vars
type flag = bool

type Union = string | int | bool

type Point = {
  @description('x coord')
  x: int
  y: int
  *: string
}

type Pair = [
  @minLength(1)
  string
  int
]

type Wrapped = (string | int)[]

type Named = Point

type Neg = -1

var addFn = (a, b) => a + b
var identityFn = x => x

var escaped = 'a\nb\tc\\d\'e\$f\u{48}g'

var multi = '''
line one
line two
'''

var tagged = foo::bar

var prec = a + b * c == d && e || f ?? g
