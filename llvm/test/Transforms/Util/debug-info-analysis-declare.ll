; RUN: opt -S -passes=debug-info-analysis -disable-output < %s | FileCheck %s
; RUN: opt -S -enable-new-pm=0 -debug-info-analysis -disable-output < %s | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define dso_local void @count_declare() !dbg !10 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  call void @llvm.dbg.declare(metadata ptr %1, metadata !15, metadata !DIExpression()), !dbg !17
  store i32 2, ptr %1, align 4, !dbg !17
  call void @llvm.dbg.declare(metadata ptr %2, metadata !18, metadata !DIExpression()), !dbg !19
  store i32 3, ptr %2, align 4, !dbg !19
  %3 = load i32, ptr %2, align 4, !dbg !20
  store i32 %3, ptr %1, align 4, !dbg !21
  ret void, !dbg !22
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "count-declare.c", directory: ".")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!10 = distinct !DISubprogram(name: "count_declare", scope: !11, file: !11, line: 1, type: !12, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!11 = !DIFile(filename: "count-declare.c", directory: ".")
!12 = !DISubroutineType(types: !13)
!13 = !{null}
!14 = !{}
!15 = !DILocalVariable(name: "X", scope: !10, file: !11, line: 2, type: !16)
!16 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!17 = !DILocation(line: 2, column: 6, scope: !10)
!18 = !DILocalVariable(name: "Y", scope: !10, file: !11, line: 3, type: !16)
!19 = !DILocation(line: 3, column: 6, scope: !10)
!20 = !DILocation(line: 4, column: 6, scope: !10)
!21 = !DILocation(line: 4, column: 4, scope: !10)
!22 = !DILocation(line: 5, column: 1, scope: !10)

; CHECK: Function count_declare:
; CHECK-NEXT: Number of llvm.dbg.value instructions:   0
; CHECK-NEXT: Number of llvm.dbg.declare instructions: 2
