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
void bddShowManInfo(DdManager *dd);
void bddShowNodeInfo(DdManager *dd, DdNode *pNode);
void bddDumpPng(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName);
void bddDumpBlif(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName);
DdNode** bddComputeSign(DdManager *dd, cuint &range);
DdNode* bddDot(DdManager *dd, DdNode **v1, DdNode **v2, cuint& len);
void bddNotVec(DdNode **vec, cuint& len);
void bddDerefVec(DdManager *dd, DdNode **v, cuint& len);
void bddFreeVec(DdManager *dd, DdNode **v, cuint& len);
st__table* bddCreateDummyState(DdManager *dd);
void bddFreeTable(DdManager *dd, st__table *tb);
} // end namespace bddUtils


namespace aigUtils
{
// aigUtils.cpp
Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, cuint start, cuint end, bool rm=false);
Abc_Ntk_t* aigSingleCone(Abc_Ntk_t *pNtk, cuint n, bool rm=false);
Abc_Ntk_t* aigPerm(Abc_Ntk_t *pNtk, size_t *perm, bool rm=false);
Abc_Ntk_t* aigConcat(Abc_Ntk_t **pNtks, cuint nNtks, bool rm=false);
Abc_Ntk_t* aigMiter(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, const bool fCompl, bool rm = false);
Abc_Ntk_t* aigCreateDummyState();

Abc_Obj_t* aigDot(Abc_Ntk_t* pNtk, Abc_Obj_t** v1, Abc_Obj_t** v2, cuint len);
Abc_Obj_t* aigBitsToCube(Abc_Ntk_t *pNtk, cuint n, Abc_Obj_t **pVars, cuint nVars);
Abc_Obj_t* aigIthVar(Abc_Ntk_t *pNtk, cuint i);
Abc_Obj_t** aigComputeSign(Abc_Ntk_t *pNtk, cuint range, bool fAddPo = true);

void aigRemovePo(Abc_Ntk_t* pNtk, cuint idx);
} // end namespace aigUtils


namespace fileWrite
{
// fileWrite.cpp
void writeKiss(cuint nPi, cuint nPo, cuint nSts, const vector<string>& stg, ostream& fp);
void addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, cuint cCnt, cuint nCnt, vector<string>& stg);
} // end namespace fileWrite

class TimeLogger
{
public:
    TimeLogger(const string& fileName="runtime.log", bool v=false, bool f=true);
    ~TimeLogger();
    void log(const string& str);
private:
    bool        verbose, instantFlush;
    clock_t     start, prev;
    ofstream    fp;
}; // end class TimeLogger

} // end namespace utils