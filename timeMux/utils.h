#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "ext-folding/timeFold/utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iomanip>

namespace timeMux
{
namespace bddUtils
{
// bddMux.cpp
int bddMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, const bool reOrd, const bool verbosity, const Cudd_ReorderingType rt = CUDD_REORDER_SIFT);
} // end namespace bddUtils

} // end namespace timeMux