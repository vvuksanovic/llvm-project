; RUN: opt -S -passes=debug-info-analysis < %s 2>&1 | FileCheck %s
; RUN: opt -S -enable-new-pm=0 -debug-info-analysis < %s 2>&1 | FileCheck %s

; CHECK-LABEL: Function count_value:
; CHECK-NEXT: Number of llvm.dbg.value instructions:   3
; CHECK-NEXT: Number of llvm.dbg.declare instructions: 0

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK-LABEL: @count_value
define dso_local void @count_value() local_unnamed_addr !dbg !8 {
  ; CHECK-NOT: call void @llvm.dbg.value(metadata i32 2, metadata !13, metadata !DIExpression()), !dbg !16
  ; CHECK: call void @llvm.dbg.declare(metadata i32 2, metadata !13, metadata !DIExpression()), !dbg !16
  call void @llvm.dbg.value(metadata i32 2, metadata !13, metadata !DIExpression()), !dbg !16
  ; CHECK-NOT: call void @llvm.dbg.value(metadata i32 3, metadata !15, metadata !DIExpression()), !dbg !16
  ; CHECK: call void @llvm.dbg.declare(metadata i32 3, metadata !15, metadata !DIExpression()), !dbg !16
  call void @llvm.dbg.value(metadata i32 3, metadata !15, metadata !DIExpression()), !dbg !16
  ; CHECK-NOT: call void @llvm.dbg.value(metadata i32 3, metadata !13, metadata !DIExpression()), !dbg !16
  ; CHECK: call void @llvm.dbg.declare(metadata i32 3, metadata !13, metadata !DIExpression()), !dbg !16
  call void @llvm.dbg.value(metadata i32 3, metadata !13, metadata !DIExpression()), !dbg !16
  ret void, !dbg !17
}

; CHECK declare void @llvm.dbg.declare(metadata, metadata, metadata)
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
!8 = distinct !DISubprogram(name: "count_value", scope: !9, file: !9, line: 1, type: !10, scopeLine: 1, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!9 = !DIFile(filename: "count-value.c", directory: "")
!10 = !DISubroutineType(types: !11)
!11 = !{null}
!12 = !{!13, !15}
!13 = !DILocalVariable(name: "X", scope: !8, file: !9, line: 2, type: !14)
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !DILocalVariable(name: "Y", scope: !8, file: !9, line: 3, type: !14)
!16 = !DILocation(line: 0, scope: !8)
!17 = !DILocation(line: 5, column: 1, scope: !8)


