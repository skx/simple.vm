/**
 *
 * This is a simple virtual-machine which uses registers, rather than
 * the stack.
 *
 * The virtual machine will execute the contents of the named program-file,
 * by reading the file and parsing the binary-opcodes.
 *
 * Each opcode is an integer between 0-255 inclusive, and will have a variable
 * number of arguments.  The opcodes are divided into classes which are
 * broadly:
 *
 * * Storing values in registers.
 * * Running mathematical operations on registers.
 * * etc.
 *
 * For example the addition operation looks like this:
 *
 *    ADD Register-For-Result, Source-Register-One, Source-REgister-Two
 *
 * And the storing of a number in a register looks like this:
 *
 *   STORE Register-For-Result NUMBER
 *
 * The machine has 10 registers total, numbered 00-09.
 *
 *
 * Steve
 *
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


/**
 * Count of registers.
 */
#define REGISTER_COUNT 10


#ifndef _Bool
#define _Bool short
#define true   1
#define false  0
#endif


/***
 * Our "opcodes".
 */
#define OPCODE_EXIT   0x00      /* EXIT() - Helpfully zero. */

#define STORE_STRING  0x01      /* STORE_STRING( RegN, LEN, "STRING" ) */
#define STORE_INT     0x02      /* STORE_INT( RegN, val ) */

#define PRINT_STRING  0x03      /* PRINT_STRING( RegN ) */
#define PRINT_INT     0x04      /* PRINT_INT( RegN ) */

#define JUMP_TO       0x06      /* JUMP_TO address */
#define JUMP_Z        0x07      /* JUMP_Z address */
#define JUMP_NZ       0x08      /* JUMP_NZ address */

#define ADD_OP 0x09
#define SUB_OP 0x0A
#define MUL_OP 0x0B
#define DIV_OP 0x0C

#define SYSTEM_STRING 0x0D



/**
 * The type of content a register has.
 */
enum TypeTag { INT, STR };

/**
 * A single register, which may be used to store a string or an integer.
 */
typedef struct registers {
    unsigned int num;
    char *str;
    enum TypeTag type;
} reg_t;



/**
 * Flags.
 *
 * The add/sub instructions set the Z flag if the result is zero.
 *
 * This flag can then be used for the JMP_Z and JUMP_NZ instructions.
 */
typedef struct flags {
    _Bool z;
} flag_t;


/**
 * The CPU type, which contains:
 *
 * 1.  An array of registers.
 * 2.  An instruction pointer.
 * 3.  A set of code to execute - which has a size.
 *
 */
typedef struct cpu {
    reg_t registers[REGISTER_COUNT];
    flag_t flags;
    unsigned int esp;
    unsigned int size;
    unsigned char *code;

    unsigned int labels[255];
} cpu_t;



/**
 * Allocate a new CPU.
 */
cpu_t *cpu_new(unsigned char *code, unsigned int size)
{
    cpu_t *cpun;
    int i;


    if (!code || !size)
        return NULL;

    cpun = malloc(sizeof(struct cpu));
    if (!cpun)
        return NULL;

    memset(cpun, '\0', sizeof(struct cpu));

    cpun->esp = 0;
    cpun->size = size;
    cpun->code = code;

    /**
     * Explicitly zero each regiester and set to be a number.
     */
    for (i = 0; i < REGISTER_COUNT; i++)
    {
        cpun->registers[i].type = INT;
        cpun->registers[i].num = 0;
        cpun->registers[i].str = NULL;
    }

    return cpun;
}


/**
 * Show the content of the various registers.
 */
void dump_regs(cpu_t * cpup)
{
    int i;

    printf("Register dump\n");

    for (i = 0; i < REGISTER_COUNT; i++)
    {
        if (cpup->registers[i].type == STR)
        {
            printf("\tRegister %02d - str: %s\n", i, cpup->registers[i].str);
        } else if (cpup->registers[i].type == INT)
        {
            printf("\tRegister %02d - Decimal:%04d [Hex:%04X]\n", i,
                   cpup->registers[i].num, cpup->registers[i].num);
        } else
        {
            printf("\tRegister %02d has unknown type!\n", i);
        }
    }
}


/**
 * Delete a CPU.
 */
void cpu_del(cpu_t * cpup)
{
    if (!cpup)
        return;

    dump_regs(cpup);
    free(cpup);
}


/**
 *  Main virtual machine execution loop
 */
void cpu_run(cpu_t * cpup)
{
    static unsigned int iterations = 0;
    int run = 1;

    if (!cpup)
        return;


    cpup->esp = 0;
    while (run && (cpup->esp < cpup->size))
    {
      restart:

        if (getenv("DEBUG") != NULL)
            printf("%04x - Parsing OpCode: %d [Hex:%02x]\n", cpup->esp,
                   cpup->code[cpup->esp], cpup->code[cpup->esp]);


        switch (cpup->code[cpup->esp])
        {

        case PRINT_INT:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];

                if (cpup->registers[reg].type == INT)
                {
                    printf("[stdout] register R%02d = %d [Hex:%04x]\n",
                           cpup->code[reg],
                           cpup->registers[reg].num, cpup->registers[reg].num);
                } else
                {
                    printf
                        ("ERROR Tried to print integer contents of register %02x but it is a string\n",
                         reg);
                }
                break;
            }

        case PRINT_STRING:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];

                if (cpup->registers[reg].type == STR)
                {
                    printf("[stdout] register R%02d = %s\n", reg,
                           cpup->registers[reg].str);
                } else
                {
                    printf
                        ("ERROR Tried to print string contents of register %02x but it is an integer\n",
                         reg);
                }
                break;
            }


        case SYSTEM_STRING:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];

                if (cpup->registers[reg].type == STR)
                {
                    system(cpup->registers[reg].str);
                } else
                {
                    printf
                        ("ERROR Tried to execute the contents register %02x but it is an integer\n",
                         reg);
                }
                break;
            }

        case JUMP_TO:
            {
                cpup->esp++;

                short off1 = cpup->code[cpup->esp];
                cpup->esp++;
                short off2 = cpup->code[cpup->esp];

                int offset = off1 + (256 * off2);

                if (getenv("DEBUG") != NULL)
                    printf("Should jump to offset %04d [Hex:%04x]\n", offset, offset);

                cpup->esp = offset;
                goto restart;
                break;
            }



        case JUMP_Z:
            {
                cpup->esp++;

                short off1 = cpup->code[cpup->esp];
                cpup->esp++;
                short off2 = cpup->code[cpup->esp];

                int offset = off1 + (256 * off2);

                if (getenv("DEBUG") != NULL)
                    printf("Should jump to offset %04d [Hex:%04x] if Z\n", offset,
                           offset);

                if (cpup->flags.z)
                {
                    cpup->esp = offset;
                    goto restart;
                }
                break;
            }


        case JUMP_NZ:
            {
                cpup->esp++;

                short off1 = cpup->code[cpup->esp];
                cpup->esp++;
                short off2 = cpup->code[cpup->esp];

                int offset = off1 + (256 * off2);

                if (getenv("DEBUG") != NULL)
                    printf("Should jump to offset %04d [Hex:%04x] if NOT Z\n", offset,
                           offset);

                if (!cpup->flags.z)
                {
                    cpup->esp = offset;
                    goto restart;
                }
                break;
            }



        case STORE_INT:
            {
                /* store an int in a register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                cpup->esp++;

                /* get the value */
                unsigned int val1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int val2 = cpup->code[cpup->esp];

                int value = val1 + (256 * val2);

                if (getenv("DEBUG") != NULL)
                    printf("STORE_INT(Reg:%02x) => %04d [Hex:%04x]\n", reg, value, value);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                cpup->registers[reg].num = value;
                cpup->registers[reg].type = INT;

                break;
            }

        case ADD_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf("Tried to add two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num +
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

            /**
             * Overflow?!
             */
                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;


                break;
            }

        case SUB_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);


                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf
                        ("Tried to subtract two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num =
                    cpup->registers[src1].num - cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;

                break;
            }

        case MUL_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf
                        ("Tried to multiply two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num *
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

                break;
            }

        case DIV_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf
                        ("Tried to subtract two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num /
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

                break;
            }

        case STORE_STRING:
            {
                /* store a string in a register */
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                cpup->esp++;

                /* get the length */
                unsigned int len = cpup->code[cpup->esp];
                cpup->esp++;

            /**
             * If we already have a string in the register delete it.
             */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                {
                    free(cpup->registers[reg].str);
                }

            /**
             * Store the new string and set the register type.
             */
                cpup->registers[reg].type = STR;
                cpup->registers[reg].str = malloc(len + 1);
                memset(cpup->registers[reg].str, '\0', len + 1);

                int i;
                for (i = 0; i < (int) len; i++)
                {
                    cpup->registers[reg].str[i] = cpup->code[cpup->esp];
                    cpup->esp++;
                }

                if (getenv("DEBUG") != NULL)
                    printf("STORE_STRING(Reg:%02x => \"%s\" [%02x bytes]\n", reg,
                           cpup->registers[reg].str, len);

                cpup->esp--;

                break;
            }

        case OPCODE_EXIT:
            {
                if (getenv("DEBUG") != NULL)
                    printf("exit\n");
                run = 0;
                break;
            }

        default:
            printf("UNKNOWN INSTRUCTION: %d [Hex: %2X]\n",
                   cpup->code[cpup->esp], cpup->code[cpup->esp]);
            break;
        }
        cpup->esp++;

        iterations++;
    }

    if (getenv("DEBUG") != NULL)
        printf("Executed %u instructions\n", iterations);
}



/**
 * Simple driver to launch our virtual machine.
 *
 * Given a filename parse/execute the opcodes contained within it.
 *
 */
int main(int argc, char **argv)
{
    struct stat sb;

    if (argc < 2)
    {
        printf("Usage: %s input-file\n", argv[0]);
        return 0;
    }


    if (stat(argv[1], &sb) != 0)
    {
        printf("Failed to read file: %s\n", argv[1]);
        return 0;
    }

    int size = sb.st_size;

    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("[SVM] Failed to load program %s\n", argv[1]);
        return -1;
    }

    unsigned char *code = malloc(size);
    memset(code, '\0', size);

    if (!code)
    {
        fclose(fp);
        return 0;
    }

    fread(code, 1, size, fp);
    fclose(fp);

    cpu_t *cpu = cpu_new(code, size);
    if (!cpu)
        return 0;

    cpu_run(cpu);
    cpu_del(cpu);
    free(code);                 /* free code */

    return 1;                   /* return success */
}
