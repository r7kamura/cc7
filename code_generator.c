#include "cc7.h"
#include <stdio.h>
#include <stdlib.h>

void generate(Node *node);

int label_counter;

void generate_code_for_load() {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

void generate_code_for_store() {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

void generate_code_for_local_variable(Node *node) {
  if (node->type != NODE_TYPE_LOCAL_VARIABLE) {
    fprintf(stderr, "Left value in assignment must be a local variable.");
    exit(1);
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->value);
  printf("  push rax\n");
}

void generate_code_for_return() {
  printf("  pop rax\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

void generate_code_for_if_only(Node *node) {
  generate(node->lhs);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  int count = label_counter++;
  printf("  je .Lend%i\n", count);
  generate(node->rhs);
  printf(".Lend%i:\n", count);
}

void generate_code_for_if_and_else(Node *node) {
  generate(node->lhs->lhs);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  int label_count = label_counter++;
  printf("  je .Lelse%i\n", label_count);
  generate(node->lhs->rhs);
  printf("  jmp .Lend%i\n", label_count);
  printf(".Lelse%i:\n", label_count);
  generate(node->rhs);
  printf(".Lend%i:\n", label_count);
}

void generate_code_for_for(Node *node) {
  if (node->lhs) {
    generate(node->lhs);
  }
  int label_count = label_counter++;
  printf(".Lbegin%i:\n", label_count);
  if (node->rhs->lhs) {
    generate(node->rhs->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%i\n", label_count);
  }
  generate(node->rhs->rhs->rhs);
  if (node->rhs->rhs->lhs) {
    generate(node->rhs->rhs->lhs);
  }
  printf("  jmp .Lbegin%i\n", label_count);
  printf(".Lend%i:\n", label_count);
}

void generate_code_for_while(Node *node) {
  int label_count = label_counter++;
  printf(".Lbegin%i:\n", label_count);
  generate(node->lhs);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je .Lend%i\n", label_count);
  generate(node->rhs);
  printf("  jmp .Lbegin%i\n", label_count);
  printf(".Lend%i:\n", label_count);
}

void generate(Node *node) {
  switch (node->type) {
  case NODE_TYPE_NUMBER:
    printf("  push %d\n", node->value);
    return;
  case NODE_TYPE_ASSIGN:
    generate_code_for_local_variable(node->lhs);
    generate(node->rhs);
    generate_code_for_load();
    return;
  case NODE_TYPE_LOCAL_VARIABLE:
    generate_code_for_local_variable(node);
    generate_code_for_store();
    return;
  case NODE_TYPE_RETURN:
    generate(node->lhs);
    generate_code_for_return();
    return;
  case NODE_TYPE_IF_ELSE:
    if (node->rhs) {
      generate_code_for_if_and_else(node);
    } else {
      generate_code_for_if_only(node->lhs);
    }
    return;
  case NODE_TYPE_IF:
    generate_code_for_if_only(node);
    return;
  case NODE_TYPE_FOR:
    generate_code_for_for(node);
    return;
  case NODE_TYPE_WHILE:
    generate_code_for_while(node);
    return;
  case NODE_TYPE_STATEMENT:
    generate(node->lhs);
    if (node->rhs) {
      printf("  pop rax\n");
      generate(node->rhs);
    }
    return;
  case NODE_TYPE_BLOCK:
    if (node->rhs) {
      generate(node->rhs);
    }
    return;
  case NODE_TYPE_FUNCTION_CALL: {
    int label_count = label_counter++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .Lcall%i\n", label_count);
    printf("  mov rax, 0\n");
    printf("  call %.*s\n", node->name_length, node->name);
    printf("  jmp .Lend%i\n", label_count);
    printf(".Lcall%i:\n", label_count);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %.*s\n", node->name_length, node->name);
    printf("  add rsp, 8\n");
    printf(".Lend%i:\n", label_count);
    printf("  push rax\n");
    return;
  }
  case NODE_TYPE_FUNCTION_DEFINITION:
    printf("%.*s:\n", node->name_length, node->name);
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");
    generate(node->lhs);
    if (node->rhs) {
      generate(node->rhs);
    }
    return;
  }

  generate(node->lhs);
  generate(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->type) {
  case NODE_TYPE_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case NODE_TYPE_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case NODE_TYPE_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case NODE_TYPE_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  case NODE_TYPE_ADD:
    printf("  add rax, rdi\n");
    break;
  case NODE_TYPE_SUBTRACT:
    printf("  sub rax, rdi\n");
    break;
  case NODE_TYPE_MULTIPLY:
    printf("  imul rax, rdi\n");
    break;
  case NODE_TYPE_DIVIDE:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
}

void generate_code(Node *node) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  generate(node);
}
