#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "cpu.h"
#include "instruction_set.h"

#define MAX_LABELS 256
#define MAX_LINE_LEN 128

typedef struct {
    char name[32];
    uint32_t addr;
} Label;

Label label_table[MAX_LABELS];
size_t label_count = 0;

uint32_t origin_addr = START_ADDR;
uint32_t pc;

void trim(char *str) {
    char *end;

    while (isspace((unsigned char)*str)) str++;

    end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = 0;
}

void add_label(const char *label, uint32_t addr) {
    if (label_count <= MAX_LABELS) {
        printf("Found label: { %s, 0x%05x }\n", label, addr);

        strncpy(label_table[label_count].name, label, 32);
        label_table[label_count].addr = addr & ADDR_MASK;
        label_count++;
    }
}

uint32_t find_label(const char *label) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(label_table[i].name, label) == 0) {
            printf("\nlabel: { %s, 0x%05x }\n\n", label_table[i].name, label_table[i].addr);

            return label_table[i].addr;
        }
    }

    return 0;
}

uint8_t parse_opcode(const char *str) {
    if (strcasecmp(str, "MOV") == 0) return MOV;
    else if (strcasecmp(str, "LD") == 0) return LD;
    else if (strcasecmp(str, "ST") == 0) return ST;
    else if (strcasecmp(str, "PUSH") == 0) return PUSH;
    else if (strcasecmp(str, "POP") == 0) return POP;
    else if (strcasecmp(str, "ADD") == 0) return ADD;
    else if (strcasecmp(str, "SUB") == 0) return SUB;
    else if (strcasecmp(str, "INC") == 0) return INC;
    else if (strcasecmp(str, "DEC") == 0) return DEC;
    else if (strcasecmp(str, "AND") == 0) return AND;
    else if (strcasecmp(str, "OR") == 0) return OR;
    else if (strcasecmp(str, "XOR") == 0) return XOR;
    else if (strcasecmp(str, "NOT") == 0) return NOT;
    else if (strcasecmp(str, "SHL") == 0) return SHL;
    else if (strcasecmp(str, "SHR") == 0) return SHR;
    else if (strcasecmp(str, "CMP") == 0) return CMP;
    else if (strcasecmp(str, "JMP") == 0) return JMP;
    else if (strcasecmp(str, "JZ") == 0) return JZ;
    else if (strcasecmp(str, "JNZ") == 0) return JNZ;
    else if (strcasecmp(str, "JE") == 0) return JE;
    else if (strcasecmp(str, "JNE") == 0) return JNE;
    else if (strcasecmp(str, "JL") == 0) return JL;
    else if (strcasecmp(str, "JLE") == 0) return JLE;
    else if (strcasecmp(str, "JG") == 0) return JG;
    else if (strcasecmp(str, "JGE") == 0) return JGE;
    else if (strcasecmp(str, "CALL") == 0) return CALL;
    else if (strcasecmp(str, "RET") == 0) return RET;
    else if (strcasecmp(str, "IRET") == 0) return IRET;
    else if (strcasecmp(str, "INT") == 0) return INT;
    else if (strcasecmp(str, "CLI") == 0) return CLI;
    else if (strcasecmp(str, "STI") == 0) return STI;
    else if (strcasecmp(str, "NOP") == 0) return NOP;
    else if (strcasecmp(str, "HLT") == 0) return HLT;

    return 0x100;
}

uint16_t get_register(const char *str) {
    if (str[0] == 'R')
        return str[1] - '0';
    else if (strcmp(str, "SP") == 0)
        return SP;
    else if (strcmp(str, "PC") == 0)
        return PC;
    else if (strcmp(str, "CS") == 0)
        return CS;
    else if (strcmp(str, "SS") == 0)
        return SS;
    else if (strcmp(str, "DS") == 0)
        return DS;
    
    return 0;
}

uint32_t first_pass(FILE *file) {
    char line[MAX_LINE_LEN];
    pc = START_ADDR;

    while (fgets(line, sizeof(line), file)) {
        trim(line);
        
        if (line[0] == '\0' || line[0] == ';') continue;

        if (strncmp(line, "org", 3) == 0) {
            unsigned int org;

            if (sscanf(line + 3, "%x", &org) == 1) {
                origin_addr = (uint32_t)org & ADDR_MASK;
                pc = origin_addr;
            }

            continue;
        }

        if (strncmp(line, "db", 2) == 0) {
            pc++;

            continue;
        }

        char *colon = strchr(line, ':');

        if (colon) {
            *colon = '\0';

            trim(line);
            add_label(line, pc);

            if (label_count == 1 && strcmp(line, "start") != 0) {
                printf("Error: Could not find label 'start'\n  > found label '%s' instead\n", line);

                return -1;
            }

            continue;
        }

        char mnemonic[16];

        sscanf(line, "%s", mnemonic);

        Instruction dummy = {.opcode = parse_opcode(mnemonic)};

        pc += INST_SIZE;
    }

    rewind(file);

    return pc;
}

int second_pass(FILE *in, FILE *out) {
    char line[MAX_LINE_LEN];
    int curr_line = 0;
    pc = origin_addr;

    fputc((pc >> 16) & 0x0f, out);
    fputc((pc >> 8) & 0xff, out);
    fputc(pc & 0xff, out);

    fputc(0, out);
    fputc(0, out);
    fputc(0, out);

    while (fgets(line, sizeof(line), in)) {
        curr_line++;
        trim(line);

        if (line[0] == '\0' || line[0] == ';') continue;
        if (strncmp(line, "org", 3) == 0) continue;
        if (strchr(line, ':')) continue;

        if (strncmp(line, "db", 2) == 0) {
            unsigned int byte;

            if (sscanf(line + 2, "%x", &byte) == 1)
                fputc(byte & 0xff, out);
            
            pc++;

            continue;
        }

        char mnemonic[16] = {0}, op1[32] = {0}, op2[32] = {0};
        int parts = sscanf(line, "%s %[^,], %s", mnemonic, op1, op2);
        uint8_t opcode = parse_opcode(mnemonic);

        if (opcode > 0xff) {
            printf("Error on line %d: Unknown instruction\n  > %s\n", curr_line, mnemonic);

            return 1;
        }

        Instruction inst = {
            .opcode = opcode,
            .mode1 = MODE_VAL_IND,
            .operand1 = 0,
            .mode2 = MODE_VAL_IND,
            .operand2 = 0
        };

        if (parts >= 2 && op1[0] == '#')
            inst.mode1 = MODE_VAL_IMM;
        
        if (parts == 3 && op2[0] == '#')
            inst.mode2 = MODE_VAL_IMM;
        
        if (parts >= 2) {
            if (inst.opcode == JMP || inst.opcode == JZ || inst.opcode == JNZ || inst.opcode == JE || inst.opcode == JNE || inst.opcode == JL || inst.opcode == JLE || inst.opcode == JG || inst.opcode == JGE || inst.opcode == CALL) {
                char *value = op1;
                
                if (value[0] == '#') value++;

                char *endptr;

                inst.operand1 = (uint32_t)strtol(value, &endptr, 0);

                if (*endptr != '\0') {
                    inst.operand1 = find_label(op1);

                    if (inst.operand1 == (uint32_t)0) {
                        printf("Error on line %d: Undefined label\n  > %s\n", curr_line, line);

                        return 1;
                    }
                }
            } else {
                if (inst.mode1 == MODE_VAL_IMM)
                    inst.operand1 = (uint16_t)strtol(op1 + 1, NULL, 0);
                else if (op1[0] == 'R' || strcmp(op1, "SP") == 0 || strcmp(op1, "PC") == 0 || strcmp(op1, "CS") == 0 || strcmp(op1, "SS") == 0 || strcmp(op1, "DS") == 0)
                    inst.operand1 = get_register(op1);
            }
        }

        if (parts == 3) {
            if (inst.mode2 == MODE_VAL_IMM)
                inst.operand2 = (uint16_t)strtol(op2 + 1, NULL, 0);
            else if (op2[0] == 'R' || strcmp(op2, "SP") == 0 || strcmp(op2, "PC") == 0 || strcmp(op2, "CS") == 0 || strcmp(op2, "SS") == 0 || strcmp(op2, "DS") == 0)
                inst.operand2 = get_register(op2);
            else
                inst.operand2 = (uint16_t)strtol(op2, NULL, 0);
        }

        printf("line %d: %s\n  addr: 0x%05x\n  bytes: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            curr_line, line, pc,
            inst.opcode, inst.mode1,
            (inst.operand1 >> 8) & 0xff,
            inst.operand1 & 0xff,
            inst.mode2,
            (inst.operand2 >> 8) & 0xff,
            inst.operand2 & 0xff
        );

        fputc(inst.opcode, out);
        fputc(inst.mode1, out);
        fputc((inst.operand1 >> 8) & 0xff, out);
        fputc(inst.operand1 & 0xff, out);
        fputc(inst.mode2, out);
        fputc((inst.operand2 >> 8) & 0xff, out);
        fputc(inst.operand2 & 0xff, out);

        pc += INST_SIZE;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char *in_file = NULL;
    char *out_file = NULL;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
            in_file = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            out_file = argv[++i];
    }

    FILE *input_file = fopen(in_file, "r");

    if (!input_file) {
        printf("Unable to open file %s\n", in_file);

        return 1;
    }

    FILE *output_file = fopen(out_file, "wb");

    if (!output_file) {
        printf("Unable to create file %s\n", out_file);
        fclose(input_file);

        return 1;
    }

    first_pass(input_file);
    second_pass(input_file, output_file);

    fclose(input_file);

    fseek(output_file, 0, SEEK_END);

    uint32_t file_size = (uint32_t)ftell(output_file);

    fseek(output_file, 3, SEEK_SET);
    fputc((file_size >> 16) & 0x0f, output_file);
    fputc((file_size >> 8) & 0xff, output_file);
    fputc(file_size & 0xff, output_file);

    fclose(output_file);

    printf("Wrote %d bytes to %s successfully\n", file_size, out_file);

    return 0;
}