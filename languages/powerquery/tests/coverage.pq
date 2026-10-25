[Attr1 = 1, Attr2 = "x"] section CoverageSection;

shared firstMember = each [Field]?;

secondMember =
    let
        a = 1,
        b = if a > 0 then a else -a,
        c = each [[A], [B]]?,
        d = {1..5, 10},
        e = d{0}?,
        f = [A = 1, B = 2, #"C D" = 3],
        g = f[A]?,
        h = f[[A], [B]],
        i = (x, optional y as number) => x + y,
        j = i(1, 2),
        k = 1 is nullable number,
        l = 1 as text,
        m = 1 meta 2,
        n = not true and false or true,
        o = 0x1F,
        p = #!"verbatim text",
        q = "a#(000A)b#(00000041)c#(	)d#(#)e#(0041,0042)f""g",
        r = .5,
        s = 1.5e10,
        t = 2E-3
    in
        Section2!Query2;

thirdMember = type [
    A = text,
    optional B = number,
    C = {number},
    D = function (x as number) as number,
    E = nullable text,
    F = (1 + 2),
    G = null,
    H = 5,
    I = "s",
    J = true,
    ...
];
