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

namespace timeFold
{

namespace bddUtils
{
// bddUtils.cpp
void printBddManInfo(DdManager *dd);
void printBddNodeInfo(DdManager *dd, DdNode *pNode);
void showBdd(DdManager *dd, DdNode **pNodeVec, int nNode, string fileName);
DdNode** computeSign(DdManager *dd, cuint &range);
DdNode* bddDot(DdManager *dd, DdNode **v1, DdNode **v2, cuint& len);
void bddNotVec(DdNode **vec, cuint& len);
void bddDerefVec(DdManager *dd, DdNode **v, cuint& len);
void bddFreeVec(DdManager *dd, DdNode **v, cuint& len);
st__table* createDummyState(DdManager *dd);
void bddFreeTable(DdManager *dd, st__table *tb);

// bddFold.cpp
int bddFold(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, const bool verbosity);
} // end namespace bddUtils


namespace aigUtils {}


namespace fileWrite
{
// fileWrite.cpp
void writeKiss(cuint nPi, cuint nPo, cuint nSts, const vector<string>& stg, ostream& fp);
void addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, cuint cCnt, cuint nCnt, vector<string>& stg);
} // end namespace fileWrite

} // end namespace timeFold