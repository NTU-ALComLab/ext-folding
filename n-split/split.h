#ifndef __SPLIT_H__
#define __SPLIT_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace nSplit
{

// aigSplit.cpp
vector<Abc_Ntk_t*> aigSplit(Abc_Ntk_t *pNtk, cuint n);

} // end namespace nSplit

#endif /* __SPLIT_H__ */