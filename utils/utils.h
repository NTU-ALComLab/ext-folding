#ifndef __UTILS_H__
#define __UTILS_H__

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

namespace utils
{

typedef unsigned uint;
typedef const uint cuint;

namespace bddUtils
{
// bddUtils.cpp
void bddShowManInfo(DdManager *dd);
void bddShowNodeInfo(DdManager *dd, DdNode *pNode);
void bddDumpPng(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName);
void bddDumpBlif(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName);
void bddNotVec(DdNode **vec, cuint len);
void bddDerefVec(DdManager *dd, DdNode **v, cuint len);
void bddFreeVec(DdManager *dd, DdNode **v, cuint len);
void bddFreeTable(DdManager *dd, st__table *tb);
void bddReordRange(DdManager *dd, cuint lev, cuint size, const Cudd_ReorderingType rt = CUDD_REORDER_SYMM_SIFT);

DdNode* bddDot(DdManager *dd, DdNode **v1, DdNode **v2, cuint len);
DdNode** bddComputeSign(DdManager *dd, cuint range, int stIdx = -1);
st__table* bddCreateDummyState(DdManager *dd);
} // end namespace bddUtils


namespace aigUtils
{
// aigUtils.cpp
Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, cuint start, cuint end, bool rm=false);
Abc_Ntk_t* aigSingleCone(Abc_Ntk_t *pNtk, cuint n, bool rm=false);
Abc_Ntk_t* aigPermCi(Abc_Ntk_t *pNtk, int *perm, bool rm=false);
void aigPermCo(Abc_Ntk_t *pNtk, int *perm);
Abc_Ntk_t* aigConcat(Abc_Ntk_t **pNtks, cuint nNtks, bool rm=false);
Abc_Ntk_t* aigMiter(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, const bool fCompl, bool rm = false);
Abc_Ntk_t* aigCreateDummyState();
Abc_Ntk_t* aigToComb(Abc_Ntk_t *pNtk, cuint mult = 1, bool rm = false);
Abc_Ntk_t* aigInitNtk(cuint nPi, cuint nPo, cuint nLatch, const char *name = NULL);

Abc_Obj_t* aigDot(Abc_Ntk_t* pNtk, Abc_Obj_t** v1, Abc_Obj_t** v2, cuint len);
Abc_Obj_t* aigBitsToCube(Abc_Ntk_t *pNtk, cuint n, Abc_Obj_t **pVars, cuint nVars);
Abc_Obj_t* aigIthVar(Abc_Ntk_t *pNtk, cuint i);
Abc_Obj_t** aigComputeSign(Abc_Ntk_t *pNtk, cuint range, bool fAddPo = true);
Abc_Obj_t* aigNewLatch(Abc_Ntk_t *pNtk, cuint initVal, char *latchName = NULL, char *inName = NULL, char *outName = NULL);


void aigRemovePo(Abc_Ntk_t* pNtk, cuint idx);
} // end namespace aigUtils

// deprecated!!
namespace fileWrite
{
// fileWrite.cpp
void writePerm(int *perm, cuint n, ostream& fp, bool isPi = true);
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

class STG
{
public:
    STG(cuint nci, cuint nco, cuint npi, cuint npo, cuint ntf): nCi(nci), nCo(nco), nPi(npi), nPo(npo), nTimeFrame(ntf) {}
    void addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint i, cuint cCnt, cuint nCnt);
    void write(ostream& fp, int *iPerm = NULL, int *oPerm = NULL);
    void setNSts(cuint nsts) { nSts = nsts; }
    uint getNCo() { return nCo; }
    uint getNPo() { return nPo; }
private:
    void writePerm(int *perm, ostream& fp, bool isPi);

    vector<string> transs;
    uint nCi, nCo, nPi, nPo, nTimeFrame, nSts;
}; // end class STG

} // end namespace utils

#endif /* __UTILS_H__ */