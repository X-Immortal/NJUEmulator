#include <common.h>
#include <elf.h>

#ifndef CONFIG_TARGET_AM
FILE *elf_fp = NULL;

#define FUNC_MAP_SIZE 4096
#define FUNC_NAME_MAX_LEN 64

static Elf32_Ehdr elf_header;
static size_t symtab_offset = 0;
static size_t syment_num;
static size_t strtab_offset = 0;
static size_t strtab_size;

static struct {
  Elf32_Addr addr;
  size_t size;
  char name[FUNC_NAME_MAX_LEN];
} func_map[FUNC_MAP_SIZE];
int func_num = 0;

static void read_elf_header() {
  rewind(elf_fp);
  int ret = fread(&elf_header, sizeof(elf_header), 1, elf_fp);
  assert(ret == 1);
  if (memcmp(elf_header.e_ident, ELFMAG, sizeof(ELFMAG)) != 0) {
    panic("Not a valid ELF file!");
  }
}

static void read_section_header_table() {
  fseek(elf_fp, elf_header.e_shoff, SEEK_SET);
  fseek(elf_fp, elf_header.e_shstrndx * elf_header.e_shentsize, SEEK_CUR);
  Elf32_Shdr shdr;
  int ret = fread(&shdr, sizeof(shdr), 1, elf_fp);
  assert(ret == 1);

  char *shstrtab = (char *) malloc(shdr.sh_size);
  fseek(elf_fp, shdr.sh_offset, SEEK_SET);
  ret = fread(shstrtab, shdr.sh_size, 1, elf_fp);
  assert(ret == 1);

  fseek(elf_fp, elf_header.e_shoff, SEEK_SET);
  for (size_t i = 0; i < elf_header.e_shnum; i++) {
    ret = fread(&shdr, sizeof(shdr), 1, elf_fp);
    assert(ret == 1);
    if (strcmp(shstrtab + shdr.sh_name, ".symtab") == 0) {
      symtab_offset = shdr.sh_offset;
      syment_num = shdr.sh_size / shdr.sh_entsize;
    } else if (strcmp(shstrtab + shdr.sh_name, ".strtab") == 0) {
      strtab_offset = shdr.sh_offset;
      strtab_size = shdr.sh_size;
    } else if (symtab_offset && strtab_offset) {
      break;
    }
  }

  free(shstrtab);
}

static void read_symbol_table() {
  if (symtab_offset == 0 || strtab_offset == 0) {
    return;
  }

  fseek(elf_fp, strtab_offset, SEEK_SET);
  char *strtab = (char *) malloc(strtab_size);
  int ret = fread(strtab, strtab_size, 1, elf_fp);
  assert(ret == 1);

  fseek(elf_fp, symtab_offset, SEEK_SET);
  for (size_t i = 0; i < syment_num; i++) {
    Elf32_Sym sym;
    ret = fread(&sym, sizeof(sym), 1, elf_fp);
    assert(ret == 1);
    if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC && sym.st_size > 0) {
      if (func_num >= FUNC_MAP_SIZE) {
        panic("Function map size exceeded!");
      }
      func_map[func_num].addr = sym.st_value;
      func_map[func_num].size = sym.st_size;
      strncpy(func_map[func_num].name, strtab + sym.st_name, FUNC_NAME_MAX_LEN - 1);
      func_map[func_num].name[FUNC_NAME_MAX_LEN - 1] = '\0';
      func_num++;
    }
  }

  free(strtab);
}

static void read_elf() {
  read_elf_header();
  read_section_header_table();
  read_symbol_table();
}

void init_ftrace(const char *elf_file) {
  if (elf_file != NULL) {
    FILE *fp = fopen(elf_file, "rb");
    Assert(fp, "Can not open '%s'", elf_file);
    elf_fp = fp;
    read_elf();
    fclose(elf_fp);
  }
}

const char* get_func_name(uint32_t addr) {
  for (int i = 0; i < func_num; i++) {
    if (func_map[i].addr >= addr && 
          addr < func_map[i].addr + func_map[i].size) {
      return func_map[i].name;
    }
  }
  return NULL;
}
#endif
