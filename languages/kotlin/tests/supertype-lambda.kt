open class Base(val f: () -> Int)
open class Generic<T>(val g: () -> T)

class WithLambda : Base({ 1 })
class WithCallAndLambda : Base ({ 2 })
class WithTypeArgsAndLambda : Generic<Int>({ 3 })
class WithTrailingLambdaOnly : Base { 4 }
