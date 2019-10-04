#ifndef __TIMEFOLD_H__
#define __TIMEFOLD_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeFold
{

// bddFold.cpp
STG* bddFold(Abc_Ntk_t *pNtk, cuint nTimeFrame, const bool verbosity, const char* logFileName);
void buildTrans(DdManager *dd, DdNode **pNodeVec, DdNode **B, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, st__table *csts, st__table *nsts, STG *stg);

// aigFold.cpp

} // end namespace timeFold

#endif /* __TIMEFOLD_H__ */