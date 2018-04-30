#include <stdio.h>
#include <stdlib.h>

#define BYTE unsigned char
#define STACK_SIZE (128)
#define MAIN_FUNCTION (0)
#define STACK_POINTER stack[frame + 1]
#define PROGRAM_COUNTER stack[frame + 2]

enum opcode { MOV, CAL, POP, RET, ADD, AND, NOT, EQU };

struct inst {
    BYTE addr_1;
    BYTE addr_type_1;
    BYTE addr_2;
    BYTE addr_type_2;
    enum opcode opcode;
};

struct fn {
    int label;
    int num_args;
    struct inst *insts;
    struct fn *next;
};

int interpret(char *filename);
int run();
int mov(struct inst *inst);
int cal(struct inst *inst);
int pop(struct inst *inst);
int ret();
int add(struct inst *ins);
int and(struct inst *ins);
int not(struct inst *inst);
int equ(struct inst *inst);
int push(BYTE value);
struct fn *add_fn(int num_inst);
struct fn *find_main_fn();
struct fn *find_fn(BYTE label);
void end();

BYTE registers[8];
BYTE stack[STACK_SIZE];
struct fn *fns = NULL;

BYTE frame = 0;
struct fn *curr_fn = NULL;

int main(int n_args, char** vargs) {
    if (n_args < 2) {
        printf("No file is provided");
        return 1;
    }
    char* filename = vargs[1];

    int res = 0;
    if ((res = interpret(filename)) != 0) { return 1; }
    if ((res = run()) != 0) { return 1; }
    end();

    return 0;
}

int interpret(char *filename) {

    BYTE buffer;

    FILE *file_ptr = fopen(filename,"rb");
    if (file_ptr == NULL) {
        printf("Cannot find the file");
        return 1;
    }
    fseek(file_ptr, 0, SEEK_END);
    size_t file_length = ftell(file_ptr);

    int num_inst = 0;
    int index_inst = 0;
    int num_bit = 8;
    int index = 0;

    // 0 - num_inst
    // 1 - opcode
    // 2 - addr_type_2
    // 3 - addr_2
    // 4 - addr_type_1
    // 5 - addr_1
    // 6 - num_args
    // 7 - fn_label
    int type = 0;

    for (int i = 0; i < file_length; i++) {

        fseek(file_ptr, file_length - 1 - i, SEEK_SET);
        fread(&buffer, 1, 1, file_ptr); // read 1 byte

        for (int index_buff = 0; index_buff < 8; index_buff++) {
            int bit = (1 << index_buff) & buffer;
            bit = (bit > 0) ? 1 : 0;

            switch (type) {
                case 0: // num_inst
                    if (i == file_length - 1) {
                        break;
                    }
                    num_inst |= (bit << index);
                    index++;
                    if (index == num_bit) {
                        type = 1; // opcode
                        num_bit = 3;
                        index = 0;
                        curr_fn = add_fn(num_inst);
                        index_inst = num_inst - 1;
                    }
                    break;
                case 1: // opcode
                    curr_fn->insts[index_inst].opcode |= (bit << index);
                    index++;
                    if (index == num_bit) {
                        index = 0;
                        switch (curr_fn->insts[index_inst].opcode) {
                            case RET:
                                type = 1;
                                num_bit = 3;
                                index_inst--;
                                break;
                            case POP:
                            case NOT:
                            case EQU:
                                type = 4;
                                num_bit = 2;
                                curr_fn->insts[index_inst].addr_type_1 = 0;
                                break;
                            case MOV:
                            case CAL:
                            case ADD:
                            case AND:
                                type = 2;
                                num_bit = 2;
                        }
                    }
                    break;
                case 2: // addr_type_2
                    curr_fn->insts[index_inst].addr_type_2 |= (bit << index);
                    index++;
                    if (index == num_bit) {
                        index = 0;
                        type = 3;
                        switch (curr_fn->insts[index_inst].addr_type_2) {
                            case 0:
                                num_bit = 8;
                                break;
                            case 1:
                                num_bit = 3;
                                break;
                            case 2:
                                num_bit = 7;
                                break;
                            case 3:
                                num_bit = 7;
                        }
                    }
                    break;
                case 3:
                    curr_fn->insts[index_inst].addr_2 |= (bit << index);
                    index++;

                    if (index == num_bit) {
                        index = 0;
                        type = 4;
                        num_bit = 2;
                    }
                    break;
                case 4: // addr_type_1
                    curr_fn->insts[index_inst].addr_type_1 |= (bit << index);
                    index++;
                    if (index == num_bit) {
                        index = 0;
                        type = 5;
                        switch (curr_fn->insts[index_inst].addr_type_1) {
                            case 0:
                                num_bit = 8;
                                break;
                            case 1:
                                num_bit = 3;
                                break;
                            case 2:
                                num_bit = 7;
                                break;
                            case 3:
                                num_bit = 7;
                        }
                        curr_fn->insts[index_inst].addr_1 = 0;
                    }
                    break;
                case 5:
                    curr_fn->insts[index_inst].addr_1 |= (bit << index);
                    index++;

                    if (index == num_bit) {
                        index = 0;
                        index_inst--;
                        if (index_inst == -1) {
                            type = 6;
                            num_bit = 4;
                            curr_fn->num_args = 0;
                        } else {
                            type = 1;
                            num_bit = 3;
                            curr_fn->insts[index_inst].opcode = 0;
                        }
                    }
                    break;
                case 6:
                    curr_fn->num_args |= (bit << index);
                    index++;
                    if (index == num_bit) {
                        index = 0;
                        type = 7;
                        num_bit = 4;
                    }
                    break;
                case 7:
                    curr_fn->label |= (bit << index);
                    index++;
                    if (index == num_bit) {
                        index = 0;
                        num_inst = 0;
                        type = 0;
                        num_bit = 8;
                    }
            }
        }
    }
    fclose(file_ptr);
    return 0;
}

int run() {

    curr_fn = find_main_fn();
    if (curr_fn == NULL) {
        printf("Cannot find main function\n");
        return 1;
    }

    frame = 0;
    stack[0] = 0; // stack frame pointer
    stack[1] = 3; // stack pointer
    stack[2] = 0; // program counter

    struct inst *inst = NULL;
    int opcode = -1;
    int res = 0;

    while (1) {

        inst = &(curr_fn->insts[PROGRAM_COUNTER]);
        opcode = inst->opcode;

        switch (opcode) {
            case MOV:
                res = mov(inst);
                break;
            case CAL:
                res = cal(inst);
                break;
            case POP:
                res = pop(inst);
                break;
            case RET:
                if (curr_fn->label == MAIN_FUNCTION) { return 0; }
                res = ret();
                break;
            case ADD:
                res = add(inst);
                break;
            case AND:
                res = and(inst);
                break;
            case NOT:
                res = not(inst);
                break;
            case EQU:
                res = equ(inst);
        }

        if (res == 1) { return 1; }

        if (opcode != CAL) { PROGRAM_COUNTER++; }
    }
    return 0;
}

int mov(struct inst *inst) {
    BYTE addr_1 = inst->addr_1;
    BYTE addr_2 = inst->addr_2;
    BYTE addr_type_1 = inst->addr_type_1;
    BYTE addr_type_2 = inst->addr_type_2;

    switch (addr_type_2) {
        case 1:
            switch (addr_type_1) {
                case 0:
                    registers[addr_2] = addr_1;
                    break;
                case 1:
                    registers[addr_2] = registers[addr_1];
                    break;
                case 2:
                    registers[addr_2] = stack[frame + addr_1];
                    break;
                case 3:
                    if (stack[frame + addr_1] >= STACK_SIZE) {
                        printf("Stack Overflow!\n");
                        return 1;
                    }
                    registers[addr_2] = stack[stack[frame + addr_1]];
            }
            break;
        case 2:
            if ((frame + addr_2) >= STACK_SIZE) {
                printf("Stack Overflow!\n");
                return 1;
            }
            switch (addr_type_1) {
                case 0:
                    stack[frame + addr_2] = addr_1;
                    break;
                case 1:
                    stack[frame + addr_2] = registers[addr_1];
                    break;
                case 2:
                    stack[frame + addr_2] = stack[frame + addr_1];
                case 3:
                    if (stack[frame + addr_1] >= STACK_SIZE) {
                        printf("Stack Overflow!\n");
                        return 1;
                    }
                    stack[frame + addr_2] = stack[stack[frame + addr_1]];
            }
            if (addr_2 >= STACK_POINTER) {
                STACK_POINTER = addr_2 + 1;
            }
            break;
        case 3:
            if (stack[frame + addr_2] >= STACK_SIZE) {
                printf("Stack Overflow!\n");
                return 1;
            }
            switch (addr_type_1) {
                case 0:
                    stack[stack[frame + addr_2]] = addr_1;
                    break;
                case 1:
                    stack[stack[frame + addr_2]] = registers[addr_1];
                    break;
                case 2:
                    stack[stack[frame + addr_2]] = stack[frame + addr_1];
                    break;
                case 3:
                    if (stack[frame + addr_1] >= STACK_SIZE) {
                        printf("Stack Overflow!\n");
                        return 1;
                    }
                    stack[stack[frame + addr_2]] = stack[stack[frame + addr_1]];
            }
            if (stack[frame + addr_2] >= (frame + STACK_POINTER)) {
                STACK_POINTER = stack[frame + addr_2] - frame + 1;
            }
            break;
    }
    return 0;
}

int cal(struct inst *inst) {

    int res = 0;
    // push return
    res = push(0);
    if (res == 1) { return 1; }
    // push function label
    res = push(curr_fn->label);
    if (res == 1) { return 1; }

    if (STACK_POINTER + 2 >= STACK_SIZE) {
        printf("Stack Overflow!\n");
        return 1;
    }
    stack[frame + STACK_POINTER] = STACK_POINTER;
    stack[frame + STACK_POINTER + 1] = 3;
    stack[frame + STACK_POINTER + 2] = 0;

    frame += STACK_POINTER;

    curr_fn = find_fn(inst->addr_1);
    if (curr_fn == NULL) {
        printf("Cannot find function %d\n", inst->addr_1);
        return 1;
    }

    for (int i = 0; i < curr_fn->num_args; i++) {
        res = push(stack[inst->addr_2 + i]);
        if (res == 1) { return 1; }
    }

    return 0;
}

int pop(struct inst *inst) {
    if (curr_fn->label == MAIN_FUNCTION) {
        printf("%d\n", stack[frame + inst->addr_1]);
    } else {
        stack[frame - 2] = stack[frame + inst->addr_1];
    }
    return 0;
}

int ret() {
    curr_fn = find_fn(stack[frame - 1]);
    frame -= stack[frame];
    return 0;
}

int add(struct inst *ins) {
    registers[ins->addr_1] = registers[ins->addr_1] + registers[ins->addr_2];
    return 0;
}

int and(struct inst *ins) {
    registers[ins->addr_1] = registers[ins->addr_1] & registers[ins->addr_2];
    return 0;
}

int not(struct inst *inst) {
    registers[inst->addr_1] = ~registers[inst->addr_1];
    return 0;
}

int equ(struct inst *inst) {
    registers[inst->addr_1] = (registers[inst->addr_1] == 0) ? 1 : 0;
    return 0;
}

struct fn *add_fn(int num_inst) {
    struct fn *new_fn = (struct fn *)calloc(1, sizeof(struct fn));
    new_fn->insts = (struct inst *)calloc(num_inst, sizeof(struct inst));
    new_fn->next = fns;

    fns = new_fn;
	return new_fn;
}

struct fn *find_main_fn() {
    return find_fn(MAIN_FUNCTION);
}

struct fn *find_fn(BYTE label) {
    struct fn *curr = fns;

    while (curr != NULL) {
        if (curr->label == label) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int push(BYTE value) {
    if ((frame + STACK_POINTER) >= STACK_SIZE) {
        printf("Stack Overflow!\n");
        return 1;
    }
    stack[frame + STACK_POINTER] = value;
    STACK_POINTER++;
    return 0;
}

void end() {
    struct fn *curr = fns;
    while (curr != NULL) {
        free(curr->insts);
        curr = curr->next;
    }
}
