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
#include "sdb.h"
#include "memory/paddr.h"
#include "memory/vaddr.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NE, TK_AND,

  /* TODO: Add more token types */
  TK_DECIMAL, TK_HEX, TK_REG,
  TK_MINUS, TK_DEREF,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},           // subtract
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"\\(", '('},         // left paranthesis
  {"\\)", ')'},         // right paranthesis
  {"0x[0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_DECIMAL},
  {"\\$[a-z0-9\\$]+", TK_REG},
  {"==", TK_EQ},        // equal
  {"!=", TK_NE},
  {"&&", TK_AND},
};


#define IS_UNARY_OP(type) \
  ((type) == TK_MINUS || (type) == TK_DEREF)

#define IS_BINARY_OP(type) \
  ((type) == '+'      || (type) == '-'      ||\
   (type) == '*'      || (type) == '/'      ||\
   (type) == TK_EQ    || (type) == TK_NE    ||\
   (type) == TK_AND)

#define IS_OP(type) (IS_UNARY_OP(type) || IS_BINARY_OP(type))

static int precedence_level(int token_type) {
  switch (token_type) {
    case TK_AND:
      return 0;
    case TK_EQ:     case TK_NE:
      return 1;
    case '+':       case '-':
      return 2;
    case '*':       case '/':
      return 3;
    case TK_DEREF:  case TK_MINUS:
      return 4;
    default:
      return INT32_MAX;
  }
}

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

#define DEFAULT_TOKENS_SIZE 32

static int tokens_size = DEFAULT_TOKENS_SIZE;

#define NR_BUFFER 32

static Token *tokens __attribute__((used)) = NULL;
static int nr_token __attribute__((used))  = 0;

#define NR_TOKENS tokens_size

static void free_tokens() {
  if (tokens == NULL) {
    return;
  }
  free(tokens);
  tokens = NULL;
}

static void init_tokens() {
  nr_token = 0;

  if (tokens != NULL && tokens_size != DEFAULT_TOKENS_SIZE) {
    free_tokens();
  }
  if (tokens == NULL) {
    tokens_size = DEFAULT_TOKENS_SIZE;
    tokens = malloc(tokens_size * sizeof(*tokens));
  }
}

static void expand_tokens() {
  Token *new_arr = malloc(2 * NR_TOKENS * sizeof(*new_arr));
  memcpy(new_arr, tokens, NR_TOKENS * sizeof(*tokens));
  free(tokens);
  tokens = new_arr;
  tokens_size *= 2;
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if (rules[i].token_type != TK_NOTYPE && nr_token >= NR_TOKENS) {
          expand_tokens();
        }

        switch (rules[i].token_type) {
          case '+':   case '-':   case '*':
          case '/':   case '(':   case ')':
          case TK_EQ: case TK_NE: case TK_AND:
            tokens[nr_token++].type = rules[i].token_type;
            break;
          case TK_DECIMAL:  case TK_HEX:  case TK_REG:
            tokens[nr_token].type = rules[i].token_type;
            if (substr_len < NR_BUFFER) {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0';
              nr_token++;
            } else {
              TODO();
            }
            break;
          default: 
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parantheses(int left, int right) {
  if (tokens[left].type != '(' || tokens[right].type != ')') {
    return false;
  }

  int num_left_paran = 0;
  for (int i = left + 1; i < right; i++) {
    if (tokens[i].type == '(') {
      num_left_paran++;
    } else if (tokens[i].type == ')') {
      if (num_left_paran <= 0) {
        return false;
      } 
      num_left_paran--;
    } 
  }
  return num_left_paran == 0;
}

static int find_main_op(int left, int right) {
    int op = left;
    if (IS_BINARY_OP(tokens[op].type)) {
      return -1;
    }
    int num_left_paran = 0;
    for (int i = left; i <= right; i++) {
      switch (tokens[i].type) {
        case '(':
          num_left_paran++;
          break;
        case ')':
          if (num_left_paran <= 0) {
            return -1;
          }
          num_left_paran--;
          break;
        case '+': case '-': case '*': case '/':
        case TK_EQ: case TK_NE: case TK_AND:
          if (num_left_paran > 0) {
            continue;
          }
          op = precedence_level(tokens[i].type) <= precedence_level(tokens[op].type) ? 
                  i : op;
          break;
        case TK_DEREF:  case TK_MINUS:
          if (num_left_paran > 0) {
            continue;
          }
          op = precedence_level(tokens[i].type) < precedence_level(tokens[op].type) ? 
                  i : op;
          break;
        default:
          continue;
      }
    } 
    if (!IS_OP(tokens[op].type)) {
      return -1;
    }
    return op;
}

static word_t parse_number(int idx, bool *success) {
    char *endptr = tokens[idx].str;
    word_t number;
    switch (tokens[idx].type) {
      case TK_DECIMAL:
        number = strtoul(tokens[idx].str, &endptr, 10);
        break;
      case TK_HEX:
        number = strtoul(tokens[idx].str, &endptr, 16);
        break;
      default:
        assert(0);
    }
    if (*endptr != '\0') {
      printf("Invalid number: '%s'\n", tokens[idx].str);
      *success = false;
      return 0;
    }
    return number;
}

static word_t calc_2(word_t val1, word_t val2, int op_type, bool *success) {
    switch (op_type) {
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/':
        if (val2 == 0) {
          printf("Can not divided by 0\n");
          *success = false;
          return 0;
        }
        return val1 / val2;
      case TK_EQ:
        return val1 == val2;
      case TK_NE:
        return val1 != val2;
      case TK_AND:
        return val1 && val2;
      default:
        assert(0);
    }
}

static word_t calc_1(word_t val, int op_type, bool *success __attribute__((unused))) {
  switch (op_type) {
    case TK_DEREF:
      if (!in_pmem(val)) {
        printf("Invalid address\n");
        *success = false;
        return 0;
      }
      return vaddr_read(val, sizeof(word_t));
    case TK_MINUS:
      return -val;
    default:
      assert(0);
  }
}

static word_t eval(int left, int right, bool *success) {
  if (left > right) {
    *success = false;
    return 0;
  } else if (left == right) {
    if (tokens[left].type == TK_REG) {
      return isa_reg_str2val(tokens[left].str + 1, success);
    }
    if (tokens[left].type == TK_DECIMAL ||
        tokens[left].type == TK_HEX) {
          return parse_number(left, success);
    }
    *success = false;
    return 0;
  } else if (check_parantheses(left, right)) {
    return eval(left + 1, right - 1, success);
  } else {
    int op = find_main_op(left, right);

    if (op < 0) {
      *success = false;
      return 0;
    }

    if (IS_UNARY_OP(tokens[op].type)) {
      word_t val = eval(op + 1, right, success);
      return calc_1(val, tokens[op].type, success);
    } else {
      word_t val1 = eval(left, op - 1, success);
      word_t val2 = eval(op + 1, right, success);
      return calc_2(val1, val2, tokens[op].type, success);
    }
  }
}

word_t eval_expr(char *e, bool *success) {
  init_tokens();

  if (!make_token(e)) {
    *success = false;
    free_tokens();
    return 0;
  }

  if (nr_token == 0) {
    printf("Null expression\n");
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i++) {
    if (i == 0 || 
        IS_OP(tokens[i - 1].type) || 
        tokens[i - 1].type == '(' ) {
      if (tokens[i].type == '-') {
        tokens[i].type = TK_MINUS;
      } else if (tokens[i].type == '*') {
        tokens[i].type = TK_DEREF;
      }
    }
  }

  *success = true;
  word_t ret = eval(0, nr_token - 1, success);
  free_tokens();
  return ret;
}
