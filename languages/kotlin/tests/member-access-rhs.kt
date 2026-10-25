fun foo(a: Int) {
    when (a) {
        a.foo -> a
        a.{bar} -> a
        a.{!bar} -> a
        a.{ -> !bar} -> a
        a.(a) -> a
        a."x" -> a
        a.if (a) 1 else 2 -> a
        else -> a
    }
}
