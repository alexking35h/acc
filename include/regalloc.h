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
 * Default number of registers available to the allocator, including REGS_SPILL.
 * This is determined by the target architecture.
 * 
 * For Aarch32, this is {R4, R5, R6, R7, R8, R9, R10, R11, R12}.
 */
#define REGS_ALLOCABLE_AARCH32 9

/*
 * Perform Register allocation
 * 
 * This function allocates all REG_ANY registers in each function, a register index in the range
 * (REGS_RESERVED + REG_SPILL, REGS_RESERVED + allocable_regs). Otherwise, registers are spilled
 * to the stack, and store/load operations are substituted in the code.
 * 
 * Register indexes are numbered from 0:
 *  - [0, REGS_RESERVED) - used by call arguments/return values
 *  - [REGS_RESERVED, REGS_RESERVED + REGS_SPILL) - used for storing/loading registers spilled to memory.
 *  - [REGS_RESERVED + REGS_SPILL, REGS_RESERVED + allocable_regs)
 * 
 * allocable_regs is the number of unreserved registers available to the allocator. allocable_regs must be
 * >= REGS_SPILL (if allocable_regs == REGS_SPILL, all registers are spilled to memory).
 */
void regalloc(IrFunction * program, int allocable_regs);

#endif