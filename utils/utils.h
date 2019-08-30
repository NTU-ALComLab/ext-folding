#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iomanip>

using namespace std;

typedef const size_t cuint;

namespace utils
{

namespace bddUtils
{
// bddUtils.cpp
void printBddManInfo(DdManager *dd);
void printBddNodeInfo(DdManager *dd, DdNode *pNode);
void bddDumpPng(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName);
void bddDumpBlif(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName);
DdNode** computeSign(DdManager *dd, cuint &range);
DdNode* bddDot(DdManager *dd, DdNode **v1, DdNode **v2, cuint& len);
void bddNotVec(DdNode **vec, cuint& len);
void bddDerefVec(DdManager *dd, DdNode **v, cuint& len);
void bddFreeVec(DdManager *dd, DdNode **v, cuint& len);
st__table* createDummyState(DdManager *dd);
void bddFreeTable(DdManager *dd, st__table *tb);
} // end namespace bddUtils


namespace aigUtils
{
// aigUtils.cpp
Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, const size_t start, const size_t end);
Abc_Ntk_t* aigPerm(Abc_Ntk_t *pNtk, size_t *perm);
} // end namespace aigUtils


namespace fileWrite
{
// fileWrite.cpp
void writeKiss(cuint nPi, cuint nPo, cuint nSts, const vector<string>& stg, ostream& fp);
void addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, cuint cCnt, cuint nCnt, vector<string>& stg);
} // end namespace fileWrite

} // end namespace utils