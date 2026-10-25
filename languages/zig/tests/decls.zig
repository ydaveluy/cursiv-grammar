//! Container doc comment for this file.

/// A struct with a backing integer type and aligned, defaulted fields.
const Point = struct(u64) {
    x: i32 align(4) = 0,
    y: i32 = 0,

    comptime scale: f32 = 1.0,

    pub fn origin() Point {
        return .{ .x = 0, .y = 0 };
    }
};

const Color = enum(u8) { red, green, blue };

const Tagged = union(enum(u8)) {
    number: i32,
    flag: bool,
};

const Untagged = union {
    number: i32,
    flag: bool,
};

const Empty = opaque {};

const MyError = error{
    /// out of range
    OutOfRange,
    NotFound,
};

pub const AliasedError = error{Other} || MyError;

pub export fn add(a: i32, b: i32) i32 {
    return a + b;
}

extern "c" fn puts(s: [*:0]const u8) i32;

pub inline fn square(x: i32) i32 {
    return x * x;
}

noinline fn slow(x: i32) i32 {
    return x;
}

threadlocal var counter: i32 = 0;

pub var shared: i32 align(8) linksection(".data") = 0;

fn variadicLike(comptime T: type, noalias p: *T, rest: anytype) void {
    _ = p;
    _ = rest;
}

fn withConv(x: i32) callconv(.C) i32 {
    return x;
}

fn generic(x: anytype) @TypeOf(x) {
    return x;
}

fn returnsError() !void {
    return;
}

test "addition works" {
    const r = add(1, 2);
    _ = r;
}

test add {
    _ = add(2, 2);
}

comptime {
    const x = 1 + 1;
    _ = x;
}
