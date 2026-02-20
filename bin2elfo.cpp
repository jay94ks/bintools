#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <elf.h>

#define print_err(...) fprintf(stderr, __VA_ARGS__)

/* entry point. */
int main(int argc, const char** argv) {
    if (argc < 3) {
        print_err(
            "usage: %s <input> <output> [symbol]\n"
            "e.g. symbol: test -> test_b, test_e. \n", argv[0]);

        return 1;
    }

    const char* input = argv[1];
    const char* output = argv[2];

    FILE* fp_i = fopen(input, "rb");
    FILE* fp_o = fopen(output, "wb");

    if (!fp_i) {
        print_err(
            "%s: error: failed to open the input file: %s.\n",
            argv[0], input
        );

        return 1;
    }

    if (!fp_o) {
        print_err(
            "%s: error: failed to create the output file: %s.\n",
            argv[0], output
        );

        return 1;
    }

    fseek(fp_i, 0, SEEK_END);
    uint32_t total_len = ftell(fp_i);
    uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t) * total_len);

    if (!data) {
        print_err(
            "%s: error: failed to malloc(%d).\n",
            argv[0], total_len
        );

        return 1;
    }

    uint32_t loaded = 0;
    fseek(fp_i, 0, SEEK_SET);
    while (loaded < total_len) {
        uint32_t slice = (total_len - loaded) > 4096 ? 4096 : total_len - loaded;
        uint32_t read = fread(data + loaded, 1, slice, fp_i);

        if (read <= 0) {
            break;
        }

        loaded += read;
    }

    // --
    fclose(fp_i);
    if (loaded != total_len) {
        print_err(
            "%s: error: failed to read data from the input file: expects %d bytes but %d read.\n",
            argv[0], total_len, loaded
        );

        return 1;
    }

    // --
    char symbol_name[256];
    memset(symbol_name, 0, sizeof(symbol_name));

    if (argc > 3) {
        strncpy(symbol_name, argv[3], sizeof(symbol_name) - 3);
    } else {
        const char* slash = strrchr(output, '/');

        if (slash) {
            strncpy(symbol_name, slash + 1, sizeof(symbol_name) - 3);
        } else {
            strncpy(symbol_name, output, sizeof(symbol_name) - 3);
        }
    }

    // --> replace special characters (including spaces) to underbar.
    int32_t len = strlen(symbol_name);
    for (int32_t i = 0; i < len; ++i) {
        const char ch = symbol_name[i];
        if (!isalnum(ch)) {
            symbol_name[i] = '_';
        }
    }

    // --> add marking characters.
    symbol_name[len++] = '_';
    symbol_name[len++] = 'b';

    // --> string table length.
    size_t strtab_len = (len + 1) * 2 + 1;
    char* strtab = (char*) malloc(strtab_len);

    /**
     * strtab = 
     *      \0
     *      symbol_b\0
     *      symbol_e\0
     */
    memset(strtab, 0, strtab_len);
    memcpy(strtab + 1, symbol_name, len);

    symbol_name[len - 1] = 'e';
    memcpy(strtab + 1 + len + 1, symbol_name, len);

    // --> section name table.
    const char shstrtab[] =
        "\0"
        ".data\0"
        ".symtab\0"
        ".strtab\0"
        ".shstrtab\0"
        ;

    /* symbol table. */
    Elf64_Sym symtab[3];
    memset(symtab, 0, sizeof(symtab));

    /* section symbol. */
    symtab[0].st_name = 0;  // --> .data.
    symtab[0].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
    symtab[0].st_shndx = 1;
    symtab[0].st_value = 0;
    symtab[0].st_size = 0;
    
    /* start of data, `_b`. */
    symtab[1].st_name = 1;  // --> symbol_b.
    symtab[1].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
    symtab[1].st_shndx = 1;
    symtab[1].st_value = 0;
    symtab[1].st_size = 0;
    
    /* end of data, `_e`. */
    symtab[2].st_name = len + 2;  // --> symbol_e.
    symtab[2].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
    symtab[2].st_shndx = 1;
    symtab[2].st_value = total_len;
    symtab[2].st_size = 0;

    size_t off = sizeof(Elf64_Ehdr);
    size_t data_off = off;
    off += total_len;

    // --> round up to alignment: 8.
    if (off & (8 - 1)) {
        off += 8 - (off & (8 - 1));
    }

    size_t symtab_off = off;
    size_t strtab_off = (off += sizeof(symtab));
    size_t shstrtab_off = (off += strtab_len);
    off += sizeof(shstrtab);

    // --> round up to alignment: 8.
    if (off & (8 - 1)) {
        off += 8 - (off & (8 - 1));
    }

    size_t shoff = off;

    // --
    Elf64_Ehdr eh;
    memset(&eh, 0, sizeof(eh));
    memcpy(eh.e_ident, ELFMAG, SELFMAG);

    eh.e_ident[EI_CLASS] = ELFCLASS64;
    eh.e_ident[EI_DATA] = ELFDATA2LSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_ident[EI_OSABI] = ELFOSABI_SYSV;

    eh.e_type = ET_REL;
    eh.e_machine = EM_X86_64;
    eh.e_version = EV_CURRENT;
    eh.e_shoff = shoff;
    eh.e_ehsize = sizeof(Elf64_Ehdr);
    eh.e_shentsize = sizeof(Elf64_Shdr);
    eh.e_shnum = 5;
    eh.e_shstrndx = 4;

    fwrite(&eh, 1, sizeof(eh), fp_o);
    fwrite(data, 1, total_len, fp_o);

    // --> 8 byte alignment.
    while (ftell(fp_o) & (8 - 1)) {
        fputc(0, fp_o);
    }

    fwrite(symtab, 1, sizeof(symtab), fp_o);
    fwrite(strtab, 1, strtab_len, fp_o);
    fwrite(shstrtab, 1, sizeof(shstrtab), fp_o);

    // --> 8 byte alignment.
    while (ftell(fp_o) & (8 - 1)) {
        fputc(0, fp_o);
    }

    Elf64_Shdr sh[5];
    memset(sh, 0, sizeof(sh));

    /* [1] .data */
    sh[1].sh_name   = 1;
    sh[1].sh_type   = SHT_PROGBITS;
    sh[1].sh_flags  = SHF_ALLOC | SHF_WRITE;
    sh[1].sh_offset = data_off;
    sh[1].sh_size   = total_len;
    sh[1].sh_addralign = 1;

    /* [2] .symtab */
    sh[2].sh_name   = 7;
    sh[2].sh_type   = SHT_SYMTAB;
    sh[2].sh_offset = symtab_off;
    sh[2].sh_size   = sizeof(symtab);
    sh[2].sh_link   = 3;
    sh[2].sh_info   = 1;
    sh[2].sh_addralign = 8;
    sh[2].sh_entsize = sizeof(Elf64_Sym);

    /* [3] .strtab */
    sh[3].sh_name   = 15;
    sh[3].sh_type   = SHT_STRTAB;
    sh[3].sh_offset = strtab_off;
    sh[3].sh_size   = strtab_len;
    sh[3].sh_addralign = 1;

    /* [4] .shstrtab */
    sh[4].sh_name   = 23;
    sh[4].sh_type   = SHT_STRTAB;
    sh[4].sh_offset = shstrtab_off;
    sh[4].sh_size   = sizeof(shstrtab);
    sh[4].sh_addralign = 1;

    fwrite(sh, 1, sizeof(sh), fp_o);
    fclose(fp_o);

    return 0;
}