#include "cpu.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define DATA_LEN 6
#define SP 7

// /**
// Helper function to write a value to the specified index in RAM
void cpu_ram_write(struct cpu *cpu, unsigned char val, unsigned char index)
{
  cpu->ram[index] = val;
}

/**
 * Helper function to read a value from the specified index in RAM
 * Returns the read value
 */
unsigned char cpu_ram_read(struct cpu *cpu, unsigned char index)
{
  return cpu->ram[index];
}

/**
 * Load the binary bytes from a .ls8 source file into a RAM array
 */

void cpu_load(char *filename, struct cpu *cpu)
{

  FILE *fp;
  char line[1024];

  int address = 0;

  // THIS WAS HARDCODED BEFORE
  // for (int i = 0; i < DATA_LEN; i++) {
  //   cpu->ram[address++] = data[i];
  // }

  // Open the source file
  if ((fp = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "Cannot open file %s\n", filename);
    exit(2);
  }

  // Read all the lines and store them in RAM
  while (fgets(line, sizeof(line), fp) != NULL)
  {

    // Convert the string to a number
    char *endchar;
    unsigned char byte = strtoul(line, &endchar, 2);

    // Ignores lines from which no numbers were read
    if (endchar == line)
    {
      continue;
    }

    // Store in RAM
    cpu_ram_write(cpu, byte, address++);
  }
}

/////////////////////////////////////////////////////
//TRACE WHAT IS HAPPENING AT CPU LEVEL
void trace(struct cpu *cpu)
{
  printf("%d | ", cpu->pc);

  printf("%d %d %d |",
         cpu_ram_read(cpu, cpu->pc),
         cpu_ram_read(cpu, cpu->pc + 1),
         cpu_ram_read(cpu, cpu->pc + 2));

  for (int i = 0; i < 8; i++)
  {
    printf(" %d", cpu->reg[i]);
  }

  printf("\n");
}
/////////////////////////////////////////////////////

// void cpu_load(char *filename, struct cpu *cpu)
// {
//   // char data[DATA_LEN] = {
//   //     // From print8.ls8
//   //     0b10000010, // LDI R0,8
//   //     0b00000000,
//   //     0b00001000,
//   //     0b01000111, // PRN R0
//   //     0b00000000,
//   //     0b00000001 // HLT
//   // };
//   // for (int i = 0; i < DATA_LEN; i++)
//   // {
//   //   cpu->ram[address++] = data[i];
//   // }
//   // TODO: Replace this with something less hard-coded
//   FILE *fp;
//   char line[1024];
//   int address = 0;

//   //Open source file
//   fp = fopen(filename, "r");
//   if (fp == NULL)
//   {
//     fprintf(stderr, "cannot open file %s\n", filename);
//     exit(2);
//   }
//   while (fgets(line, 128, fp) != NULL)
//   {
//     char *endchar;
//     unsigned char byte = strtol(line, &endchar, 2);
//     if (endchar == line)
//     {
//       continue;
//     }
//     cpu_ram_write(cpu, byte, address++);
//   }
// }

/**
 * ALU
 */
void alu(struct cpu *cpu, enum alu_op op, unsigned char regA, unsigned char regB)
{
  switch (op)
  {
  case ALU_MUL:
    // TODO
    cpu->reg[regA] = cpu->reg[regA] * cpu->reg[regB];
    break;

  case ALU_CMP:
    if (cpu->reg[regA] == cpu->reg[regB])
    {
      cpu->FL = 0b00000001;
    }
    else if (cpu->reg[regA] < cpu->reg[regB])
    {
      cpu->FL = 0b00000100;
    }
    else if (cpu->reg[regA] > cpu->reg[regB])
    {
      cpu->FL = 0b00000010;
    }
    else if (cpu->reg[regA] != cpu->reg[regB])
    {
      cpu->FL = 0b00000000;
    }
    break;
  }
}

/**
 * Run the CPU
 */
void cpu_run(struct cpu *cpu)
{
  int running = 1; // True until we get a HLT instruction

  while (running)
  {
    unsigned char operandA = 0;
    unsigned char operandB = 0;

    // TODO
    // 1. Get the value of the current instruction (in address PC).
    unsigned char IR = cpu_ram_read(cpu, cpu->pc);
    // 2. Figure out how many operands this next instruction requires
    unsigned int num_operands = IR >> 6;
    // 3. Get the appropriate value(s) of the operands following this instruction
    if (num_operands == 2)
    {
      operandA = cpu_ram_read(cpu, (cpu->pc + 1) & 0xff);
      operandB = cpu_ram_read(cpu, (cpu->pc + 2) & 0xff);
    }
    else if (num_operands == 1)
    {
      operandA = cpu_ram_read(cpu, (cpu->pc + 1) & 0xff);
    }
    // else
    // {
    //   reutrn(1);
    // }
    // 4. switch() over it to decide on a course of action.
    int instruction_set_pc = (IR >> 4) & 1;

    // 5. Do whatever the instruction should do according to the spec.

    // 6. Move the PC to the next instruction.
    trace(cpu);
    switch (IR)
    {
    case HLT:
      running = 0;
      break;

    case LDI:
      cpu->reg[operandA] = operandB;
      break;

    case PRN:
      printf("%d\n", cpu->reg[operandA]);
      break;

    case MUL:
      alu(cpu, ALU_MUL, operandA, operandB);
      break;

    case PUSH:
      cpu->reg[SP]--;
      cpu->ram[cpu->reg[SP]] = cpu->reg[operandA];
      break;

    case POP:
      cpu->reg[operandA] = cpu->ram[cpu->reg[SP]];
      cpu->reg[SP]++;
      break;

    case CMP:
      alu(cpu, ALU_CMP, operandA, operandB);
      break;

    case JMP:
      cpu->pc = cpu->reg[operandA];
      break;

    case JEQ:
      if (cpu->FL == 00000001)
      {
        cpu->pc = cpu->reg[operandA];
      }
      else
      {
        cpu->pc += 2;
      }
      break;

    case JNE:
      if (cpu->FL != 00000001)
      {
        cpu->pc = cpu->reg[operandA];
      }
      else
      {
        cpu->pc += 2;
      }
      break;

    default:
      fprintf(stderr, "PC %02x: unknown instruction %02x\n", cpu->pc, IR);
      exit(3);
    }
    if (!instruction_set_pc)
    {
      cpu->pc += num_operands + 1;
    }
  }
}

// *Initialize a CPU struct
// * /
void cpu_init(struct cpu *cpu)
{
  // TODO: Initialize the PC and other special registers
  // initialize PC
  cpu->pc = 0;
  cpu->FL = 0;
  memset(cpu->ram, 0, sizeof(cpu->ram));
  memset(cpu->reg, 0, sizeof(cpu->reg));

  //Init Stack Pointer
}
