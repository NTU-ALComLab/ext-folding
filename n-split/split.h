#ifndef __SPLIT_H__
#define __SPLIT_H__

#include "ext-folding/utils/utils.h"

using namespace utils;

namespace nSplit
{

typedef pair<string, string> InfoPair;
typedef vector<InfoPair> InfoPairs;

// aigSplit.cpp
vector<Abc_Ntk_t*> aigSplit(Abc_Ntk_t *pNtk, cuint n);
void checkEqv(Abc_Ntk_t *pNtk, vector<Abc_Ntk_t*> vNtks);

} // end namespace nSplit

#endif /* __SPLIT_H__ */