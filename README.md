## Bintools.
My own binary tools. currently, this supports only `x86_64`.

### bin2elfo
generate `ELF64` relocatable binary from any binary files.
this tool generate two symbol that has postfix: `_b` and `_e`.
the purpose of this is to simplify process. using `objdump` way has same feature, but it is some complex.
and it requires many pipe between commands and many procedure to do, rather than this.

1. `_b` postfix means: begining of data.
2. `_e` postfix means: ending of data.

```
usage: ./bin2elfo <input.bin> <output.o> [symbol-name]
```
if no symbol name is specified, it'll be generated based on `output` name.
1. abc-234.o ---> abc_234_o_b, abc_234_o_e.
2. __mydata.o ---> __mydata_o_b, __mydata_o_e.


example output's `readelf` result:
```log
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          744 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           64 (bytes)
  Number of section headers:         5
  Section header string table index: 4
  
....

Symbol table '.symtab' contains 3 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .data
     1: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT    1 test_o_b
     2: 0000000000000226     0 NOTYPE  GLOBAL DEFAULT    1 test_o_e

No version information found in this file.

test.o:     file format elf64-x86-64

SYMBOL TABLE:
0000000000000000 g       .data	0000000000000000 test_o_b
0000000000000226 g       .data	0000000000000000 test_o_e
```

how to use:
```cpp
extern "C" {
    const void* test_o_b;
    const void* test_o_e;
}

int main() {
    size_t len = uintptr_t(&test_o_e) - uintptr_t(&test_o_b)
    const uint8_t* data = (uint8_t*)(&test_o_e);

    // TODO: do something...
}
```

linking:
```shell
# g++ or gcc
g++ cppcode.cpp test.o -o a.out

# ld
ld myobj.o test.o -o b.out
```

### nelfo
generate `ELF64` relocatable binary from `NASM` assembly code. 
this is just an wrapper to simplify multiple procedures to single command.

```shell
$ nasm -f bin <input.asm> -o a.bin
$ ./bin2elfo a.bin <output.o> __my_bin
```

to

```shell
$ ./nelfo <input.asm> <output.o> __my_bin
```

this is all.

I'll implement assembler itself if I need, but not now. :)

Let's be satisfied with this for now.