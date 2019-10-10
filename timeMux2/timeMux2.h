#ifndef __TIMEMUX2_H__
#define __TIMEMUX2_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux2
{

// bddMux.cpp
STG* bddMux2(Abc_Ntk_t *pNtk, cuint nTimeFrame, uint &nPo, int *iPerm, int *oPerm, const bool verbosity, const char *logFileName, cuint expConfig);

// reord.cpp
uint reordIO(Abc_Ntk_t *pNtk, DdManager *dd, cuint nTimeFrame, int *iPerm, int *oPerm, TimeLogger *logger, const bool verbosity, cuint expConfig);
bool checkPerm(int *perm, cuint size, cuint cap);

// checkEqv.cpp
void checkEqv(Abc_Ntk_t *pNtk, int *iPerm, int *oPerm, cuint nTimeFrame, STG *stg);

} // end namespace timeMux2

#endif /* __TIMEMUX2_H__ */