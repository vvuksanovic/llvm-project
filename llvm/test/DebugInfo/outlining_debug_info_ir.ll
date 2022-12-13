; RUN: opt -passes=iroutliner -ir-outlining-no-cost -S < %s | FileCheck %s

; RUN: opt -passes=iroutliner -ir-outlining-no-cost < %s | \
; RUN: llc -filetype=obj | llvm-dwarfdump -debug-info - | FileCheck -check-prefix=DWARF %s

; DWARF: DW_TAG_subprogram
; DWARF:      DW_AT_linkage_name ("outlined_ir_func_1")
; DWARF-NEXT: DW_AT_name ("outlined_ir_func_1")
; DWARF-NEXT: DW_AT_LLVM_outlined (true)

; DWARF:      DW_AT_linkage_name ("outlined_ir_func_0")
; DWARF-NEXT: DW_AT_name ("outlined_ir_func_0")
; DWARF-NEXT: DW_AT_LLVM_outlined  (true)

@.str = private unnamed_addr constant [14 x i8] c"Hello World!\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @main() #0 !dbg !7 {
entry:
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %0 = load i32, i32* %x, align 4
  %add = add nsw i32 %0, 1
  %add1 = add nsw i32 %add, 1
  %add2 = add nsw i32 %add1, 1
  %add3 = add nsw i32 %add2, 1
  %add4 = add nsw i32 %add3, 1
  %add5 = add nsw i32 %add4, 1
  store i32 %add5, i32* %x
  %1 = load i32, i32* %y, align 4
  %add6 = add nsw i32 %1, 1
  %add7 = add nsw i32 %add6, 1
  %add8 = add nsw i32 %add7, 1
  %add9 = add nsw i32 %add8, 1
  %add10 = add nsw i32 %add9, 1
  %add11 = add nsw i32 %add10, 1
  store i32 %add11, i32* %y
  store i32 0, i32* %x, align 4
  %2 = bitcast i32* %y to i8*
  store i32 0, i32* %x, align 4
  %3 = bitcast i32* %y to i8*
  store i32 0, i32* %x, align 4
  %4 = bitcast i32* %y to i8*
  store i32 0, i32* %x, align 4
  %5 = bitcast i32* %y to i8*
  %call = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i64 0, i64 0))
  store i32 0, i32* %y, align 4
  %6 = bitcast i32* %x to i8*
  store i32 0, i32* %y, align 4
  %7 = bitcast i32* %x to i8*
  store i32 0, i32* %y, align 4
  %8 = bitcast i32* %x to i8*
  store i32 0, i32* %y, align 4
  %9 = bitcast i32* %x to i8*
  %call1 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i64 0, i64 0))
  ret i32 0
}

declare dso_local i32 @printf(i8* noundef, ...)

attributes #0 = { nounwind uwtable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.0", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "test.c", directory: "/dir", checksumkind: CSK_MD5, checksum: "c55e81c20d55d4f337262bccf7b5d12c")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 1}
!6 = !{!"clang version 15.0.0"}
!7 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !8, scopeLine: 3, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!8 = !DISubroutineType(types: !9)
!9 = !{!10}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !{!12, !13}
!12 = !DILocalVariable(name: "x", scope: !7, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "y", scope: !7, file: !1, line: 4, type: !10)
; CHECK: ![[#OUTLINED_SUBROUTINE0:]] = distinct !DISubprogram(name: "outlined_ir_func_0",
; CHECK-SAME: linkageName: "outlined_ir_func_0", scope: !1, file: !1,
; CHECK-SAME: type: ![[#OUTLINED_SUBROUTINE0+1]], flags: DIFlagArtificial | DIFlagOutlined,
; CHECK-SAME: spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
; CHECK: !{{[1-9][0-9]+}} = distinct !DISubprogram(name: "outlined_ir_func_1",
; CHECK-SAME: linkageName: "outlined_ir_func_1", scope: !1, file: !1,
; CHECK-SAME: type: ![[#OUTLINED_SUBROUTINE0+1]], flags: DIFlagArtificial | DIFlagOutlined,
; CHECK-SAME: spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
