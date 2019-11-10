#ifndef __TIMEMUX3_H__
#define __TIMEMUX3_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux3
{

// aigMux.cpp
Abc_Ntk_t* aigStrMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, const bool verbose, const char *logFileName);

// checkEqv.cpp
//void checkEqv(Abc_Ntk_t *pNtk, int *perm, cuint nTimeFrame, STG *stg, const bool onlyLast);

} // end namespace timeMux3

#endif /* __TIMEMUX3_H__ */