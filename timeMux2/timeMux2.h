#include "ext-folding/utils/utils.h"

using namespace utils;

namespace timeMux2
{

// bddMux.cpp
int bddMux2(Abc_Ntk_t *pNtk, cuint nTimeFrame, int *iPerm, int *oPerm, vector<string>& stg, const bool verbosity, const char *logFileName);
// reord.cpp
size_t reordIO(Abc_Ntk_t *pNtk, DdManager *dd, cuint nTimeFrame, int *iPerm, int *oPerm, TimeLogger *logger, const bool verbosity);
// checkEqv.cpp
//void checkEqv(Abc_Ntk_t *pNtk, size_t *perm, cuint nTimeFrame, const vector<string>& stg, cuint nSts);

} // end namespace timeMux2