#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#define CAUSE_BREAKPOINT 3
#define CAUSE_ECALL_FROM_M_MODE 11

int arr[10];

Context *handler(Event ev, Context* c) {
    printf("Exception handler called, cause=%d\n", c->mcause);

    switch (c->mcause) {
        case CAUSE_BREAKPOINT:
            printf("Breakpoint, epc=%x, instr=%x, tdata2=%x(%d)\n", c->mepc, *(volatile unsigned int *)c->mepc, c->tdata2, *(volatile int *)c->tdata2);
            break;
        case CAUSE_ECALL_FROM_M_MODE:
            printf("Environment call from M-mode, epc=%x, instr=%x\n", c->mepc, *(volatile unsigned int *)c->mepc);
            c->mepc += 4;
            break;
        default:
            panic("Unknown trap cause, check your PA2 implementation\n");
            break;
    }
    return c;
}

void explain_mcontrol6(long mcontrol6) {
    printf("type = %d\n", (int)((mcontrol6 >> (32-4)) & 0xf));
    printf("dmode = %d\n", (int)((mcontrol6 >> (32-5)) & 0x1));
    printf("uncertain = %d\n", (int)((mcontrol6 >> 26) & 0x1));
    printf("vs = %d\n", (int)((mcontrol6 >> 24) & 0x1));
    printf("vu = %d\n", (int)((mcontrol6 >> 23) & 0x1));
    printf("hit = %d\n", (int)(((mcontrol6 >> 24) & 0x2) | ((mcontrol6 >> 22) & 0x1)));
    printf("select = %d\n", (int)((mcontrol6 >> 21) & 0x1));
    printf("size = %d\n", (int)((mcontrol6 >> 16) & 0x7));
    printf("action = %d\n", (int)((mcontrol6 >> 12) & 0xf));
    printf("chain = %d\n", (int)((mcontrol6 >> 11) & 0x1));
    printf("match = %d\n", (int)((mcontrol6 >> 7) & 0xf));
    printf("m = %d\n", (int)((mcontrol6 >> 6) & 0x1));
    printf("uncertainen = %d\n", (int)((mcontrol6 >> 5) & 0x1));
    printf("s = %d\n", (int)((mcontrol6 >> 4) & 0x1));
    printf("u = %d\n", (int)((mcontrol6 >> 3) & 0x1));
    printf("execute = %d\n", (int)((mcontrol6 >> 2) & 0x1));
    printf("store = %d\n", (int)((mcontrol6 >> 1) & 0x1));
    printf("load = %d\n", (int)(mcontrol6 & 0x1));
}

int main() {
    cte_init(handler);

    int tmp = 0;
    printf("Testing environment call\n");
    asm volatile("ecall\nli %0, 10" : "=r"(tmp));
    if (tmp == 10) {
        printf("Environment call test passed\n");
    } else {
        panic("Environment call test failed, checks your mepc implementation\n");
    }

    //                 33222222222211111111110000000000
    //                 10987654321098765432109876543210
    long mcontrol6 = 0b01100000000000000000000001000010L;
    explain_mcontrol6(mcontrol6);
    printf("Setting up trigger on &arr[5]=%x\n", &arr[5]);
    asm volatile(
        "csrw mcontrol, %0\n"
        "csrw tdata2, %1\n"
        :: "r"(mcontrol6), "r"(&arr[5])
    );

    printf("Enabling mstatus.mie\n");
    asm volatile(
        "csrs mstatus, %0\n"
        :: "r"(1 << 3)
    );
    printf("Trigger set, writing to arr\n");

    for (int i = 0; i < 10; i++) {
      printf("Storing arr[%d], addr = %x, value = %d\n", i, &arr[i], i + 1);
      arr[i] = i + 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("Loading arr[%d], result = %d\n", i, arr[i]);
    }
}