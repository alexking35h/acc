#ifndef __REGALLOC_H__
#define __REGALLOC_H__
/*
 * Register Allocation
 * 
 * REG_ANY register indexes are mapped from an infinite domain (the output of the IR generation)
 * to a finite range - the register file of the target architecture. The allocation algorithm
 * is Linear scan, using the liveness-analysis information carried by REG_ANY registers in the .liveness
 * struct field.
 */
#include "ir.h"

/*
 * Number of arguments reserved for argument/return value passing.
 */
#define REGS_RESERVED 4

/*
 * Number of arguments reserved for register spill handling.
 */
#define REGS_SPILL 4

/*
 * This function tries to allocate all REG_ANY registers in each function a register index in the set
 * free_registers. Otherwise, registers are spilled to the stack, and store/load operations are
 * substituted in the code.
 * 
 * The first four registers in free_registers are reserved for spill code (the first is used for holding 
 * stack addresses in spill code; the remaining three are used for storing destination, left, and right
 * instruction registers).
 * 
 * The free_registers set must include at least REGS_SPILL registers. If |free_registers| = REGS_SPILL, 
 * all registers are spilled. free_registers is an array of integers, that must terminate in -1.
 */
void regalloc(IrFunction * program, int * free_registers);

#endif