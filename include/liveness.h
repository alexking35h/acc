#ifndef __LIVENESS_H__
#define __LIVENESS_H__
/*
 * Register Liveness Analysis
 * 
 * Live ranges are generated for all REG_ANY type registers, within the context
 * of functions (IrFunction) in the Intermediate Representation (ir.h)
 * 
 * This is a precursor to Linear Scan registory allocation, where we attempt to 
 * select the optimal choice of registers for keeping in physical registers, or
 * spilling to memory.
 * 
 */
#include "ir.h"

/*
 * Perform Liveness analysis.
 * 
 * After calling this function, the 'enter' and 'exit' fields are set
 * in all REG_ANY registers used in the program.
 */
void Liveness_analysis(IrProgram * program);

#endif