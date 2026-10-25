fun f() {
    outer@ class Local {
        fun g() {}
    }
    labelled@ fun h() {}
    tagged@ val x = 1
    named@ typealias LocalAlias = Local
}
