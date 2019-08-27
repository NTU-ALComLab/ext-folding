#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux
{

// bddMux.cpp
int bddMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, size_t *perm, const bool verbosity, const Cudd_ReorderingType rt = CUDD_REORDER_EXACT);

// checkEqv.cpp
void checkEqv(Abc_Ntk_t *pNtk, size_t *perm, cuint nTimeFrame, const vector<string>& stg, cuint nSts);

} // end namespace timeMux