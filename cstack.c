#include "cstack.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


enum {
    STACK_CHUNK_SIZE = 10
};

// data structure
struct stack_node {
    struct stack_node* prev;
    unsigned int size;
    char data[0];
};

typedef struct stack_node stack_node_t;

struct stack {
    unsigned int size;
    stack_node_t* top_ptr;
};

typedef struct stack stack_t;

struct stack_holder {
    struct stack_holder* prev;
    struct stack_holder* next;
    unsigned int stack_count;
    stack_t* stacks[STACK_CHUNK_SIZE];
};

typedef struct stack_holder stack_holder_t;

struct stack_info {
    stack_holder_t* stack_holder_ptr;
    stack_t* stack_ptr;
};

typedef struct stack_info stack_info_t;

//  variables
stack_holder_t stack_holder = {};

// inner functions
stack_info_t get_stack_info(const hstack_t hstack) {
    stack_info_t result = {NULL, NULL};
    if (hstack < 0) {
        return result;
    }
    unsigned int chunk_id = hstack / STACK_CHUNK_SIZE;
    unsigned int stack_id = hstack % STACK_CHUNK_SIZE;
    stack_holder_t* sh_ptr = &stack_holder;

    while (chunk_id != 0) {
        if (!sh_ptr->next) {
            return result;
        }
        sh_ptr = sh_ptr->next;
        --chunk_id;
    }
    result.stack_holder_ptr = sh_ptr;
    result.stack_ptr = sh_ptr->stacks[stack_id];
    return result;
}

stack_t* get_stack_ptr(const hstack_t hstack) {
    stack_info_t si = get_stack_info(hstack);
    return si.stack_ptr;
}

// user interface
hstack_t stack_new(void) {
    stack_t* stack = calloc(1, sizeof(stack_t));
    if (!stack) {
        return -1;
    }

    stack_holder_t* sh_ptr = &stack_holder;
    unsigned int chunk_id = 0;
    unsigned int stack_id = 0;

    // find chunk_id or create new
    while (sh_ptr->stack_count == STACK_CHUNK_SIZE) {
        ++chunk_id;
        if (!sh_ptr->next) {
            sh_ptr->next = calloc(1, sizeof(stack_holder));
            if (!sh_ptr->next) {
                return -1;
            }
            sh_ptr->next->prev = sh_ptr;
            sh_ptr = sh_ptr->next;
            break;
        }
        sh_ptr = sh_ptr->next;
    }
    // find stack_id
    for (; stack_id < STACK_CHUNK_SIZE; ++stack_id) {
        if (!sh_ptr->stacks[stack_id]) {
            sh_ptr->stacks[stack_id] = stack;
            ++sh_ptr->stack_count;
            break;
        }
    }
    return chunk_id * STACK_CHUNK_SIZE + stack_id;
}

void stack_free(const hstack_t hstack) {
    stack_info_t si = get_stack_info(hstack);
    stack_t* stack_ptr = si.stack_ptr;

    if (!stack_ptr) {
        return;
    }

    while (stack_ptr->top_ptr) {
        stack_node_t* prev = stack_ptr->top_ptr->prev;
        free(stack_ptr->top_ptr);
        stack_ptr->top_ptr = prev;
    }
    free(stack_ptr);
    si.stack_holder_ptr->stacks[hstack % STACK_CHUNK_SIZE] = NULL;
    --si.stack_holder_ptr->stack_count;

    // free stack_holder if it is empty and is the last
    while (si.stack_holder_ptr->stack_count == 0 &&
           si.stack_holder_ptr != &stack_holder &&
           !si.stack_holder_ptr->next) {
        stack_holder_t* sh_prev = si.stack_holder_ptr->prev;
        sh_prev->next = NULL;
        free(si.stack_holder_ptr);
        si.stack_holder_ptr = sh_prev;
    }
}

int stack_valid_handler(const hstack_t hstack) {
    stack_t* stack_ptr = get_stack_ptr(hstack);
    return (stack_ptr) ? 0 : 1;
}

unsigned int stack_size(const hstack_t hstack) {
    stack_t* stack_ptr = get_stack_ptr(hstack);
    if (!stack_ptr) {
        return 0;
    }
    return stack_ptr->size;
}

void stack_push(const hstack_t hstack, const void* data_in, const unsigned int size) {
    if (size == 0 || !data_in) {
        return;
    }

    stack_t* stack_ptr = get_stack_ptr(hstack);
    if (!stack_ptr) {
        return;
    }

    stack_node_t* node_ptr = calloc(1, sizeof(stack_node_t) + size);
    if (!node_ptr) {
        return;
    }
    memcpy(node_ptr->data, data_in, size);
    node_ptr->size = size;
    if (stack_ptr->top_ptr) {
        node_ptr->prev = stack_ptr->top_ptr;
    }
    stack_ptr->top_ptr = node_ptr;
    ++stack_ptr->size;
}

unsigned int stack_pop(const hstack_t hstack, void* data_out, const unsigned int size) {
    if (size == 0 || !data_out) {
        return 0;
    }

    stack_t* stack_ptr = get_stack_ptr(hstack);
    if (!stack_ptr || stack_ptr->size == 0) {
        return 0;
    }

    stack_node_t* top_node_ptr = stack_ptr->top_ptr;
    unsigned int cpy_size = top_node_ptr->size;
    if (cpy_size == 0 || cpy_size > size) {
        return 0;
    }
    memcpy(data_out, top_node_ptr->data, cpy_size);
    stack_ptr->top_ptr = top_node_ptr->prev;
    free(top_node_ptr);
    if (stack_ptr->size != 0) {
        --stack_ptr->size;
    }
    return cpy_size;
}