#ifndef __TIMEMUX2_H__
#define __TIMEMUX2_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux2
{

// bddMux.cpp
STG* bddMux2(Abc_Ntk_t *pNtk, cuint nTimeFrame, uint &nPo, int *iPerm, int *oPerm, const bool verbose, const char *logFileName, cuint expConfig);

// reord.cpp
uint reordIO(Abc_Ntk_t *pNtk, DdManager *dd, cuint nTimeFrame, int *iPerm, int *oPerm, TimeLogger *logger, const bool verbose, cuint expConfig);
bool checkPerm(int *perm, cuint size, cuint cap);

// reord2.cpp
void manualReord(DdManager *dd, int *iPerm, int *oPerm, uint &nPo, cuint expConfig);

// checkEqv.cpp
void checkEqv(Abc_Ntk_t *pNtk, int *iPerm, int *oPerm, cuint nTimeFrame, STG *stg);
int* extendPerm(int *perm, cuint fromSize, cuint toSize);

} // end namespace timeMux2

#endif /* __TIMEMUX2_H__ */