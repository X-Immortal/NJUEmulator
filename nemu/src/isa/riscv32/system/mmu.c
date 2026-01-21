/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <../local-include/reg.h>

#define MODE (satp >> 31)
#define VPN1(va) ((va) >> 22)
#define VPN2(va) (((va) >> 12) & 0x000003FF)
#define PG_OFFSET(addr) ((addr) & 0x00000FFF)
#define PPN(pa) ((pa) >> 12)
#define EXTRACT_PTE(pte) ((pte) >> 10)
#define PDIR (satp & 0x000FFFFF)
#define MERGE_ADDR(pno, pgoff) (((pno) << 12) | (pgoff))
#define PTE_LEN 4

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  if (MODE) {
    return MMU_TRANSLATE;
  }
  return MMU_DIRECT;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  paddr_t pgt = EXTRACT_PTE(paddr_read(MERGE_ADDR(PDIR, VPN1(vaddr)), PTE_LEN));
  paddr_t pno = EXTRACT_PTE(paddr_read(MERGE_ADDR(pgt, VPN2(vaddr)), PTE_LEN));
  return MERGE_ADDR(pno, PG_OFFSET(vaddr));
}
