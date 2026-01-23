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
#include "../local-include/reg.h"

#define IRQ_TIMER 0x80000007

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
#ifdef CONFIG_ETRACE
  if (NO == 11) {
    printf("exception %u occured at 0x%x, pc switch to 0x%x\n", NO, epc, mtvec);
  }
#endif

  mepc = epc;
  mcause = NO;
  SET_MPIE(MIE);
  SET_MIE(0);
  return mtvec;
}

word_t isa_query_intr() {
  // if (cpu.INTR && MIE) {
  //   cpu.INTR = false;
  //   return IRQ_TIMER;
  // }
  return INTR_EMPTY;
}
