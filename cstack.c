#include "cstack.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


enum {
    STACK_HOLDER_MIN_SIZE = 4
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
    unsigned int size;
    unsigned int capacity;
    unsigned int last_stack_number;
    stack_t* stacks[0];
};

typedef struct stack_holder stack_holder_t;

//  variables
stack_holder_t* stack_holder_ptr = NULL;

// inner functions
stack_t* get_stack_ptr(const hstack_t hstack) {
    if (hstack < 0 || !stack_holder_ptr || stack_holder_ptr->capacity <= (unsigned)hstack) {
        return NULL;
    }

    return stack_holder_ptr->stacks[hstack];
}

void resize_stack_holder(stack_holder_t** sh, unsigned size) {
    stack_holder_t* new_sh = calloc(1, sizeof(stack_holder_t) + size * sizeof(stack_t*));
    if (!new_sh) {
        return;
    }

    memcpy(new_sh, *sh, sizeof(stack_holder_t) + ((*sh)->capacity < size ? (*sh)->capacity : size) * sizeof(stack_t*));
    new_sh->capacity = size;

    stack_holder_t* old_sh = *sh;
    *sh = new_sh;
    free(old_sh);
}

// user interface
hstack_t stack_new(void) {
    stack_t* stack = calloc(1, sizeof(stack_t));
    if (!stack) {
        return -1;
    }

    if (stack_holder_ptr == NULL) {
        stack_holder_ptr = calloc(1, sizeof(stack_holder_t) + STACK_HOLDER_MIN_SIZE * sizeof(stack_t*));
        if (!stack_holder_ptr) {
            free(stack);
            return -1;
        }
        stack_holder_ptr->capacity = STACK_HOLDER_MIN_SIZE;
    }

    if (stack_holder_ptr->capacity == stack_holder_ptr->size) {
        stack_holder_t* prev_sh_ptr = stack_holder_ptr;
        resize_stack_holder(&stack_holder_ptr, 2 * stack_holder_ptr->capacity);
        if (prev_sh_ptr == stack_holder_ptr) {
            return -1;
        }
    }

    unsigned stack_id = stack_holder_ptr->last_stack_number;
    if (stack_id != stack_holder_ptr->capacity) {
        ++stack_holder_ptr->last_stack_number;
    } else {
        stack_id = 0;
        while (stack_id < stack_holder_ptr->capacity) {
            if (!stack_holder_ptr->stacks[stack_id]) {
                break;
            }
            ++stack_id;
        }
    }

    stack_holder_ptr->stacks[stack_id] = stack;
    ++stack_holder_ptr->size;
    return stack_id;
}

void stack_free(const hstack_t hstack) {
    stack_t* stack_ptr = get_stack_ptr(hstack);
    if (!stack_ptr) {
        return;
    }

    while (stack_ptr->top_ptr) {
        stack_node_t* prev = stack_ptr->top_ptr->prev;
        free(stack_ptr->top_ptr);
        stack_ptr->top_ptr = prev;
    }
    free(stack_ptr);
    stack_holder_ptr->stacks[hstack] = NULL;
    --stack_holder_ptr->size;
    if ((unsigned)hstack == stack_holder_ptr->last_stack_number) {
        --stack_holder_ptr->last_stack_number;
        if (STACK_HOLDER_MIN_SIZE < stack_holder_ptr->capacity &&
            stack_holder_ptr->last_stack_number < stack_holder_ptr->capacity / 3) {
            resize_stack_holder(&stack_holder_ptr, stack_holder_ptr->capacity / 2);
        }
    }
}

int stack_valid_handler(const hstack_t hstack) {
    stack_t* stack = get_stack_ptr(hstack);
    return (stack) ? 0 : 1;
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