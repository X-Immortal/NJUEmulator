#include <proc.h>
#include <fs.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
# define EXPECT_TYPE EM_386  // see /usr/include/elf.h to get the right type
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_MIPS32__)
# define EXPECT_TYPE EM_MIPS
#else
# error Unsupported ISA
#endif

#define ALIGNMENT 16
#define NR_PGS_USTACK 8

size_t ramdisk_read(void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  Elf_Ehdr elf_header;
  size_t ret = fs_read(fd, &elf_header, sizeof(Elf_Ehdr));
  assert(ret == sizeof(Elf_Ehdr));
  assert(memcmp(elf_header.e_ident, ELFMAG, sizeof(ELFMAG) - 1) == 0);
  assert(elf_header.e_type == ET_EXEC);
  assert(elf_header.e_machine == EXPECT_TYPE);

  Elf_Phdr program_header;
  size_t f_off;
  void *page = new_page(1);
  uintptr_t vaddr = 0;
  size_t p_off = PGSIZE;
  for (int i = 0; i < elf_header.e_phnum; i++) {
    f_off = elf_header.e_phoff + i * elf_header.e_phentsize;
    ret = fs_lseek(fd, f_off, SEEK_SET);
    assert(ret == f_off);
    ret = fs_read(fd, &program_header, elf_header.e_phentsize);
    assert(ret == elf_header.e_phentsize);
    if (program_header.p_type == PT_LOAD) {
      f_off = program_header.p_offset;
      p_off = program_header.p_vaddr - vaddr;
      vaddr = program_header.p_vaddr - PGSIZE;
      size_t len = 0;
      while (len < program_header.p_filesz) {
        if (p_off >= PGSIZE) {
          page = new_page(1);
          p_off = 0;
          vaddr += PGSIZE;
          map(&pcb->as, (void *) vaddr, page, 0);
        }
        assert(fs_lseek(fd, f_off + len, SEEK_SET) == f_off + len);
        size_t nr_read = program_header.p_filesz - len;
        nr_read = (nr_read < PGSIZE - p_off) ? nr_read : (PGSIZE - p_off);
        assert(fs_read(fd, (void *) (page + p_off), nr_read) == nr_read);
        len += nr_read;
        p_off += nr_read;
      }
      
      len = ROUNDUP(len, PGSIZE);
      while (len < program_header.p_memsz) {
        page = new_page(1);
        memset(page, 0, PGSIZE);
        len += PGSIZE;
      }
    }
  }
  assert(fs_close(fd) == 0);
  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  pcb->cp = kcontext((Area) { pcb->stack, &pcb->stack + 1 }, entry, arg);
}

void context_uload(PCB *pcb, const char *filename, char * const argv[], char * const envp[]) {
  static char *default_envp[] = { NULL };
  assert(pcb && filename && argv);
  if (!envp) {
    envp = default_envp;
  }

  protect(&pcb->as);

  size_t str_len = 0;
  size_t argc = 0;
  for (size_t i = 0; argv[i]; ++i) {
    ++argc;
  }
  size_t argv_slen[argc];
  for (size_t i = 0; argv[i]; ++i) {
    str_len += argv_slen[i] = strlen(argv[i]) + 1;
  }

  size_t envc = 0;
  for (size_t i = 0; envp[i]; ++i) {
    ++envc;
  }
  size_t env_slen[envc];
  for (size_t i = 0; envp[i]; ++i) {
    str_len += env_slen[i] = strlen(envp[i]) + 1;
  }

  uintptr_t stack = (uintptr_t) new_page(NR_PGS_USTACK);
  for (int i = 0; i < NR_PGS_USTACK; ++i) {
    map(&pcb->as, 
      (void *) (pcb->as.area.end - (NR_PGS_USTACK - i) * PGSIZE), 
      (void *) (stack + i * PGSIZE),
      0);
  }

  uintptr_t end = stack + 8 * PGSIZE;
  uintptr_t str_area_p = end - ROUNDUP(str_len, ALIGNMENT);
  uintptr_t start = str_area_p - ROUNDUP((argc + envc + 2) * sizeof(char *) + sizeof(int), ALIGNMENT);
  int *argc_p = (int *) start;
  char **argv_p = (char **) (argc_p + 1);
  char **envp_p = argv_p + argc + 1;

  *argc_p = argc;

  char *str_p = (char *) str_area_p;
  for (size_t i = 0; i < argc; ++i) {
    strcpy(str_p, argv[i]);
    argv_p[i] = str_p;
    str_p += argv_slen[i];
  }
  argv_p[argc] = NULL;

  for (size_t i = 0; i < envc; ++i) {
    strcpy(str_p, envp[i]);
    envp_p[i] = str_p;
    str_p += env_slen[i];
  }
  envp_p[envc] = NULL;

  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  pcb->cp = ucontext(&pcb->as, (Area) { pcb->stack, &pcb->stack + 1 }, (void *) entry);

  pcb->cp->GPRx = start;
}
