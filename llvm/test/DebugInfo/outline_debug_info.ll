; RUN: opt -passes=iroutliner -ir-outlining-no-cost=true -S < %s | FileCheck %s

; RUN: opt -passes=verify,iroutliner -ir-outlining-no-cost < %s | \
; RUN: llc -filetype=obj | llvm-dwarfdump - | FileCheck -check-prefix=DWARF %s

; CHECK: call void @outlined_ir_func_0
; CHECK-SAME: !outline_id ![[CALL1:[0-9]+]]
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF1:[0-9]+]], metadata ![[CALL1]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF2:[0-9]+]], metadata ![[CALL1]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF3:[0-9]+]], metadata ![[CALL1]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF4:[0-9]+]], metadata ![[CALL1]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF5:[0-9]+]], metadata ![[CALL1]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF6:[0-9]+]], metadata ![[CALL1]])
; CHECK-SAME: !dbg

; CHECK: call void @outlined_ir_func_0
; CHECK-SAME: !outline_id ![[CALL2:[0-9]+]]
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF1]], metadata ![[CALL2]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF2]], metadata ![[CALL2]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF3]], metadata ![[CALL2]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF4]], metadata ![[CALL2]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF5]], metadata ![[CALL2]])
; CHECK-SAME: !dbg
; CHECK-NEXT: call void @llvm.dbg.outlined(metadata ![[REF6]], metadata ![[CALL2]])
; CHECK-SAME: !dbg

; CHECK: declare void @llvm.dbg.outlined(metadata, metadata)

; Generated from
; __attribute__((noinline)) int foo(int a, int b) {
; 	return a / b * 2;
; }
; 
; int main() {
; 	int x = 0;
; 	int y = 1;
; 	
; 	int c1 = x + y;
; 	foo(c1, 2);
; 	
; 	int c2 = x + y;
; 	foo(c2, 2);
; 	
; 	return 0;
; }

; DWARF:  DW_TAG_call_site
; DWARF-NEXT: DW_AT_call_origin  (0x{{[0-9a-f]+}} {{".*"}})
; DWARF-NEXT: DW_AT_call_return_pc  (0x{{[0-9a-f]+}})
; DWARF-NEXT: DW_AT_LLVM_outlined  (true)
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: NULL

; DWARF:  DW_TAG_call_site
; DWARF-NEXT: DW_AT_call_origin  (0x{{[0-9a-f]+}} {{".*"}})
; DWARF-NEXT: DW_AT_call_return_pc  (0x{{[0-9a-f]+}})
; DWARF-NEXT: DW_AT_LLVM_outlined  (true)
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: DW_TAG_LLVM_outlined_ref
; DWARF-NEXT: DW_AT_decl_file ({{".*"}})
; DWARF-NEXT: DW_AT_decl_line  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_decl_column  ({{[0-9]+}})
; DWARF-NEXT: DW_AT_low_pc  (0x{{[0-9a-f]+}})
; DWARF-EMPTY:
; DWARF-NEXT: NULL

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @foo(i32 noundef %a, i32 noundef %b) #0 !dbg !9 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, ptr %a.addr, align 4, !tbaa !16
  call void @llvm.dbg.declare(metadata ptr %a.addr, metadata !14, metadata !DIExpression()), !dbg !20
  store i32 %b, ptr %b.addr, align 4, !tbaa !16
  call void @llvm.dbg.declare(metadata ptr %b.addr, metadata !15, metadata !DIExpression()), !dbg !21
  %0 = load i32, ptr %a.addr, align 4, !dbg !22, !tbaa !16
  %1 = load i32, ptr %b.addr, align 4, !dbg !23, !tbaa !16
  %div = sdiv i32 %0, %1, !dbg !24
  %mul = mul nsw i32 %div, 2, !dbg !25
  ret i32 %mul, !dbg !26
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind uwtable
define dso_local i32 @main() #2 !dbg !27 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %c1 = alloca i32, align 4
  %c2 = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr %x) #4, !dbg !35
  call void @llvm.dbg.declare(metadata ptr %x, metadata !31, metadata !DIExpression()), !dbg !36
  store i32 0, ptr %x, align 4, !dbg !36, !tbaa !16
  call void @llvm.lifetime.start.p0(i64 4, ptr %y) #4, !dbg !37
  call void @llvm.dbg.declare(metadata ptr %y, metadata !32, metadata !DIExpression()), !dbg !38
  store i32 1, ptr %y, align 4, !dbg !38, !tbaa !16
  call void @llvm.lifetime.start.p0(i64 4, ptr %c1) #4, !dbg !39
  call void @llvm.dbg.declare(metadata ptr %c1, metadata !33, metadata !DIExpression()), !dbg !40
  %0 = load i32, ptr %x, align 4, !dbg !41, !tbaa !16
  %1 = load i32, ptr %y, align 4, !dbg !42, !tbaa !16
  %add = add nsw i32 %0, %1, !dbg !43
  store i32 %add, ptr %c1, align 4, !dbg !40, !tbaa !16
  %2 = load i32, ptr %c1, align 4, !dbg !44, !tbaa !16
  %call = call i32 @foo(i32 noundef %2, i32 noundef 2), !dbg !45
  call void @llvm.lifetime.start.p0(i64 4, ptr %c2) #4, !dbg !46
  call void @llvm.dbg.declare(metadata ptr %c2, metadata !34, metadata !DIExpression()), !dbg !47
  %3 = load i32, ptr %x, align 4, !dbg !48, !tbaa !16
  %4 = load i32, ptr %y, align 4, !dbg !49, !tbaa !16
  %add1 = add nsw i32 %3, %4, !dbg !50
  store i32 %add1, ptr %c2, align 4, !dbg !47, !tbaa !16
  %5 = load i32, ptr %c2, align 4, !dbg !51, !tbaa !16
  %call2 = call i32 @foo(i32 noundef %5, i32 noundef 2), !dbg !52
  call void @llvm.lifetime.end.p0(i64 4, ptr %c2) #4, !dbg !53
  call void @llvm.lifetime.end.p0(i64 4, ptr %c1) #4, !dbg !53
  call void @llvm.lifetime.end.p0(i64 4, ptr %y) #4, !dbg !53
  call void @llvm.lifetime.end.p0(i64 4, ptr %x) #4, !dbg !53
  ret i32 0, !dbg !54
}

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #3

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #3

attributes #0 = { noinline nounwind uwtable }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nounwind uwtable }
attributes #3 = { argmemonly nocallback nofree nosync nounwind willreturn }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 16.0.0", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "test.c", directory: "")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{!"clang version 16.0.0"}
!9 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 1, type: !10, scopeLine: 1, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !13)
!10 = !DISubroutineType(types: !11)
!11 = !{!12, !12, !12}
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !{!14, !15}
!14 = !DILocalVariable(name: "a", arg: 1, scope: !9, file: !1, line: 1, type: !12)
!15 = !DILocalVariable(name: "b", arg: 2, scope: !9, file: !1, line: 1, type: !12)
!16 = !{!17, !17, i64 0}
!17 = !{!"int", !18, i64 0}
!18 = !{!"omnipotent char", !19, i64 0}
!19 = !{!"Simple C/C++ TBAA"}
!20 = !DILocation(line: 1, column: 39, scope: !9)
!21 = !DILocation(line: 1, column: 46, scope: !9)
!22 = !DILocation(line: 2, column: 9, scope: !9)
!23 = !DILocation(line: 2, column: 13, scope: !9)
!24 = !DILocation(line: 2, column: 11, scope: !9)
!25 = !DILocation(line: 2, column: 15, scope: !9)
!26 = !DILocation(line: 2, column: 2, scope: !9)
!27 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 5, type: !28, scopeLine: 5, flags: DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !30)
!28 = !DISubroutineType(types: !29)
!29 = !{!12}
!30 = !{!31, !32, !33, !34}
!31 = !DILocalVariable(name: "x", scope: !27, file: !1, line: 6, type: !12)
!32 = !DILocalVariable(name: "y", scope: !27, file: !1, line: 7, type: !12)
!33 = !DILocalVariable(name: "c1", scope: !27, file: !1, line: 9, type: !12)
!34 = !DILocalVariable(name: "c2", scope: !27, file: !1, line: 12, type: !12)
!35 = !DILocation(line: 6, column: 2, scope: !27)
!36 = !DILocation(line: 6, column: 6, scope: !27)
!37 = !DILocation(line: 7, column: 2, scope: !27)
!38 = !DILocation(line: 7, column: 6, scope: !27)
!39 = !DILocation(line: 9, column: 2, scope: !27)
!40 = !DILocation(line: 9, column: 6, scope: !27)
!41 = !DILocation(line: 9, column: 11, scope: !27)
!42 = !DILocation(line: 9, column: 15, scope: !27)
!43 = !DILocation(line: 9, column: 13, scope: !27)
!44 = !DILocation(line: 10, column: 6, scope: !27)
!45 = !DILocation(line: 10, column: 2, scope: !27)
!46 = !DILocation(line: 12, column: 2, scope: !27)
!47 = !DILocation(line: 12, column: 6, scope: !27)
!48 = !DILocation(line: 12, column: 11, scope: !27)
!49 = !DILocation(line: 12, column: 15, scope: !27)
!50 = !DILocation(line: 12, column: 13, scope: !27)
!51 = !DILocation(line: 13, column: 6, scope: !27)
!52 = !DILocation(line: 13, column: 2, scope: !27)
!53 = !DILocation(line: 16, column: 1, scope: !27)
!54 = !DILocation(line: 15, column: 2, scope: !27)
