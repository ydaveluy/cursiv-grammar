const std = @import("std");

fn classify(n: i32) []const u8 {
    if (n < 0) {
        return "negative";
    } else if (n == 0) {
        return "zero";
    } else {
        return "positive";
    }
}

fn labeledLoops() i32 {
    var total: i32 = 0;
    outer: while (true) : (total += 1) {
        inner: for (0..10) |i| {
            if (i == 3) continue :inner;
            if (total > 20) break :outer total;
            total += @as(i32, @intCast(i));
        }
        if (total > 100) break;
    }
    return total;
}

fn switchy(n: i32) i32 {
    return switch (n) {
        0 => 0,
        1, 2, 3 => 1,
        4...10 => 2,
        else => blk: {
            const v = n * 2;
            break :blk v;
        },
    };
}

fn destructure(pair: struct { a: i32, b: i32 }) i32 {
    var a: i32 = undefined;
    var b: i32 = undefined;
    a, b = .{ pair.a, pair.b };
    return a + b;
}

fn compoundAssign() i32 {
    var x: i32 = 1;
    x += 1;
    x -= 1;
    x *= 2;
    x /= 2;
    x %= 3;
    x &= 1;
    x |= 2;
    x ^= 3;
    x <<= 1;
    x >>= 1;
    return x;
}

fn compareAndBitwise(a: i32, b: i32) bool {
    const c = (a & b) | (a ^ b);
    const d = a << 1 >> 1;
    return c == d and a != b or a <= b;
}

fn errorHandling(ok: bool) !i32 {
    const v = maybeFail(ok) catch |err| {
        return err;
    };
    const w = maybeFail(ok) catch 0;
    const opt: ?i32 = null;
    const u = opt orelse -1;
    return v + w + u;
}

fn maybeFail(ok: bool) !i32 {
    if (!ok) return error.Failed;
    return 42;
}

fn tryIt(ok: bool) !i32 {
    const v = try maybeFail(ok);
    return v;
}

fn pointersAndSlices(buf: []u8, arr: *[4]u8) void {
    const p: *u8 = &arr[0];
    const many: [*]u8 = buf.ptr;
    const sentinel: [*:0]const u8 = "hi";
    const cptr: [*c]u8 = @ptrCast(many);
    const slice = buf[1..3];
    const openSlice = buf[1..];
    const fullSlice = buf[0..buf.len :0];
    const alignedPtr: *align(4) u8 = @alignCast(p);
    const bitRangePtr: *align(4:0:8) u8 = @alignCast(p);
    _ = .{ p, many, sentinel, cptr, slice, openSlice, fullSlice, alignedPtr, bitRangePtr };
}

fn optionalsAndUnwrap(x: ?i32) i32 {
    if (x) |v| {
        return v;
    }
    const y: ?i32 = 5;
    return y.?;
}

fn derefAndUnreachable(p: *i32) i32 {
    p.* = 10;
    if (false) unreachable;
    return p.*;
}

fn asmExample() i32 {
    var result: i32 = 0;
    asm volatile ("nop"
        : [ret] "=r" (-> i32),
        : [inp] "r" (result),
        : "memory"
    );
    return result;
}

fn builtins(a: i32, b: i32) i32 {
    return @max(a, b) + @min(a, b) + @intFromBool(a > b);
}

fn prefixOps(a: i32, b: bool) i32 {
    const n = -a;
    const m = ~a;
    const nn = !b;
    _ = nn;
    return n +% m -% a *% 1;
}

fn charAndFloat() void {
    const c: u8 = 'x';
    const f: f64 = 3.14;
    const e: f64 = 1.5e10;
    _ = .{ c, f, e };
}
