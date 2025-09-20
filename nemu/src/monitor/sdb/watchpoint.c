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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char *expr;
  word_t val;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].expr = NULL;
    wp_pool[i].val = 0;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
static WP *new_wp(bool *success) {
  if (free_ == NULL) {
    *success = false;
    return NULL;
  }
  WP *ret = free_;
  free_ = free_->next;
  ret->next = head;
  head = ret;
  return ret;
}

static bool free_wp(WP *wp) {
  assert(wp);

  if (head == NULL) {
    return false;
  }

  if (wp == head) {
    head = head->next;
  } else {
    WP *prev = head;
    while (prev != NULL && prev->next != wp) {
      prev = prev->next;
    }
    if (prev == NULL) {
      return false;
    }
    prev->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
  
  if (wp->expr) {
    free(wp->expr);
    wp->expr = NULL;
  }

  return true;
}

bool add_wp(char *expr) {
  bool success = true;
  word_t val = eval_expr(expr, &success);
  if (!success) {
    printf("Invalid expression\n");
    return false;
  }

  success = true;
  WP *wp = new_wp(&success);
  if (!success) {
    return false;
  }

  int expr_len = strlen(expr);
  wp->expr = malloc((expr_len + 1) * sizeof(*expr));
  memcpy(wp->expr, expr, expr_len * sizeof(*expr));
  wp->expr[expr_len] = '\0';
  wp->val = val;
  return true;
}

bool delete_wp(int no) {
  if (no < 0 || no >= NR_WP) {
    printf("No such watchpoint\n");
    return false;
  }
  return free_wp(&wp_pool[no]);
}

void print(WP *head) {
  if (head == NULL) {
    return;
  }
  print(head->next);
  printf("%-5d%s\n", head->NO, head->expr);
}

void print_wp() {
  if (head == NULL) {
    printf("No watchpoints now\n");
    return;
  }
  printf("num  expression\n");
  print(head);
}