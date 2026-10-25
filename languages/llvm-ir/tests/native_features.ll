source_filename = "code-audit.ll"

%Pair = type <{ b8, i32 }>
@raw = constant [4 x i8] c"a\00\22\5C"
@base = global i32 0
@auth = global ptr ptrauth (ptr @base, i32 0, i64 7)
@tls = thread_local(localdynamic) alias i32, ptr @base
@ifunc = ifunc void (), ptr @resolver, !annotation !0

define void @implementation() {
  ret void
}
define ptr @resolver() {
  ret ptr @implementation
}
declare void @target_arg(target("spirv.Event"))
declare void @callee(ptr captures(none), i32 range(i32 0, 8))

define i64 @features(ptr %p, i32 %x, i64 %y) memory(argmem : readwrite) {
entry:
  %r = or disjoint i32 %x, 1
  %e = zext nneg i32 %x to i64
  %t = trunc nuw nsw i64 %y to i32
  %s = icmp samesign eq i32 %r, 0
  %g = getelementptr nusw nuw i32, ptr %p, i32 %x
  %fr = freeze i64 %e, !annotation !0
  call void @callee(ptr captures(ret : address, provenance) %p, i32 range(i32 0, 4) %x)
  ret i64 %fr
}

define <vscale x 4 x i32> @splat() {
  ret <vscale x 4 x i32> splat (i32 1)
}

define void @debug(i32 %x) !dbg !102 {
  #dbg_value(i32 %x, !104, !DIExpression(), !105)
  ret void
}

!0 = !{!"annotation"}
!llvm.dbg.cu = !{!100}
!llvm.module.flags = !{!106}
!100 = distinct !DICompileUnit(language: DW_LANG_C, file: !101, producer: "audit", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!101 = !DIFile(filename: "audit.c", directory: "/")
!102 = distinct !DISubprogram(name: "debug", scope: !101, file: !101, line: 1, type: !103, spFlags: DISPFlagDefinition, unit: !100)
!103 = !DISubroutineType(types: !{})
!104 = !DILocalVariable(name: "x", scope: !102, file: !101, line: 1, type: !107)
!105 = !DILocation(line: 1, column: 1, scope: !102)
!106 = !{i32 2, !"Debug Info Version", i32 3}
!107 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
