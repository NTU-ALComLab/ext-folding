#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux
{

// bddMux.cpp
int bddMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, int *perm, const bool verbosity, const char *logFileName, const Cudd_ReorderingType rt = CUDD_REORDER_SYMM_SIFT);

// checkEqv.cpp
void checkEqv(Abc_Ntk_t *pNtk, int *perm, cuint nTimeFrame, const vector<string>& stg, cuint nSts);

} // end namespace timeMux