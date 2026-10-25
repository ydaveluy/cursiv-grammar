extern fn printf(fmt: [*:0]const u8, ...) i32;

fn withAddrSpace(p: *addrspace(.generic) u8) void {
    _ = p;
}

const GpuPtr = [*]addrspace(.global) u8;

fn tagIndex(x: union(enum) { a: i32, b: bool }) i32 {
    return switch (x) {
        .a => |v| v,
        .b => |v| if (v) 1 else 0,
    };
}

fn param(
    /// the left operand
    a: i32,
    comptime b: i32,
) i32 {
    return a + b;
}

test {
    _ = tagIndex(.{ .a = 1 });
}
