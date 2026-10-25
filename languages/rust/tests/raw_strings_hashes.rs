fn main() {
    let a = r"abc";
    let b = r#"a"b"#;
    let c = r##"a"#b"##;
    let d = r####"x"###y"####;
    let e = br####"bytes with "### inside"####;
    println!("{} {} {} {} {:?}", a, b, c, d, e);
}
