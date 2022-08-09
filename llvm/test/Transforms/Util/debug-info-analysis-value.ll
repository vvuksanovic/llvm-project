; RUN: opt -S -passes=debug-info-analysis -disable-output < %s | FileCheck %s
; RUN: opt -S -enable-new-pm=0 -debug-info-analysis -disable-output < %s | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define dso_local void @count_value() local_unnamed_addr !dbg !9 {
  call void @llvm.dbg.value(metadata i32 2, metadata !14, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 3, metadata !16, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 3, metadata !14, metadata !DIExpression()), !dbg !17
  ret void, !dbg !18
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "count-value.c", directory: ".")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!9 = distinct !DISubprogram(name: "count_value", scope: !10, file: !10, line: 1, type: !11, scopeLine: 1, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !13)
!10 = !DIFile(filename: "count-value.c", directory: "")
!11 = !DISubroutineType(types: !12)
!12 = !{null}
!13 = !{!14, !16}
!14 = !DILocalVariable(name: "X", scope: !9, file: !10, line: 2, type: !15)
!15 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!16 = !DILocalVariable(name: "Y", scope: !9, file: !10, line: 3, type: !15)
!17 = !DILocation(line: 0, scope: !9)
!18 = !DILocation(line: 5, column: 1, scope: !9)

; CHECK: Function count_value:
; CHECK-NEXT: Number of llvm.dbg.value instructions:   3
; CHECK-NEXT: Number of llvm.dbg.declare instructions: 0
