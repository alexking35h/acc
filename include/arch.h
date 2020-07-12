#ifndef __ARCH_H__
#define __ARCH_H__

#include "ctype.h"

int arch_get_align(CType *);

int arch_get_size(CType *);

bool arch_get_signed(CType *);

#endif