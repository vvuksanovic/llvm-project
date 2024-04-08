# Saving debug locations of outlined instructions

## Problem

When the debugger reaches an instruction that has been outlined by the compiler and steps over (or into) it, the debugger will suddenly skip over all instructions that have been outlined in that group. This is very confusing for the end user as there is also no indication of why the lines were skipped.

Looking at the following example:
```c
int global;

__attribute__((noinline)) int foo(int a, int b) {
	return a / b * 2;
}

int main() {
	int x = 0;
	int y = 1;
	
	int c1 = x + y; # outlined region 1
	c1--;
	global += 2;
	foo(c1, global);
	
	int c2 = x + y; # outlined region 2
	c2--;
	global += 2;
	foo(c2, global);
	
	return c1;
}
```
Compiled with these commands:
```shell
$ clang -Xclang -disable-llvm-passes -g -O3 -S -emit-llvm outlined.c -o outlined.ll
$ opt -passes="iroutliner" -ir-outlining-no-cost=true -S outlined.ll -o outlined.ll
$ llc -filetype=obj outlined.ll -o outlined.o
$ clang outlined.o -o outlined
```
This happens when debugging:
```text
(lldb) next
* thread #1, name = 'outlined', stop reason = breakpoint 1.1
    frame #0: 0x000055555555516e outlined`main at outlined.c:9:12
   6   	  int x = 0;
   7   	  int y = 1;
   8   	
-> 9   	  int c1 = x + y;
   10  	  global += 2;
   11  	  foo(c1, global);
   12  	
(lldb) next
* thread #1, name = 'outlined', stop reason = step over
    frame #0: 0x000055555555517e outlined`main at outlined.c:13:12
   10  	  global += 2;
   11  	  foo(c1, global);
   12  	
-> 13  	  int c2 = x + y;
   14  	  global += 2;
   15  	  foo(c2, global);
   16  	
(lldb) next
* thread #1, name = 'outlined', stop reason = step over
    frame #0: 0x0000555555555189 outlined`main at outlined.c:17:10
   14  	  global += 2;
   15  	  foo(c2, global);
   16  	
-> 17  	  return c1;
   18  	}
```

The debugger jumped from line 11 to 16 and immediately after that to line 21.

## Solution

The main idea is to save locations of these instructions in the `.debug_info` section instead of `.debug_line`. Every call site DIE that references an outlined function should add 
`DW_TAG_LLVM_outlined_ref` DIEs as children for each instruction in the callee that would have had a debug location before outlining. Each of those tags contains source code location information (file, line and column) and the address in the executable of the corresponding instruction. This is a snippet of a call site tag from the compiled example code:

```text
0x00000093:     DW_TAG_call_site
                  DW_AT_call_origin	(0x000000e8 "outlined_ir_func_0")
                  DW_AT_call_return_pc	(0x0000000000001179)
                  DW_AT_LLVM_outlined	(true)

0x00000099:       DW_TAG_LLVM_outlined_ref
                    DW_AT_decl_file	("outlined.c")
                    DW_AT_decl_line	(9)
                    DW_AT_decl_column	(12)
                    DW_AT_low_pc	(0x0000000000001196)

0x0000009e:       DW_TAG_LLVM_outlined_ref
                    DW_AT_decl_file	("outlined.c")
                    DW_AT_decl_line	(9)
                    DW_AT_decl_column	(16)
                    DW_AT_low_pc	(0x0000000000001198)

0x000000a3:       DW_TAG_LLVM_outlined_ref
                    DW_AT_decl_file	("outlined.c")
                    DW_AT_decl_line	(9)
                    DW_AT_decl_column	(7)
                    DW_AT_low_pc	(0x000000000000119a)
```

To make that happen, new `outline_id` metadata is added to each outlined instruction and the generated call instruction. Then new `llvm.dbg.outlined` debug intrinsic is added after the generated call for each outlined instruction. Those intrinsics have 2 arguments: a pointer to `outline_id` of the corresponding outlined instruction and a pointer to the `outline_id` of the call instruction to the outlined function.

```llvm
define dso_local i32 @main() #2 !dbg !30 {
   ...
   call void @outlined_ir_func_0(ptr %x, ptr %y, ptr %c1), !dbg !44, !outline_id !45
   call void @llvm.dbg.outlined(metadata !46, metadata !45), !dbg !44
   call void @llvm.dbg.outlined(metadata !47, metadata !45), !dbg !48
   call void @llvm.dbg.outlined(metadata !49, metadata !45), !dbg !50
   call void @llvm.dbg.outlined(metadata !51, metadata !45), !dbg !43
   call void @llvm.dbg.outlined(metadata !52, metadata !45), !dbg !53
   call void @llvm.dbg.outlined(metadata !54, metadata !45), !dbg !53
   call void @llvm.dbg.outlined(metadata !55, metadata !45), !dbg !53
   call void @llvm.dbg.outlined(metadata !56, metadata !45), !dbg !57
   call void @llvm.dbg.outlined(metadata !58, metadata !45), !dbg !57
   call void @llvm.dbg.outlined(metadata !59, metadata !45), !dbg !57
   call void @llvm.dbg.outlined(metadata !60, metadata !45), !dbg !61
   call void @llvm.dbg.outlined(metadata !62, metadata !45), !dbg !63
   call void @llvm.dbg.outlined(metadata !64, metadata !45), !dbg !65
   ...
}
...
define internal void @outlined_ir_func_0(ptr %0, ptr %1, ptr %2) {
   ...
   %3 = load i32, ptr %0, !outline_id !46
   %4 = load i32, ptr %1, !outline_id !47
   %add = add nsw i32 %3, %4, !outline_id !49
   store i32 %add, ptr %2, !outline_id !51
   %5 = load i32, ptr %2, !outline_id !52
   %dec = add nsw i32 %5, -1, !outline_id !54
   store i32 %dec, ptr %2, !outline_id !55
   %6 = load i32, ptr @global, !outline_id !56
   %add1 = add nsw i32 %6, 2, !outline_id !58
   store i32 %add1, ptr @global, !outline_id !59
   %7 = load i32, ptr %2, !outline_id !60
   %8 = load i32, ptr @global, !outline_id !62
   %call = call i32 @foo(i32 noundef %7, i32 noundef %8), !outline_id !64
   ...
}
```

Those intrinsics are lowered to new `DBG_OUTLINED` debug intrinsics in MIR, just like `DBG_LABEL` or `DBG_VALUE`.

```yaml
name: main
...
body:
   ...
   CALL64pcrel32 @outlined_ir_func_0, outline-id !45, debug-location !44
   ADJCALLSTACKUP64 0, 0, debug-location !44
   DBG_OUTLINED !46, !45, debug-location !44
   DBG_OUTLINED !47, !45, debug-location !48
   DBG_OUTLINED !49, !45, debug-location !50
   DBG_OUTLINED !51, !45, debug-location !43
   DBG_OUTLINED !52, !45, debug-location !53
   DBG_OUTLINED !54, !45, debug-location !53
   DBG_OUTLINED !55, !45, debug-location !53
   DBG_OUTLINED !56, !45, debug-location !57
   DBG_OUTLINED !58, !45, debug-location !57
   DBG_OUTLINED !59, !45, debug-location !57
   DBG_OUTLINED !60, !45, debug-location !61
   DBG_OUTLINED !62, !45, debug-location !63
   DBG_OUTLINED !64, !45, debug-location !65
   ...
```

Finally, these instructions are turned into `DW_TAG_LLVM_outlined_ref` DIEs and placed in the appropriate call site DIE.

```text
0x000000a3:       DW_TAG_LLVM_outlined_ref
                    DW_AT_decl_file	("outlined.c")
                    DW_AT_decl_line	(9)
                    DW_AT_decl_column	(7)
                    DW_AT_low_pc	(0x000000000000119a)
```

On the LLDB side, a separate line table for outlined instructions is created by parsing these `DW_TAG_LLVM_outlined_ref` DIEs. That line table is used to map source locations to addresses (e.g. for placing breakpoints). Mapping addresses to source locations is a little more complicated. `DW_TAG_LLVM_outlined_ref` DIEs are parsed with other call site data, and saved in `CallEdge` objects which can be accessed through the caller function. Then during execution, when inside an outlined function, the appropriate call edge can be found using the return PC and its address and source location info can be used.

Using this information, it is possible to improve stepping through code and placing breakpoints on outlined code. A prototype implementation is available [here](https://github.com/vvuksanovic/llvm-project/tree/outline-debug-info).

## Results

Now LLDB properly stops on outlined instructions, and displays a note when that is the case.
```text
(lldb) step
* thread #1, name = 'outlined', stop reason = breakpoint 1.1
    frame #0: 0x000055555555516e outlined`main at outlined.c:9:12
   6   	  int x = 0;
   7   	  int y = 1;
   8   	
-> 9   	  int c1 = x + y;
   10  	  global += 2;
   11  	  foo(c1, global);
   12  	
(lldb) step
* thread #1, name = 'outlined', stop reason = step in
    frame #0: 0x0000555555555196 outlined`outlined_ir_func_0 at outlined.c:0
   6   	  int x = 0;
   7   	  int y = 1;
   8   	
-> 9   	  int c1 = x + y;
   10  	  global += 2;
   11  	  foo(c1, global);
   12  	
Note: this function is outlined.
(lldb) step
* thread #1, name = 'outlined', stop reason = step in
    frame #0: 0x000055555555519c outlined`outlined_ir_func_0 at outlined.c:0
   7   	  int y = 1;
   8   	
   9   	  int c1 = x + y;
-> 10  	  global += 2;
   11  	  foo(c1, global);
   12  	
   13  	  int c2 = x + y;
Note: this function is outlined.
(lldb) step
* thread #1, name = 'outlined', stop reason = step in
    frame #0: 0x00005555555551a2 outlined`outlined_ir_func_0 at outlined.c:0
   8   	
   9   	  int c1 = x + y;
   10  	  global += 2;
-> 11  	  foo(c1, global);
   12  	
   13  	  int c2 = x + y;
   14  	  global += 2;
Note: this function is outlined.
(lldb) step
* thread #1, name = 'outlined', stop reason = step in
    frame #0: 0x000055555555513a outlined`foo(a=1, b=2) at outlined.c:3:60
   1   	int global;
   2   	
-> 3   	__attribute__((noinline)) int foo(int a, int b) { return a / b * 2; }
   4   	
   5   	int main() {
   6   	  int x = 0;
   7   	  int y = 1;
```
Additionally, the solution also enables placing breakpoints on outlined code:
```text
(lldb) b 10
Breakpoint 1: where = outlined`outlined_ir_func_0 + 7 at outlined.c, address = 0x000000000000119c
(lldb) run
* thread #1, name = 'outlined', stop reason = breakpoint 1.1
    frame #0: 0x000055555555519c outlined`outlined_ir_func_0 at outlined.c:0
   7   	  int y = 1;
   8   	
   9   	  int c1 = x + y;
-> 10  	  global += 2;
   11  	  foo(c1, global);
   12  	
   13  	  int c2 = x + y;
Note: this function is outlined.
```

## To do

- Port to [debug records](https://llvm.org/docs/SourceLevelDebugging.html#debug-records): https://llvm.org/docs/RemoveDIsDebugInfo.html
- Add support for MachineOutliner pass
- Add support for GlobalIsel
