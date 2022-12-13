; RUN: llc -stop-after="livedebugvars" -mtriple=x86_64-- < %s | FileCheck %s

; CHECK: CALL64pcrel32 @outlined_ir_func_0
; CHECK-SAME: outline-id ![[CALL1:[0-9]+]]
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE1:[0-9]+]], ![[CALL1]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE2:[0-9]+]], ![[CALL1]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE3:[0-9]+]], ![[CALL1]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE4:[0-9]+]], ![[CALL1]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE5:[0-9]+]], ![[CALL1]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE6:[0-9]+]], ![[CALL1]], debug-location !{{[0-9]+}}

; CHECK: CALL64pcrel32 @outlined_ir_func_0
; CHECK-SAME: outline-id ![[CALL2:[0-9]+]]
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE1]], ![[CALL2]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE2]], ![[CALL2]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE3]], ![[CALL2]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE4]], ![[CALL2]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE5]], ![[CALL2]], debug-location !{{[0-9]+}}
; CHECK-NEXT: DBG_OUTLINED ![[OUTLINE6]], ![[CALL2]], debug-location !{{[0-9]+}}

@global = dso_local global i32 0, align 4, !dbg !0

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @foo(i32 noundef %a, i32 noundef %b) #0 !dbg !13 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, ptr %a.addr, align 4, !tbaa !19
  call void @llvm.dbg.declare(metadata ptr %a.addr, metadata !17, metadata !DIExpression()), !dbg !23
  store i32 %b, ptr %b.addr, align 4, !tbaa !19
  call void @llvm.dbg.declare(metadata ptr %b.addr, metadata !18, metadata !DIExpression()), !dbg !24
  %0 = load i32, ptr %a.addr, align 4, !dbg !25, !tbaa !19
  %1 = load i32, ptr %b.addr, align 4, !dbg !26, !tbaa !19
  %div = sdiv i32 %0, %1, !dbg !27
  %mul = mul nsw i32 %div, 2, !dbg !28
  ret i32 %mul, !dbg !29
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind uwtable
define dso_local i32 @main() #2 !dbg !30 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %c1 = alloca i32, align 4
  %c2 = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr %x) #5, !dbg !38
  call void @llvm.dbg.declare(metadata ptr %x, metadata !34, metadata !DIExpression()), !dbg !39
  store i32 0, ptr %x, align 4, !dbg !39, !tbaa !19
  call void @llvm.lifetime.start.p0(i64 4, ptr %y) #5, !dbg !40
  call void @llvm.dbg.declare(metadata ptr %y, metadata !35, metadata !DIExpression()), !dbg !41
  store i32 1, ptr %y, align 4, !dbg !41, !tbaa !19
  call void @llvm.lifetime.start.p0(i64 4, ptr %c1) #5, !dbg !42
  call void @llvm.dbg.declare(metadata ptr %c1, metadata !36, metadata !DIExpression()), !dbg !43
  call void @outlined_ir_func_0(ptr %x, ptr %y, ptr %c1), !dbg !44, !outline_id !45
  call void @llvm.dbg.outlined(metadata !46, metadata !45), !dbg !44
  call void @llvm.dbg.outlined(metadata !47, metadata !45), !dbg !48
  call void @llvm.dbg.outlined(metadata !49, metadata !45), !dbg !50
  call void @llvm.dbg.outlined(metadata !51, metadata !45), !dbg !43
  call void @llvm.dbg.outlined(metadata !52, metadata !45), !dbg !53
  call void @llvm.dbg.outlined(metadata !54, metadata !45), !dbg !55
  call void @llvm.lifetime.start.p0(i64 4, ptr %c2) #5, !dbg !56
  call void @llvm.dbg.declare(metadata ptr %c2, metadata !37, metadata !DIExpression()), !dbg !57
  call void @outlined_ir_func_0(ptr %x, ptr %y, ptr %c2), !dbg !58, !outline_id !59
  call void @llvm.dbg.outlined(metadata !46, metadata !59), !dbg !58
  call void @llvm.dbg.outlined(metadata !47, metadata !59), !dbg !60
  call void @llvm.dbg.outlined(metadata !49, metadata !59), !dbg !61
  call void @llvm.dbg.outlined(metadata !51, metadata !59), !dbg !57
  call void @llvm.dbg.outlined(metadata !52, metadata !59), !dbg !62
  call void @llvm.dbg.outlined(metadata !54, metadata !59), !dbg !63
  call void @llvm.lifetime.end.p0(i64 4, ptr %c2) #5, !dbg !64
  call void @llvm.lifetime.end.p0(i64 4, ptr %c1) #5, !dbg !64
  call void @llvm.lifetime.end.p0(i64 4, ptr %y) #5, !dbg !64
  call void @llvm.lifetime.end.p0(i64 4, ptr %x) #5, !dbg !64
  ret i32 0, !dbg !65
}

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #3

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #3

; Function Attrs: minsize nounwind optsize uwtable
define internal void @outlined_ir_func_0(ptr %0, ptr %1, ptr %2) #4 !dbg !66 {
newFuncRoot:
  br label %entry_to_outline

entry_to_outline:                                 ; preds = %newFuncRoot
  %3 = load i32, ptr %0, align 4, !tbaa !19, !outline_id !46
  %4 = load i32, ptr %1, align 4, !tbaa !19, !outline_id !47
  %add = add nsw i32 %3, %4, !outline_id !49
  store i32 %add, ptr %2, align 4, !tbaa !19, !outline_id !51
  %5 = load i32, ptr %2, align 4, !tbaa !19, !outline_id !52
  %call = call i32 @foo(i32 noundef %5, i32 noundef 2), !dbg !69, !outline_id !54
  br label %entry_after_outline.exitStub

entry_after_outline.exitStub:                     ; preds = %entry_to_outline
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.outlined(metadata, metadata) #1

attributes #0 = { noinline nounwind uwtable }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nounwind uwtable }
attributes #3 = { argmemonly nocallback nofree nosync nounwind willreturn }
attributes #4 = { minsize nounwind optsize uwtable }
attributes #5 = { nounwind }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!6, !7, !8, !9, !10, !11}
!llvm.ident = !{!12}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "global", scope: !2, file: !3, line: 2, type: !5, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang version 16.0.0", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, globals: !4, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "test.c", directory: "")
!4 = !{!0}
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !{i32 7, !"Dwarf Version", i32 5}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = !{i32 1, !"wchar_size", i32 4}
!9 = !{i32 7, !"PIC Level", i32 2}
!10 = !{i32 7, !"PIE Level", i32 2}
!11 = !{i32 7, !"uwtable", i32 2}
!12 = !{!"clang version 16.0.0"}
!13 = distinct !DISubprogram(name: "foo", scope: !3, file: !3, line: 4, type: !14, scopeLine: 4, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !16)
!14 = !DISubroutineType(types: !15)
!15 = !{!5, !5, !5}
!16 = !{!17, !18}
!17 = !DILocalVariable(name: "a", arg: 1, scope: !13, file: !3, line: 4, type: !5)
!18 = !DILocalVariable(name: "b", arg: 2, scope: !13, file: !3, line: 4, type: !5)
!19 = !{!20, !20, i64 0}
!20 = !{!"int", !21, i64 0}
!21 = !{!"omnipotent char", !22, i64 0}
!22 = !{!"Simple C/C++ TBAA"}
!23 = !DILocation(line: 4, column: 39, scope: !13)
!24 = !DILocation(line: 4, column: 46, scope: !13)
!25 = !DILocation(line: 5, column: 9, scope: !13)
!26 = !DILocation(line: 5, column: 13, scope: !13)
!27 = !DILocation(line: 5, column: 11, scope: !13)
!28 = !DILocation(line: 5, column: 15, scope: !13)
!29 = !DILocation(line: 5, column: 2, scope: !13)
!30 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 8, type: !31, scopeLine: 8, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !33)
!31 = !DISubroutineType(types: !32)
!32 = !{!5}
!33 = !{!34, !35, !36, !37}
!34 = !DILocalVariable(name: "x", scope: !30, file: !3, line: 9, type: !5)
!35 = !DILocalVariable(name: "y", scope: !30, file: !3, line: 10, type: !5)
!36 = !DILocalVariable(name: "c1", scope: !30, file: !3, line: 12, type: !5)
!37 = !DILocalVariable(name: "c2", scope: !30, file: !3, line: 17, type: !5)
!38 = !DILocation(line: 9, column: 2, scope: !30)
!39 = !DILocation(line: 9, column: 6, scope: !30)
!40 = !DILocation(line: 10, column: 2, scope: !30)
!41 = !DILocation(line: 10, column: 6, scope: !30)
!42 = !DILocation(line: 12, column: 2, scope: !30)
!43 = !DILocation(line: 12, column: 6, scope: !30)
!44 = !DILocation(line: 12, column: 11, scope: !30)
!45 = distinct !DIOutlineId()
!46 = distinct !DIOutlineId()
!47 = distinct !DIOutlineId()
!48 = !DILocation(line: 12, column: 15, scope: !30)
!49 = distinct !DIOutlineId()
!50 = !DILocation(line: 12, column: 13, scope: !30)
!51 = distinct !DIOutlineId()
!52 = distinct !DIOutlineId()
!53 = !DILocation(line: 15, column: 6, scope: !30)
!54 = distinct !DIOutlineId()
!55 = !DILocation(line: 15, column: 2, scope: !30)
!56 = !DILocation(line: 17, column: 2, scope: !30)
!57 = !DILocation(line: 17, column: 6, scope: !30)
!58 = !DILocation(line: 17, column: 11, scope: !30)
!59 = distinct !DIOutlineId()
!60 = !DILocation(line: 17, column: 15, scope: !30)
!61 = !DILocation(line: 17, column: 13, scope: !30)
!62 = !DILocation(line: 20, column: 6, scope: !30)
!63 = !DILocation(line: 20, column: 2, scope: !30)
!64 = !DILocation(line: 23, column: 1, scope: !30)
!65 = !DILocation(line: 22, column: 2, scope: !30)
!66 = distinct !DISubprogram(name: "outlined_ir_func_0", linkageName: "outlined_ir_func_0", scope: !3, file: !3, type: !67, flags: DIFlagOutlined, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !68)
!67 = !DISubroutineType(types: !68)
!68 = !{}
!69 = !DILocation(line: 0, scope: !66)