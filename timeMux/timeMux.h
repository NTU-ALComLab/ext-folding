#ifndef __TIMEMUX_H__
#define __TIMEMUX_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux
{

// bddMux.cpp
STG* bddMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, int *perm, const bool verbosity, const char *logFileName, const Cudd_ReorderingType rt = CUDD_REORDER_SYMM_SIFT);

// checkEqv.cpp
void checkEqv(Abc_Ntk_t *pNtk, int *perm, cuint nTimeFrame, STG *stg, const bool onlyLast);
void dumpSTG(Abc_Ntk_t *pNtk, cuint nTimeFrame, STG *stg, cuint pid);
void dumpBLIF(cuint pid);

} // end namespace timeMux

#endif /* __TIMEMUX_H__ */