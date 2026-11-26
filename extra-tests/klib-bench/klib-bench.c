#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <am.h>
#include <klib.h>
#include <klib-macros.h>

// 测试宏定义，可以根据需要调整
#define SHORT_SIZE 12
#define MIDDLE_SIZE 4096 // 4KB
#define LARGE_SIZE (4 * 1024 * 1024) // 4MB

// 合理分配迭代次数以平衡测试时间
#define SHORT_ITERATIONS 100000
#define MIDDLE_ITERATIONS 1000
#define LARGE_ITERATIONS 1
#define SPRINTF_ITERATIONS 500

// 计时结构体，用于存储开始和结束时间
uint64_t start, end;

// 防止编译器优化掉被测函数调用
#define BLACKBOX(x) ({                          \
    typeof(x) _x = x;                           \
    asm volatile("" : "+r"(_x) :: "memory");    \
    _x;                                         \
})

static inline int uptime_ms() {
  return io_read(AM_TIMER_UPTIME).us;
}

// 测量函数，用于计算并打印测试结果
void measure_performance(const char *test_name, void (*func)(), int iterations) {
    start = uptime_ms(); // 获取开始时间
    func();                                 // 执行测试函数
    end = uptime_ms();   // 获取结束时间

    // 计算总耗时（纳秒）
    uint64_t elapsed_us = end - start;
    int elapsed_ms = elapsed_us / 1000;

    printf("--- %s Test ---\n", test_name);
    printf("Iterations: %d\n", iterations);
    printf("Total Time: %d ms\n", elapsed_ms);
    printf("-------------------------\n\n");
}

// ======================= 测试函数 =======================

char src_buf[LARGE_SIZE + 1024];
char dst_buf[LARGE_SIZE + 1024];

// ======================= 内存内容验证函数 =======================
// 验证内存块的前100个字节、中间100个字节和最后100个字节是否正确
void check_memory_contents(const char *src, const char *dest, size_t size, const char *test_name) {
    // 检查开头部分
    for (size_t i = 0; i < 100; ++i) {
        if (src[i] != dest[i]) {
            printf("%s check FAILED! (start) src[%zu]=%d, dest[%zu]=%d\n", test_name, i, src[i], i, dest[i]);
            halt(1);
        }
    }
    // 检查中间部分
    size_t mid_start = size / 2 - 50;
    for (size_t i = mid_start; i < mid_start + 100; ++i) {
        if (src[i] != dest[i]) {
            printf("%s check FAILED! (middle) src[%zu]=%d, dest[%zu]=%d\n", test_name, i, src[i], i, dest[i]);
            halt(1);
        }
    }
    // 检查结尾部分
    size_t end_start = size - 100;
    for (size_t i = end_start; i < size; ++i) {
        if (src[i] != dest[i]) {
            printf("%s check FAILED! (end) src[%zu]=%d, dest[%zu]=%d\n", test_name, i, src[i], i, dest[i]);
            halt(1);
        }
    }
}

// ======================= 正确性验证函数 =======================
void run_sanity_checks() {
    printf("Running basic sanity checks...\n");

    // memset Sanity Check
    char m_buf[100];
    memset(m_buf, 0x55, 100);
    for (int i = 0; i < 100; i++) {
        if (m_buf[i] != 0x55) {
            printf("memset sanity check FAILED!\n");
            halt(1);
        }
    }

    // memcpy Sanity Check
    char s_buf[100] = "abcdefg";
    char d_buf[100];
    memcpy(d_buf, s_buf, 8);
    if (strcmp(s_buf, d_buf) != 0) {
        printf("memcpy sanity check FAILED!\n");
        halt(1);
    }

    // strcmp Sanity Check
    if (strcmp("abc", "abc") != 0 || strcmp("abc", "abd") >= 0 || strcmp("abd", "abc") <= 0) {
        printf("strcmp sanity check FAILED!\n");
        halt(1);
    }

    // memmove Sanity Check for overlapping memory
    char mem_block[100];
    strcpy(mem_block, "abcdefghijklmnopqrstuvwxyz");

    // // Case 1: Overlap where dest > src (forward copy)
    memmove(mem_block + 2, mem_block, 5);
    if (mem_block[2] != 'a' || mem_block[6] != 'e') {
        printf("memmove sanity check (dest > src) FAILED!\n");
        halt(1);
    }
    strcpy(mem_block, "abcdefghijklmnopqrstuvwxyz"); // Reset for next test

    // Case 2: Overlap where dest < src (backward copy)
    memmove(mem_block, mem_block + 2, 5);
    if (mem_block[0] != 'c' || mem_block[4] != 'g') {
        printf("memmove sanity check (dest < src) FAILED!\n");
        halt(1);
    }
    strcpy(mem_block, "abcdefghijklmnopqrstuvwxyz"); // Reset for next test
    printf("All basic sanity checks passed.\n\n");
}

// ======================= 大规模测试验证 =======================
void run_large_scale_sanity_checks() {
    printf("Running large-scale sanity checks...\n");

    // 分配大内存
    char *src = src_buf;
    char *dest = dst_buf;

    // 填充源数据，使用不同的值
    for (size_t i = 0; i < LARGE_SIZE + 1024; ++i) {
        src[i] = (char)(i % 256);
    }

    // --- 大规模 memcpy 正确性测试 ---
    printf("Testing memcpy with %dMB data...\n", LARGE_SIZE / 1024 / 1024);
    memcpy(dest, src, LARGE_SIZE);
    check_memory_contents(src, dest, LARGE_SIZE, "memcpy (large)");
    printf("memcpy (large) passed.\n\n");

    // --- 大规模 memmove (无重叠) 正确性测试 ---
    // 确保 src 和 dest 不重叠
    if (src < dest && src + LARGE_SIZE > dest) {
        printf("memmove test skipped due to overlapping allocation.\n");
    } else {
        printf("Testing memmove (non-overlapping) with %dMB data...\n", LARGE_SIZE / 1024 / 1024);
        memmove(dest, src, LARGE_SIZE);
        check_memory_contents(src, dest, LARGE_SIZE, "memmove (non-overlapping, large)");
        printf("memmove (non-overlapping, large) passed.\n\n");
    }

    // --- 大规模 memmove (重叠) 正确性测试 ---
    // 测试 dest > src 的情况
    printf("Testing memmove (overlapping, dest > src) with %dMB data...\n", LARGE_SIZE / 1024 / 1024);
    char *overlap_mem = dst_buf; // 额外分配空间
    for (size_t i = 0; i < LARGE_SIZE + 1024; ++i) {
        overlap_mem[i] = (char)(i % 256);
    }
    char *ovlp_src = overlap_mem;
    char *ovlp_dest = overlap_mem + 10;
    memmove(ovlp_dest, ovlp_src, LARGE_SIZE);
    check_memory_contents(src_buf, ovlp_dest, LARGE_SIZE, "memmove (overlapping, dest > src, large)");
    printf("memmove (overlapping, dest > src, large) passed.\n\n");

    // 测试 dest < src 的情况
    printf("Testing memmove (overlapping, dest < src) with %dMB data...\n", LARGE_SIZE / 1024 / 1024);
    overlap_mem = dst_buf;
    for (size_t i = 0; i < LARGE_SIZE + 1024; ++i) {
        overlap_mem[i] = (char)(i % 256);
    }
    ovlp_src = overlap_mem + 10;
    ovlp_dest = overlap_mem;
    memmove(ovlp_dest, ovlp_src, LARGE_SIZE);
    check_memory_contents(src_buf + 10, ovlp_dest, LARGE_SIZE, "memmove (overlapping, dest < src, large)");
    printf("memmove (overlapping, dest < src, large) passed.\n\n");
}

// ------------------- 性能测试函数 -------------------
// memset 测试，使用静态分配的缓冲区
void test_memset_short() {
    for (long i = 0; i < BLACKBOX(SHORT_ITERATIONS); ++i) {
        BLACKBOX(memset(src_buf, 0, SHORT_SIZE));
    }
}

void test_memset_middle() {
    for (long i = 0; i < BLACKBOX(MIDDLE_ITERATIONS); ++i) {
        BLACKBOX(memset(src_buf, 0, MIDDLE_SIZE));
    }
}

void test_memset_large() {
    for (long i = 0; i < BLACKBOX(LARGE_ITERATIONS); ++i) {
        BLACKBOX(memset(src_buf, 0, LARGE_SIZE));
    }
}

// memcpy 测试，使用静态分配的缓冲区
void test_memcpy_aligned_middle() {
    for (long i = 0; i < BLACKBOX(MIDDLE_ITERATIONS); ++i) {
        BLACKBOX(memcpy(src_buf + MIDDLE_SIZE, src_buf, MIDDLE_SIZE));
    }
}

void test_memcpy_unaligned_middle() {
    for (long i = 0; i < BLACKBOX(MIDDLE_ITERATIONS); ++i) {
        BLACKBOX(memcpy(src_buf + MIDDLE_SIZE + 1, src_buf + 5, MIDDLE_SIZE));
    }
}

// memmove 测试，使用静态分配的缓冲区，包括非重叠和重叠情况
void test_memmove_aligned_middle() {
    for (long i = 0; i < BLACKBOX(MIDDLE_ITERATIONS); ++i) {
        // 非重叠
        BLACKBOX(memmove(src_buf + MIDDLE_SIZE, src_buf, MIDDLE_SIZE));
    }
}

void test_memmove_overlapping_middle() {
    for (long i = 0; i < BLACKBOX(MIDDLE_ITERATIONS); ++i) {
        // 重叠
        BLACKBOX(memmove(src_buf + 16, src_buf, MIDDLE_SIZE));
    }
}

// strcmp 测试，针对不同长度字符串
void test_strcmp_short() {
    for (size_t i = 0; i < BLACKBOX(MIDDLE_SIZE); ++i) {
        src_buf[i] = (char)(i % 16) + 'a';
        dst_buf[i] = (char)(i % 16) + 'a';
    }
    src_buf[SHORT_SIZE - 1] = '1';
    dst_buf[MIDDLE_SIZE - 1] = '\0';
    const char *s1 = src_buf;
    const char *s2 = dst_buf;
    for (long i = 0; i < BLACKBOX(SHORT_ITERATIONS); ++i) {
        BLACKBOX(strcmp(s1, s2));
    }
}

void test_strcmp_long() {
    for (size_t i = 0; i < BLACKBOX(MIDDLE_SIZE); ++i) {
        src_buf[i] = (char)(i % 16) + 'a';
        dst_buf[i] = (char)(i % 16) + 'a';
    }
    src_buf[MIDDLE_SIZE - 1] = '\0';
    dst_buf[MIDDLE_SIZE - 1] = '\0';
    const char *s1 = src_buf;
    const char *s2 = dst_buf;
    for (long i = 0; i < BLACKBOX(MIDDLE_ITERATIONS); ++i) {
        BLACKBOX(strcmp(s1, s2));
    }
}

// ------------------- 主函数 -------------------
int main() {
    printf("Starting kernel lib benchmark with advanced workloads...\n\n");

    // 第一步：正确性验证
    run_sanity_checks();
    run_large_scale_sanity_checks();

    // 第二步：性能测试
    // memset tests
    measure_performance("memset (short)", test_memset_short, SHORT_ITERATIONS);
    measure_performance("memset (middle)", test_memset_middle, MIDDLE_ITERATIONS);
    measure_performance("memset (large)", test_memset_large, LARGE_ITERATIONS);

    // memcpy tests
    measure_performance("memcpy (aligned, middle)", test_memcpy_aligned_middle, MIDDLE_ITERATIONS);
    measure_performance("memcpy (unaligned, middle)", test_memcpy_unaligned_middle, MIDDLE_ITERATIONS);

    // memmove tests
    measure_performance("memmove (non-overlapping, middle)", test_memmove_aligned_middle, MIDDLE_ITERATIONS);
    measure_performance("memmove (overlapping, middle)", test_memmove_overlapping_middle, MIDDLE_ITERATIONS);

    // strcmp tests
    measure_performance("strcmp (short)", test_strcmp_short, SHORT_ITERATIONS);
    measure_performance("strcmp (long)", test_strcmp_long, MIDDLE_ITERATIONS);

    printf("Benchmark finished.\n");
    return 0;
}
