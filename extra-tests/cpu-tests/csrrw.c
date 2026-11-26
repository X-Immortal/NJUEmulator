#include <am.h>
#include <klib.h>

int arr[10];

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
    //                 33222222222211111111110000000000
    //                 10987654321098765432109876543210
    long mcontrol6 = 0b01100000000000001000000001000010L;
    explain_mcontrol6(mcontrol6);
    printf("Setting up trigger on &arr[5]=%x\n", &arr[5]);
    asm volatile(
        "csrw mcontrol, %0\n"
        "csrw tdata2, %1\n"
        :: "r"(mcontrol6), "r"(&arr[5])
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