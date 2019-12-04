# Android PLT hook 概述


## 获取代码和资源

你始终可以从 [这里](https://github.com/iqiyi/xHook/blob/master/docs/overview/android_plt_hook_overview.zh-CN.md) 访问本文的最新版本。

文中使用的示例代码可以从 [这里](https://github.com/iqiyi/xHook/tree/master/docs/overview/code) 获取。文中提到的 xhook 开源项目可以从 [这里](https://github.com/iqiyi/xHook) 获取。 


## 开始


### 新的动态库

我们有一个新的动态库：libtest.so。

> 头文件 test.h

```c
#ifndef TEST_H
#define TEST_H 1

#ifdef __cplusplus
extern "C" {
#endif

void say_hello();

#ifdef __cplusplus
}
#endif

#endif
```

> 源文件 test.c

```c
#include <stdlib.h>
#include <stdio.h>

void say_hello()
{
    char *buf = malloc(1024);
    if(NULL != buf)
    {
        snprintf(buf, 1024, "%s", "hello\n");
        printf("%s", buf);
    }
}
```

`say_hello` 的功能是在终端打印出 `hello\n` 这6个字符（包括结尾的 `\n`）。

我们需要一个测试程序：main。

> 源文件 main.c

```c
#include <test.h>

int main()
{
    say_hello();
    return 0;
}
```

编译它们分别生成 libtest.so 和 main。运行一下：

```
caikelun@debian:~$ adb push ./libtest.so ./main /data/local/tmp
caikelun@debian:~$ adb shell "chmod +x /data/local/tmp/main"
caikelun@debian:~$ adb shell "export LD_LIBRARY_PATH=/data/local/tmp; /data/local/tmp/main"
hello
caikelun@debian:~$
```

太棒了！libtest.so 的代码虽然看上去有些愚蠢，但是它居然可以正确的工作，那还有什么可抱怨的呢？赶紧在新版 APP 中开始使用它吧！

遗憾的是，正如你可能已经发现的，libtest.so 存在严重的内存泄露问题，每调用一次 `say_hello` 函数，就会泄露 1024 字节的内存。新版 APP 上线后崩溃率开始上升，各种诡异的崩溃信息和报障信息跌撞而至。


### 面临的问题

幸运的是，我们修复了 libtest.so 的问题。可是以后怎么办呢？我们面临 2 个问题：

1. 当测试覆盖不足时，如何及时发现和准确定位线上 APP 的此类问题？
2. 如果 libtest.so 是某些机型的系统库，或者第三方的闭源库，我们如何修复它？如果监控它的行为？


### 怎么做？

如果我们能对动态库中的函数调用做 hook（替换，拦截，窃听，或者你觉得任何正确的描述方式），那就能够做到很多我们想做的事情。比如 hook `malloc`，`calloc`，`realloc` 和 `free`，我们就能统计出各个动态库分配了多少内存，哪些内存一直被占用没有释放。

这真的能做到吗？答案是：hook 我们自己的进程是完全可以的。hook 其他进程需要 root 权限（对于其他进程，没有 root 权限就没法修改它的内存空间，也没法注入代码）。幸运的是，我们只要 hook 自己就够了。


## ELF


### 概述

ELF（Executable and Linkable Format）是一种行业标准的二进制数据封装格式，主要用于封装可执行文件、动态库、object 文件和 core dumps 文件。

使用 google NDK 对源代码进行编译和链接，生成的动态库或可执行文件都是 ELF 格式的。用 readelf 可以查看 ELF 文件的基本信息，用 objdump 可以查看 ELF 文件的反汇编输出。

ELF 格式的概述可以参考 [这里](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)，完整定义可以参考 [这里](http://refspecs.linuxbase.org/elf/elf.pdf)。其中最重要的部分是：ELF 文件头、SHT（section header table）、PHT（program header table）。


### ELF 文件头

ELF 文件的起始处，有一个固定格式的定长的文件头（32 位架构为 52 字节，64 位架构为 64 字节）。ELF 文件头以 magic number `0x7F 0x45 0x4C 0x46` 开始（其中后 3 个字节分别对应可见字符 `E` `L` `F`）。

libtest.so 的 ELF 文件头信息：

```
caikelun@debian:~$ arm-linux-androideabi-readelf -h ./libtest.so
 
ELF Header:
  Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF32
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Shared object file)
  Machine:                           ARM
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          52 (bytes into file)
  Start of section headers:          12744 (bytes into file)
  Flags:                             0x5000200, Version5 EABI, soft-float ABI
  Size of this header:               52 (bytes)
  Size of program headers:           32 (bytes)
  Number of program headers:         8
  Size of section headers:           40 (bytes)
  Number of section headers:         25
  Section header string table index: 24
```

ELF 文件头中包含了 SHT 和 PHT 在当前 ELF 文件中的起始位置和长度。例如，libtest.so 的 SHT 起始位置为 12744，长度 40 字节；PHT 起始位置为 52，长度 32字节。

![](https://raw.githubusercontent.com/iqiyi/xHook/master/docs/overview/res/elfheader.png)


### SHT（section header table）

ELF 以 section 为单位来组织和管理各种信息。ELF 使用 SHT 来记录所有 section 的基本信息。主要包括：section 的类型、在文件中的偏移量、大小、加载到内存后的虚拟内存相对地址、内存中字节的对齐方式等。

libtest.so 的 SHT：

```
caikelun@debian:~$ arm-linux-androideabi-readelf -S ./libtest.so
 
There are 25 section headers, starting at offset 0x31c8:

Section Headers:
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            00000000 000000 000000 00      0   0  0
  [ 1] .note.android.ide NOTE            00000134 000134 000098 00   A  0   0  4
  [ 2] .note.gnu.build-i NOTE            000001cc 0001cc 000024 00   A  0   0  4
  [ 3] .dynsym           DYNSYM          000001f0 0001f0 0003a0 10   A  4   1  4
  [ 4] .dynstr           STRTAB          00000590 000590 0004b1 00   A  0   0  1
  [ 5] .hash             HASH            00000a44 000a44 000184 04   A  3   0  4
  [ 6] .gnu.version      VERSYM          00000bc8 000bc8 000074 02   A  3   0  2
  [ 7] .gnu.version_d    VERDEF          00000c3c 000c3c 00001c 00   A  4   1  4
  [ 8] .gnu.version_r    VERNEED         00000c58 000c58 000020 00   A  4   1  4
  [ 9] .rel.dyn          REL             00000c78 000c78 000040 08   A  3   0  4
  [10] .rel.plt          REL             00000cb8 000cb8 0000f0 08  AI  3  18  4
  [11] .plt              PROGBITS        00000da8 000da8 00017c 00  AX  0   0  4
  [12] .text             PROGBITS        00000f24 000f24 0015a4 00  AX  0   0  4
  [13] .ARM.extab        PROGBITS        000024c8 0024c8 00003c 00   A  0   0  4
  [14] .ARM.exidx        ARM_EXIDX       00002504 002504 000100 08  AL 12   0  4
  [15] .fini_array       FINI_ARRAY      00003e3c 002e3c 000008 04  WA  0   0  4
  [16] .init_array       INIT_ARRAY      00003e44 002e44 000004 04  WA  0   0  1
  [17] .dynamic          DYNAMIC         00003e48 002e48 000118 08  WA  4   0  4
  [18] .got              PROGBITS        00003f60 002f60 0000a0 00  WA  0   0  4
  [19] .data             PROGBITS        00004000 003000 000004 00  WA  0   0  4
  [20] .bss              NOBITS          00004004 003004 000000 00  WA  0   0  1
  [21] .comment          PROGBITS        00000000 003004 000065 01  MS  0   0  1
  [22] .note.gnu.gold-ve NOTE            00000000 00306c 00001c 00      0   0  4
  [23] .ARM.attributes   ARM_ATTRIBUTES  00000000 003088 00003b 00      0   0  1
  [24] .shstrtab         STRTAB          00000000 0030c3 000102 00      0   0  1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  y (noread), p (processor specific)
```

比较重要，且和 hook 关系比较大的几个 section 是：

* `.dynstr`：保存了所有的字符串常量信息。
* `.dynsym`：保存了符号（symbol）的信息（符号的类型、起始地址、大小、符号名称在 `.dynstr` 中的索引编号等）。函数也是一种符号。
* `.text`：程序代码经过编译后生成的机器指令。
* `.dynamic`：供动态链接器使用的各项信息，记录了当前 ELF 的外部依赖，以及其他各个重要 section 的起始位置等信息。
* `.got`：Global Offset Table。用于记录外部调用的入口地址。动态链接器（linker）执行重定位（relocate）操作时，这里会被填入真实的外部调用的绝对地址。
* `.plt`：Procedure Linkage Table。外部调用的跳板，主要用于支持 lazy binding 方式的外部调用重定位。（Android 目前只有 MIPS 架构支持 lazy binding）
* `.rel.plt`：对外部函数直接调用的重定位信息。
* `.rel.dyn`：除 `.rel.plt` 以外的重定位信息。（比如通过全局函数指针来调用外部函数）

![](https://raw.githubusercontent.com/iqiyi/xHook/master/docs/overview/res/elfpltgot.png)


### PHT（program header table）

ELF 被加载到内存时，是以 segment 为单位的。一个 segment 包含了一个或多个 section。ELF 使用 PHT 来记录所有 segment 的基本信息。主要包括：segment 的类型、在文件中的偏移量、大小、加载到内存后的虚拟内存相对地址、内存中字节的对齐方式等。

libtest.so 的 PHT：

```
caikelun@debian:~$ arm-linux-androideabi-readelf -l ./libtest.so 

Elf file type is DYN (Shared object file)
Entry point 0x0
There are 8 program headers, starting at offset 52

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  PHDR           0x000034 0x00000034 0x00000034 0x00100 0x00100 R   0x4
  LOAD           0x000000 0x00000000 0x00000000 0x02604 0x02604 R E 0x1000
  LOAD           0x002e3c 0x00003e3c 0x00003e3c 0x001c8 0x001c8 RW  0x1000
  DYNAMIC        0x002e48 0x00003e48 0x00003e48 0x00118 0x00118 RW  0x4
  NOTE           0x000134 0x00000134 0x00000134 0x000bc 0x000bc R   0x4
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x00000 RW  0x10
  EXIDX          0x002504 0x00002504 0x00002504 0x00100 0x00100 R   0x4
  GNU_RELRO      0x002e3c 0x00003e3c 0x00003e3c 0x001c4 0x001c4 RW  0x4

 Section to Segment mapping:
  Segment Sections...
   00     
   01     .note.android.ident .note.gnu.build-id .dynsym .dynstr .hash .gnu.version .gnu.version_d .gnu.version_r .rel.dyn .rel.plt .plt .text .ARM.extab .ARM.exidx 
   02     .fini_array .init_array .dynamic .got .data 
   03     .dynamic 
   04     .note.android.ident .note.gnu.build-id 
   05     
   06     .ARM.exidx 
   07     .fini_array .init_array .dynamic .got
```

所有类型为 `PT_LOAD` 的 segment 都会被动态链接器（linker）映射（mmap）到内存中。


### 连接视图（Linking View）和执行视图（Execution View）

* 连接视图：ELF 未被加载到内存执行前，以 section 为单位的数据组织形式。
* 执行视图：ELF 被加载到内存后，以 segment 为单位的数据组织形式。

我们关心的 hook 操作，属于动态形式的内存操作，因此主要关心的是执行视图，即 ELF 被加载到内存后，ELF 中的数据是如何组织和存放的。

![](https://raw.githubusercontent.com/iqiyi/xHook/master/docs/overview/res/elfview.png)


### .dynamic section

这是一个十分重要和特殊的 section，其中包含了 ELF 中其他各个 section 的内存位置等信息。在执行视图中，总是会存在一个类型为 `PT_DYNAMIC` 的 segment，这个 segment 就包含了 .dynamic section 的内容。

无论是执行 hook 操作时，还是动态链接器执行动态链接时，都需要通过 `PT_DYNAMIC` segment 来找到 .dynamic section 的内存位置，再进一步读取其他各项 section 的信息。

libtest.so 的 .dynamic section：

```
caikelun@debian:~$ arm-linux-androideabi-readelf -d ./libtest.so 

Dynamic section at offset 0x2e48 contains 30 entries:
  Tag        Type                         Name/Value
 0x00000003 (PLTGOT)                     0x3f7c
 0x00000002 (PLTRELSZ)                   240 (bytes)
 0x00000017 (JMPREL)                     0xcb8
 0x00000014 (PLTREL)                     REL
 0x00000011 (REL)                        0xc78
 0x00000012 (RELSZ)                      64 (bytes)
 0x00000013 (RELENT)                     8 (bytes)
 0x6ffffffa (RELCOUNT)                   3
 0x00000006 (SYMTAB)                     0x1f0
 0x0000000b (SYMENT)                     16 (bytes)
 0x00000005 (STRTAB)                     0x590
 0x0000000a (STRSZ)                      1201 (bytes)
 0x00000004 (HASH)                       0xa44
 0x00000001 (NEEDED)                     Shared library: [libc.so]
 0x00000001 (NEEDED)                     Shared library: [libm.so]
 0x00000001 (NEEDED)                     Shared library: [libstdc++.so]
 0x00000001 (NEEDED)                     Shared library: [libdl.so]
 0x0000000e (SONAME)                     Library soname: [libtest.so]
 0x0000001a (FINI_ARRAY)                 0x3e3c
 0x0000001c (FINI_ARRAYSZ)               8 (bytes)
 0x00000019 (INIT_ARRAY)                 0x3e44
 0x0000001b (INIT_ARRAYSZ)               4 (bytes)
 0x0000001e (FLAGS)                      BIND_NOW
 0x6ffffffb (FLAGS_1)                    Flags: NOW
 0x6ffffff0 (VERSYM)                     0xbc8
 0x6ffffffc (VERDEF)                     0xc3c
 0x6ffffffd (VERDEFNUM)                  1
 0x6ffffffe (VERNEED)                    0xc58
 0x6fffffff (VERNEEDNUM)                 1
 0x00000000 (NULL)                       0x0
```


## 动态链接器（linker）

安卓中的动态链接器程序是 linker。源码在 [这里](https://android.googlesource.com/platform/bionic/+/master/linker/)。

动态链接（比如执行 dlopen）的大致步骤是：

1. 检查已加载的 ELF 列表。（如果 libtest.so 已经加载，就不再重复加载了，仅把 libtest.so 的引用计数加一，然后直接返回。）
2. 从 libtest.so 的 .dynamic section 中读取 libtest.so 的外部依赖的 ELF 列表，从此列表中剔除已加载的 ELF，最后得到本次需要加载的 ELF 完整列表（包括 libtest.so 自身）。
3. 逐个加载列表中的 ELF。加载步骤：
    * 用 `mmap` 预留一块足够大的内存，用于后续映射 ELF。（`MAP_PRIVATE` 方式）
    * 读 ELF 的 PHT，用 `mmap` 把所有类型为 `PT_LOAD` 的 segment 依次映射到内存中。
    * 从 .dynamic segment 中读取各信息项，主要是各个 section 的虚拟内存相对地址，然后计算并保存各个 section 的虚拟内存绝对地址。
    * 执行重定位操作（relocate），这是最关键的一步。重定位信息可能存在于下面的一个或多个 secion 中：`.rel.plt`, `.rela.plt`, `.rel.dyn`, `.rela.dyn`, `.rel.android`, `.rela.android`。动态链接器需要逐个处理这些 `.relxxx` section 中的重定位诉求。根据已加载的 ELF 的信息，动态链接器查找所需符号的地址（比如 libtest.so 的符号 `malloc`），找到后，将地址值填入 `.relxxx` 中指明的**目标地址**中，这些“**目标地址**”一般存在于`.got` 或 `.data` 中。
    * ELF 的引用计数加一。
4. 逐个调用列表中 ELF 的构造函数（constructor），这些构造函数的地址是之前从 .dynamic segment 中读取到的（类型为 `DT_INIT` 和 `DT_INIT_ARRAY`）。各 ELF 的构造函数是按照依赖关系逐层调用的，先调用被依赖 ELF 的构造函数，最后调用 libtest.so 自己的构造函数。（ELF 也可以定义自己的析构函数（destructor），在 ELF 被 unload 的时候会被自动调用）

等一下！我们似乎发现了什么！再看一遍重定位操作（relocate）的部分。难道我们只要从这些 `.relxxx` 中获取到“**目标地址**”，然后在“**目标地址**”中重新填上一个新的函数地址，这样就完成 hook 了吗？也许吧。


## 追踪

静态分析验证一下还是很容易的。以 armeabi-v7a 架构的 libtest.so 为例。先看一下 say_hello 函数对应的汇编代码吧。

```
caikelun@debian:~/$ arm-linux-androideabi-readelf -s ./libtest.so

Symbol table '.dynsym' contains 58 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 00000000     0 FUNC    GLOBAL DEFAULT  UND __cxa_finalize@LIBC (2)
     2: 00000000     0 FUNC    GLOBAL DEFAULT  UND snprintf@LIBC (2)
     3: 00000000     0 FUNC    GLOBAL DEFAULT  UND malloc@LIBC (2)
     4: 00000000     0 FUNC    GLOBAL DEFAULT  UND __cxa_atexit@LIBC (2)
     5: 00000000     0 FUNC    GLOBAL DEFAULT  UND printf@LIBC (2)
     6: 00000f61    60 FUNC    GLOBAL DEFAULT   12 say_hello
...............
...............
```

找到了！`say_hello` 在地址 `f61`，对应的汇编指令体积为 `60`（10 进制）字节。用 objdump 查看 `say_hello` 的反汇编输出。

```
caikelun@debian:~$ arm-linux-androideabi-objdump -D ./libtest.so
...............
...............
00000f60 <say_hello@@Base>:
     f60:   b5b0        push    {r4, r5, r7, lr}
     f62:   af02        add r7, sp, #8
     f64:   f44f 6080   mov.w   r0, #1024   ; 0x400
     f68:   f7ff ef34   blx dd4 <malloc@plt>
     f6c:   4604        mov r4, r0
     f6e:   b16c        cbz r4, f8c <say_hello@@Base+0x2c>
     f70:   a507        add r5, pc, #28 ; (adr r5, f90 <say_hello@@Base+0x30>)
     f72:   a308        add r3, pc, #32 ; (adr r3, f94 <say_hello@@Base+0x34>)
     f74:   4620        mov r0, r4
     f76:   f44f 6180   mov.w   r1, #1024   ; 0x400
     f7a:   462a        mov r2, r5
     f7c:   f7ff ef30   blx de0 <snprintf@plt>
     f80:   4628        mov r0, r5
     f82:   4621        mov r1, r4
     f84:   e8bd 40b0   ldmia.w sp!, {r4, r5, r7, lr}
     f88:   f001 ba96   b.w 24b8 <_Unwind_GetTextRelBase@@Base+0x8>
     f8c:   bdb0        pop {r4, r5, r7, pc}
     f8e:   bf00        nop
     f90:   7325        strb    r5, [r4, #12]
     f92:   0000        movs    r0, r0
     f94:   6568        str r0, [r5, #84]   ; 0x54
     f96:   6c6c        ldr r4, [r5, #68]   ; 0x44
     f98:   0a6f        lsrs    r7, r5, #9
     f9a:   0000        movs    r0, r0
...............
...............
```

对 `malloc` 函数的调用对应于指令 `blx dd4`。跳转到了地址 `dd4`。看看这个地址里有什么吧：

```
caikelun@debian:~$ arm-linux-androideabi-objdump -D ./libtest.so
...............
...............
00000dd4 <malloc@plt>:
 dd4:   e28fc600    add ip, pc, #0, 12
 dd8:   e28cca03    add ip, ip, #12288  ; 0x3000
 ddc:   e5bcf1b4    ldr pc, [ip, #436]! ; 0x1b4
...............
...............
```

果然，跳转到了 `.plt` 中，经过了几次地址计算，最后跳转到了地址 `3f90` 中的值指向的地址处，`3f90` 是个函数指针。

稍微解释一下：因为 arm 处理器使用 3 级流水线，所以第一条指令取到的 `pc` 的值是当前执行的指令地址 + `8`。
于是：`dd4` + `8` + `3000` + `1b4` = `3f90`。

地址 `3f90` 在哪里呢：

```
caikelun@debian:~$ arm-linux-androideabi-objdump -D ./libtest.so
...............
...............
00003f60 <.got>:
    ...
    3f70:   00002604    andeq   r2, r0, r4, lsl #12
    3f74:   00002504    andeq   r2, r0, r4, lsl #10
    ...
    3f88:   00000da8    andeq   r0, r0, r8, lsr #27
    3f8c:   00000da8    andeq   r0, r0, r8, lsr #27
    3f90:   00000da8    andeq   r0, r0, r8, lsr #27
...............
...............
```

果然，在 `.got` 里。

顺便再看一下 `.rel.plt`：

```
caikelun@debian:~$ arm-linux-androideabi-readelf -r ./libtest.so

Relocation section '.rel.plt' at offset 0xcb8 contains 30 entries:
 Offset     Info    Type            Sym.Value  Sym. Name
00003f88  00000416 R_ARM_JUMP_SLOT   00000000   __cxa_atexit@LIBC
00003f8c  00000116 R_ARM_JUMP_SLOT   00000000   __cxa_finalize@LIBC
00003f90  00000316 R_ARM_JUMP_SLOT   00000000   malloc@LIBC
...............
...............
```

`malloc` 的地址居然正好存放在 `3f90` 里，这绝对不是巧合啊！还等什么，赶紧改代码吧。我们的 main.c 应该改成这样：

```c
#include <test.h>

void *my_malloc(size_t size)
{
    printf("%zu bytes memory are allocated by libtest.so\n", size);
    return malloc(size);
}

int main()
{
    void **p = (void **)0x3f90;
    *p = (void *)my_malloc; // do hook
    
    say_hello();
    return 0;
}
```

编译运行一下：

```
caikelun@debian:~$ adb push ./main /data/local/tmp
caikelun@debian:~$ adb shell "chmod +x /data/local/tmp/main"
caikelun@debian:~$ adb shell "export LD_LIBRARY_PATH=/data/local/tmp; /data/local/tmp/main"
Segmentation fault
caikelun@debian:~$
```

思路是正确的。但之所以还是失败了，是因为这段代码存在下面的 3 个问题：

1. `3f90` 是个相对内存地址，需要把它换算成绝对地址。
2. `3f90` 对应的绝对地址很可能没有写入权限，直接对这个地址赋值会引起段错误。
3. 新的函数地址即使赋值成功了，`my_malloc` 也不会被执行，因为处理器有指令缓存（instruction cache）。

我们需要解决这些问题。


## 内存


### 基地址

在进程的内存空间中，各种 ELF 的加载地址是随机的，只有在运行时才能拿到加载地址，也就是**基地址**。我们需要知道 ELF 的基地址，才能将相对地址换算成绝对地址。

没有错，熟悉 Linux 开发的聪明的你一定知道，我们可以直接调用 `dl_iterate_phdr`。详细的定义见 [这里](http://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html)。

嗯，先等等，多年的 Android 开发被坑经历告诉我们，还是再看一眼 NDK 里的 `linker.h` 头文件吧：

```c
#if defined(__arm__)

#if __ANDROID_API__ >= 21
int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info*, size_t, void*), void* __data) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

#else
int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info*, size_t, void*), void* __data);
#endif
```

为什么？！ARM 架构的 Android 5.0 以下版本居然不支持 `dl_iterate_phdr`！我们的 APP 可是要支持 Android 4.0 以上的所有版本啊。特别是 ARM，怎么能不支持呢？！这还让不让人写代码啦！

幸运的是，我们想到了，我们还可以解析 `/proc/self/maps`:

```
root@android:/ # ps | grep main
ps | grep main
shell     7884  7882  2616   1016  hrtimer_na b6e83824 S /data/local/tmp/main

root@android:/ # cat /proc/7884/maps
cat /proc/7884/maps

address           perms offset  dev   inode       pathname
---------------------------------------------------------------------
...........
...........
b6e42000-b6eb5000 r-xp 00000000 b3:17 57457      /system/lib/libc.so
b6eb5000-b6eb9000 r--p 00072000 b3:17 57457      /system/lib/libc.so
b6eb9000-b6ebc000 rw-p 00076000 b3:17 57457      /system/lib/libc.so
b6ec6000-b6ec9000 r-xp 00000000 b3:19 753708     /data/local/tmp/libtest.so
b6ec9000-b6eca000 r--p 00002000 b3:19 753708     /data/local/tmp/libtest.so
b6eca000-b6ecb000 rw-p 00003000 b3:19 753708     /data/local/tmp/libtest.so
b6f03000-b6f20000 r-xp 00000000 b3:17 32860      /system/bin/linker
b6f20000-b6f21000 r--p 0001c000 b3:17 32860      /system/bin/linker
b6f21000-b6f23000 rw-p 0001d000 b3:17 32860      /system/bin/linker
b6f25000-b6f26000 r-xp 00000000 b3:19 753707     /data/local/tmp/main
b6f26000-b6f27000 r--p 00000000 b3:19 753707     /data/local/tmp/main
becd5000-becf6000 rw-p 00000000 00:00 0          [stack]
ffff0000-ffff1000 r-xp 00000000 00:00 0          [vectors]
...........
...........
```

maps 返回的是指定进程的内存空间中 `mmap` 的映射信息，包括各种动态库、可执行文件（如：linker），栈空间，堆空间，甚至还包括字体文件。maps 格式的详细说明见 [这里](http://man7.org/linux/man-pages/man5/proc.5.html)。

我们的 libtest.so 在 maps 中有 3 行记录。offset 为 `0` 的第一行的起始地址 `b6ec6000` 在**绝大多数情况下**就是我们寻找的**基地址**。


### 内存访问权限

maps 返回的信息中已经包含了权限访问信息。如果要执行 hook，就需要写入的权限，可以使用 `mprotect` 来完成：

```c
#include <sys/mman.h>

int mprotect(void *addr, size_t len, int prot);
```

注意修改内存访问权限时，只能以“页”为单位。`mprotect` 的详细说明见 [这里](http://man7.org/linux/man-pages/man2/mprotect.2.html)。


### 指令缓存

注意 `.got` 和 `.data` 的 section 类型是 `PROGBITS`，也就是执行代码。处理器可能会对这部分数据做缓存。修改内存地址后，我们需要清除处理器的指令缓存，让处理器重新从内存中读取这部分指令。方法是调用 `__builtin___clear_cache`：

```c
void __builtin___clear_cache (char *begin, char *end);
```

注意清除指令缓存时，也只能以“页”为单位。`__builtin___clear_cache` 的详细说明见 [这里](https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html)。


## 验证


### 修改 main.c

我们把 `main.c` 修改为：

```c
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <test.h>

#define PAGE_START(addr) ((addr) & PAGE_MASK)
#define PAGE_END(addr)   (PAGE_START(addr) + PAGE_SIZE)

void *my_malloc(size_t size)
{
    printf("%zu bytes memory are allocated by libtest.so\n", size);
    return malloc(size);
}

void hook()
{
    char       line[512];
    FILE      *fp;
    uintptr_t  base_addr = 0;
    uintptr_t  addr;

    //find base address of libtest.so
    if(NULL == (fp = fopen("/proc/self/maps", "r"))) return;
    while(fgets(line, sizeof(line), fp))
    {
        if(NULL != strstr(line, "libtest.so") &&
           sscanf(line, "%"PRIxPTR"-%*lx %*4s 00000000", &base_addr) == 1)
            break;
    }
    fclose(fp);
    if(0 == base_addr) return;

    //the absolute address
    addr = base_addr + 0x3f90;
    
    //add write permission
    mprotect((void *)PAGE_START(addr), PAGE_SIZE, PROT_READ | PROT_WRITE);

    //replace the function address
    *(void **)addr = my_malloc;

    //clear instruction cache
    __builtin___clear_cache((void *)PAGE_START(addr), (void *)PAGE_END(addr));
}

int main()
{
    hook();
    
    say_hello();
    return 0;
}
```

重新编译运行：

```
caikelun@debian:~$ adb push ./main /data/local/tmp
caikelun@debian:~$ adb shell "chmod +x /data/local/tmp/main"
caikelun@debian:~$ adb shell "export LD_LIBRARY_PATH=/data/local/tmp; /data/local/tmp/main"
1024 bytes memory are allocated by libtest.so
hello
caikelun@debian:~$
```

是的，成功了！我们并没有修改 libtest.so 的代码，甚至没有重新编译它。我们仅仅修改了 main 程序。

libtest.so 和 main 的源码放在 github 上，可以从 [这里](https://github.com/iqiyi/xhook/tree/master/docs/overview/code) 获取到。（根据你使用的编译器不同，或者编译器的版本不同，生成的 libtest.so 中，也许 `malloc` 对应的地址不再是 `0x3f90`，这时你需要先用 readelf 确认，然后再到 `main.c` 中修改。）


### 使用 xhook

当然，我们已经开源了一个叫 xhook 的工具库。使用 xhook，你可以更优雅的完成对 libtest.so 的 hook 操作，也不必担心硬编码 `0x3f90` 导致的兼容性问题。

```c
#include <stdlib.h>
#include <stdio.h>
#include <test.h>
#include <xhook.h>

void *my_malloc(size_t size)
{
    printf("%zu bytes memory are allocated by libtest.so\n", size);
    return malloc(size);
}

int main()
{
    xhook_register(".*/libtest\\.so$", "malloc", my_malloc, NULL);
    xhook_refresh(0);
    
    say_hello();
    return 0;
}
```

xhook 支持 armeabi, armeabi-v7a 和 arm64-v8a。支持 Android 4.0 (含) 以上版本 (API level >= 14)。经过了产品级的稳定性和兼容性验证。可以在 [这里](https://github.com/iqiyi/xhook) 获取 `xhook`。

总结一下 xhook 中执行 PLT hook 的流程：

1. 读 maps，获取 ELF 的内存首地址（start address）。
2. 验证 ELF 头信息。
3. 从 PHT 中找到类型为 `PT_LOAD` 且 offset 为 `0` 的 segment。计算 ELF 基地址。
4. 从 PHT 中找到类型为 `PT_DYNAMIC` 的 segment，从中获取到 `.dynamic` section，从 `.dynamic` section中获取其他各项 section 对应的内存地址。
5. 在 `.dynstr` section 中找到需要 hook 的 symbol 对应的 index 值。
6. 遍历所有的 `.relxxx` section（重定位 section），查找 symbol index 和 symbol type 都匹配的项，对于这项重定位项，执行 hook 操作。hook 流程如下：
    * 读 maps，确认当前 hook 地址的内存访问权限。
    * 如果权限不是可读也可写，则用 `mprotect` 修改访问权限为可读也可写。
    * 如果调用方需要，就保留 hook 地址当前的值，用于返回。
    * 将 hook 地址的值替换为新的值。（执行 hook）
    * 如果之前用 `mprotect` 修改过内存访问权限，现在还原到之前的权限。
    * 清除 hook 地址所在内存页的处理器指令缓存。


## FAQ


### 可以直接从文件中读取 ELF 信息吗？

可以。而且对于格式解析来说，读文件是最稳妥的方式，因为 ELF 在运行时，原理上有很多 section 不需要一直保留在内存中，可以在加载完之后就从内存中丢弃，这样可以节省少量的内存。但是从实践的角度出发，各种平台的动态链接器和加载器，都不会这么做，可能它们认为增加的复杂度得不偿失。所以我们从内存中读取各种 ELF 信息就可以了，读文件反而增加了性能损耗。另外，某些系统库 ELF 文件，APP 也不一定有访问权限。


### 计算基地址的精确方法是什么？

正如你已经注意到的，前面介绍 libtest.so 基地址获取时，为了简化概念和编码方便，用了“**绝大多数情况下**”这种不应该出现的描述方式。对于 hook 来说，精确的基地址计算流程是：

1. 在 maps 中找到找到 offset 为 `0`，且 `pathname` 为目标 ELF 的行。保存该行的 start address 为 `p0`。
2. 找出 ELF 的 PHT 中第一个类型为 `PT_LOAD` 且 offset 为 `0` 的 segment，保存该 segment 的虚拟内存相对地址（`p_vaddr`）为 `p1`。
3. `p0` - `p1` 即为该 ELF 当前的基地址。

绝大多数的 ELF 第一个 `PT_LOAD` segment 的 `p_vaddr` 都是 `0`。

另外，之所以要在 maps 里找 offset 为 `0` 的行，是因为我们在执行 hook 之前，希望对内存中的 ELF 文件头进行校验，确保当前操作的是一个有效的 ELF，而这种 ELF 文件头只能出现在 offset 为 `0` 的 mmap 区域。

可以在 Android linker 的源码中搜索“load_bias”，可以找到很多详细的注释说明，也可以参考 linker 中对 `load_bias_` 变量的赋值程序逻辑。


### 目标 ELF 使用的编译选项对 hook 有什么影响？

会有一些影响。

对于外部函数的调用，可以分为 3 中情况：

1. 直接调用。无论编译选项如何，都可以被 hook 到。外部函数地址始终保存在 `.got` 中。
2. 通过全局函数指针调用。无论编译选项如何，都可以被 hook 到。外部函数地址始终保存在 `.data` 中。
3. 通过局部函数指针调用。如果编译选项为 -O2（默认值），调用将被优化为直接调用（同情况 1）。如果编译选项为 -O0，则在执行 hook 前已经被赋值到临时变量中的外部函数的指针，通过 PLT 方式无法 hook；对于执行 hook 之后才被赋值的，可以通过 PLT 方式 hook。

一般情况下，产品级的 ELF 很少会使用 -O0 进行编译，所以也不必太纠结。但是如果你希望你的 ELF 尽量不被别人 PLT hook，那可以试试使用 -O0 来编译，然后尽量早的将外部函数的指针赋值给局部函数指针变量，之后一直使用这些局部函数指针来访问外部函数。

总之，查看 C/C++ 的源代码对这个问题的理解没有意义，需要查看使用不同的编译选项后，生成的 ELF 的反汇编输出，比较它们的区别，才能知道哪些情况由于什么原因导致无法被 PLT hook。


### hook 时遇到偶发的段错误是什么原因？如何处理？

我们有时会遇到这样的问题：

* 读取 `/proc/self/maps` 后发现某个内存区域的访问权限为**可读**，当我们读取该区域的内容做 ELF 文件头校验时，发生了段错误（sig: SIGSEGV, code: SEGV_ACCERR）。
* 已经用 `mprotect()` 修改了某个内存区域的访问权限为**可写**，`mprotect()` 返回修改成功，然后再次读取 `/proc/self/maps` 确认对应内存区域的访问权限确实为**可写**，执行写入操作（替换函数指针，执行 hook）时发生段错误（sig: SIGSEGV, code: SEGV_ACCERR）。
* 读取和验证 ELF 文件头成功了，根据 ELF 头中的相对地址值，进一步读取 PHT 或者 `.dynamic` section 时发生段错误（sig: SIGSEGV, code: SEGV_ACCERR 或 SEGV_MAPERR）。

可能的原因是：

* 进程的内存空间是多线程共享的，我们在执行 hook 时，其他线程（甚至 linker）可能正在执行 `dlclose()`，或者正在用 `mprotect()` 修改这块内存区域的访问权限。
* 不同厂家、机型、版本的 Android ROM 可能有未公开的行为，比如在某些情况下对某些内存区域存在**写保护**或者**读保护**机制，而这些保护机制并不反应在 `/proc/self/maps` 的内容中。

问题分析：

* 读内存时发生段错误其实是无害的。
* 我在 hook 执行的流程中，需要直接通过计算内存地址的方式来写入数据的地方只有一处：即替换函数指针的最关键的那一行。只要其他地方的逻辑没有错误，这里就算写入失败了，也不会对其他内存区域造成破坏。
* 加载运行安卓平台的 APP 进程时，加载器已经向我们注入了 signal handler 的注册逻辑，以便 APP 崩溃时与系统的 `debuggerd` 守护进程通讯，`debuggerd` 使用 `ptrace` 调试崩溃进程，获取需要的崩溃现场信息，记录到 tombstone 文件中，然后 APP 自杀。
* 系统会精确的把段错误信号发送给“发生段错误的线程”。
* 我们希望能有一种隐秘的，且可控的方式来避免段错误引起 APP 崩溃。

先明确一个观点：不要只从应用层程序开发的角度来看待段错误，段错误不是洪水猛兽，它只是内核与用户进程的一种正常的交流方式。当用户进程访问了无权限或未 mmap 的虚拟内存地址时，内核向用户进程发送 SIGSEGV 信号，来通知用户进程，仅此而已。只要段错误的发生位置是可控的，我们就可以在用户进程中处理它。

解决方案：

* 当 hook 逻辑进入我们认为的危险区域（直接计算内存地址进行读写）之前，通过一个全局 `flag` 来进行标记，离开危险区域后将 `flag` 复位。
* 注册我们自己的 signal handler，只捕获段错误。在 signal handler 中，通过判断 `flag` 的值，来判断当前线程逻辑是否在危险区域中。如果是，就用 `siglongjmp` 跳出 signal handler，直接跳到我们预先设置好的“危险区域以外的下一行代码处”；如果不是，就恢复之前加载器向我们注入的 signal handler，然后直接返回，这时系统会再次向我们的线程发送段错误信号，由于已经恢复了之前的 signal handler，这时会进入默认的系统 signal handler 中走正常逻辑。
* 我们把这种机制简称为：SFP (segmentation fault protection，段错误保护)
* 注意：SFP需要一个开关，让我们随时能够开启和关闭它。在 APP 开发调试阶段，SFP 应该始终被关闭，这样就不会错过由于编码失误导致的段错误，这些错误是应该被修复的；在正式上线后 SFP 应该被开启，这样能保证 APP 不会崩溃。（当然，以采样的形式部分关闭 SFP，用以观察和分析 hook 机制本身导致的崩溃，也是可以考虑的）

具体代码可以参考 `xhook` 中的实现，在源码中搜索 `siglongjmp` 和 `sigsetjmp`。


### ELF 内部函数之间的调用能 hook 吗？


我们这里介绍的 hook 方式为 PLT hook，不能做 ELF 内部函数之间调用的 hook。

inline hook 可以做到，你需要先知道想要 hook 的内部函数符号名（symbol name）或者地址，然后可以 hook。

有很多开源和非开源的 inline hook 实现，比如：

* substrate：http://www.cydiasubstrate.com/
* frida：https://www.frida.re/

inline hook 方案强大的同时可能带来以下的问题：

* 由于需要直接解析和修改 ELF 中的机器指令（汇编码），对于不同架构的处理器、处理器指令集、编译器优化选项、操作系统版本可能存在不同的兼容性和稳定性问题。
* 发生问题后可能难以分析和定位，一些知名的 inline hook 方案是闭源的。
* 实现起来相对复杂，难度也较大。
* 未知的坑相对较多，这个可以自行 google。

建议如果 PLT hook 够用的话，就不必尝试 inline hook 了。


## 联系作者


* caikelun@gmail.com
* https://github.com/caikelun


## 许可证


Copyright (c) 2018, 爱奇艺, Inc. All rights reserved.

本文使用 [Creative Commons 许可证](https://creativecommons.org/licenses/by/4.0/legalcode) 授权。
