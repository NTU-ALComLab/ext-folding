#ifndef __TIMEMUX4_H__
#define __TIMEMUX4_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux4
{

// aigMux.cpp
Abc_Ntk_t* aigStrMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, int *oPerm, const bool verbose, const char *logFileName);
void checkEqv(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, int *oPerm, cuint nTimeFrame);

// perm.cpp
Abc_Ntk_t* permPi(Abc_Ntk_t *pNtk, int *iPerm, const bool verbose);
} // end namespace timeMux4

#endif /* __TIMEMUX4_H__ */