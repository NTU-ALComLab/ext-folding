extern "C"
{
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "sat/cnf/cnf.h"
Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t *pNtk, int fExors, int fRegisters );
}

#include "ext-folding/utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iomanip>

using namespace std;

namespace
{
Abc_Obj_t* bitsToCube(Abc_Ntk_t *pNtk, size_t n, Abc_Obj_t **pVars, const size_t& nVars)
{
    assert((1<<nVars) >= n);
    Abc_Obj_t *var, *cube = Abc_AigConst1(pNtk);
    
    for(size_t i=0; i<nVars; ++i) {
        var = pVars[i];
        if(!(n & 1)) var = Abc_ObjNot(var);
        cube = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, cube, var);
        n >>= 1;
    }
    assert(n == 0);
    
    return cube;
}

Abc_Obj_t** computeSign(Abc_Ntk_t *pNtk, const size_t& range, const bool& fAddPo = true)
{
    // introduce variable alpha(a)
    // #a = log2(ceil(range))
    // a = [ a_n-1, a_n-2 , ... , a_1, a_0 ]
    size_t initVarSize = Abc_NtkCiNum(pNtk);
    size_t na = range ? (size_t)ceil(log2(double(range))) : 0;
    //Abc_Obj_t **a = new Abc_Obj_t*[na];
    
    char buf[1000];
    for(size_t j=0; j<na; ++j) {
        sprintf(buf, "a_%d", j);
        Abc_Obj_t *pObj = Abc_NtkCreatePi(pNtk);
        Abc_ObjAssignName(pObj, buf, NULL);
    }
    assert(initVarSize + na == Abc_NtkCiNum(pNtk));
    
    // compute signature for each number within range
    Abc_Obj_t **A = new Abc_Obj_t*[range];
    Abc_Obj_t *cube, *var;
    for(size_t j=0; j<range; ++j) {
        // TODO: change this part to bitsToCube??
        cube = Abc_AigConst1(pNtk);
        size_t k = j;
        for(size_t m=0; m<na; ++m) {
            var = Abc_NtkCi(pNtk, initVarSize+m);
            if(!(k & 1)) var = Abc_ObjNot(var);
            cube = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, cube, var);
            k >>= 1;
        }
        assert(k == 0);
        
        A[j] = cube;
        
        if(fAddPo) {
            sprintf(buf, "A_%d", j);
            var = Abc_NtkCreatePo(pNtk);
            Abc_ObjAssignName(var, buf, NULL);
            Abc_ObjAddFanin(var, cube);
        }
    }
    
    return A;
}

Abc_Obj_t* aigDot(Abc_Ntk_t* pNtk, Abc_Obj_t** v1, Abc_Obj_t** v2, const size_t& len)
{
    Abc_Obj_t *ret = Abc_ObjNot(Abc_AigConst1(pNtk));
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtk->pManFunc;
    for(size_t i=0; i<len; ++i)
        ret = Abc_AigOr(pMan, ret, Abc_AigAnd(pMan, v1[i], v2[i]));
    return ret;
}

void rmNtkPo(Abc_Ntk_t* pNtk, size_t idx)
{
    const size_t initPoNum = Abc_NtkPoNum(pNtk);
    assert(idx < initPoNum);
    Abc_Obj_t *pObj = Abc_NtkPo(pNtk, idx);
    Abc_ObjRemoveFanins(pObj);
    Abc_ObjAddFanin(pObj, Abc_AigConst1(pNtk));
    Abc_NtkRemovePo(pNtk, idx, 0);
    //Abc_NtkDeleteObjPo(Abc_NtkPo(pNtk, 3));
    assert(initPoNum+1 == Abc_NtkPoNum(pNtk));
}

Abc_Obj_t* getNtkIthVar(Abc_Ntk_t *pNtk, const size_t i)
{
    if(i < Abc_NtkPiNum(pNtk)) return Abc_NtkPi(pNtk, i);
    char buf[1000];
    sprintf(buf, "new_%d", i);
    Abc_Obj_t *pObj = Abc_NtkCreatePi(pNtk);
    Abc_ObjAssignName(pObj, buf, NULL);
    return pObj;
}

Abc_Ntk_t* createDummyState()
{
    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = "states";
    Abc_Obj_t *pObj = Abc_NtkCreatePo(pNtk);
    Abc_ObjAssignName(pObj, "S0", NULL);
    Abc_ObjAddFanin(pObj, Abc_AigConst1(pNtk));
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    assert(Abc_NtkCheck(pNtk));
    return pNtk;
}

void showBdd(Abc_Ntk_t *pNtk, const string& name)
{
    if(!Abc_NtkIsStrash(pNtk)) pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, 10000000, 1, 0, 0, 0);
    const size_t n = Abc_NtkPoNum(pNtk);
    DdNode **nv = new DdNode*[n];
    int i;  Abc_Obj_t *pObj;
    Abc_NtkForEachCo(pNtk, pObj, i)
        nv[i] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
    showBdd(dd, nv, n, name);
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);
    delete [] nv;
}

bool getNextMinterm(Abc_Ntk_t *pNtk, vector<bool>& minterm)
{
    const size_t nPo = Abc_NtkPoNum(pNtk);
    cout << nPo << " states" << endl;
    Aig_Man_t *pMan = Abc_NtkToDar(pNtk, 0, 0 );
    Cnf_Dat_t *pCnf = Cnf_Derive(pMan, nPo);
    sat_solver_t *pSat = sat_solver_new();
    sat_solver_setnvars(pSat, pCnf->nVars);
    for(int i=0; i<pCnf->nClauses; ++i)
        assert(sat_solver_addclause(pSat, pCnf->pClauses[i], pCnf->pClauses[i+1]));
    
    lit *ua = new lit[nPo];
    for(size_t i=0; i<nPo; ++i) {
        Aig_Obj_t *pObj = Aig_ManCo(pMan, i);
        ua[i] = toLitCond(pCnf->pVarNums[Aig_ObjId(pObj)], 1); //???????
    }
    
    cout << "mt: ";
    int r = sat_solver_solve(pSat, ua, ua+nPo, 0, 0, 0, 0);
    if(r == l_True) for(size_t i=0; i<minterm.size(); ++i) {
        int var = pCnf->pVarNums[Aig_ObjId(Aig_ManCi(pMan, i))];
        minterm[i] = (bool)sat_solver_var_value(pSat, var);
        cout << minterm[i];
    }
    cout << endl;
    
    // release memory
    sat_solver_delete(pSat);
    Cnf_DataFree(pCnf);
    Aig_ManStop(pMan);
    delete [] ua;
    
    if(r == l_True)
        cout << "SAT!!" << endl;
    
    return r == l_True;
}

Abc_Ntk_t* getStates(Abc_Ntk_t *pNtk, const size_t level)
{
    assert(Abc_NtkPoNum(pNtk) == 1);
    int i;  char buf[1000];
    Abc_Obj_t *tmp1, *tmp2;
    vector<bool> minterm(level, true);
    bool next = true;
    size_t iter = 0;
    Abc_Ntk_t *pNtkDup, *pNtkMiter;
    Abc_Ntk_t *sts = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    sts->pName = "states";
    
    Abc_NtkForEachCi(pNtk, tmp1, i)
        Abc_ObjAssignName(Abc_NtkCreatePi(sts), Abc_ObjName(tmp1), NULL);
    
    cout << "start!!" << endl;
    showBdd(pNtk, "qdump/orig");
    while(next) {
        // build miter
        pNtkDup = Abc_NtkDup(pNtk);
        pNtkDup = Abc_NtkToLogic(pNtkDup);
        for(i=0; i<minterm.size(); ++i)
            Abc_ObjReplaceByConstant(Abc_NtkPi(pNtkDup, i), (int)minterm[i]);
        showBdd(pNtkDup, "qdump/c"+to_string(iter));
        pNtkMiter = Abc_NtkMiter(pNtk, pNtkDup, 0, 0, 0, 0);
        showBdd(pNtkMiter, "qdump/n"+to_string(iter));
        tmp1 = Abc_NtkPo(pNtkMiter, 0);
        tmp2 = Abc_ObjChild0(tmp1);
        Abc_ObjRemoveFanins(tmp1);
        Abc_ObjAddFanin(tmp1, Abc_ObjNot(tmp2));
        assert(Abc_NtkPoNum(pNtkMiter) == 1);
        Abc_NtkDelete(pNtkDup);
        showBdd(pNtkMiter, "qdump/m"+to_string(iter));
        
        // univ quant
        for(i=level; i<Abc_NtkPiNum(pNtkMiter); ++i) {
            assert(Abc_NtkQuantify(pNtkMiter, 1, i, 0));
            showBdd(pNtkMiter, "qdump/q"+to_string(iter)+"_"+to_string(i));
        }
        Abc_AigCleanup((Abc_Aig_t*)pNtkMiter->pManFunc);
        showBdd(pNtkMiter, "qdump/q"+to_string(iter));
        
        
        // add new state
        assert(Abc_NtkPiNum(sts) == Abc_NtkPiNum(pNtkMiter));
        Abc_AigConst1(pNtkMiter)->pCopy = Abc_AigConst1(sts);
        for(i=0; i<Abc_NtkPiNum(sts); ++i)
            Abc_NtkPi(pNtkMiter, i)->pCopy = Abc_NtkPi(sts, i);
        Abc_AigForEachAnd(pNtkMiter, tmp1, i)
            tmp1->pCopy = Abc_AigAnd((Abc_Aig_t*)sts->pManFunc, 
                                 Abc_ObjChild0Copy(tmp1), 
                                 Abc_ObjChild1Copy(tmp1));
        tmp1 = Abc_NtkCreatePo(sts);
        sprintf(buf, "S%d", iter);
        Abc_ObjAssignName(tmp1, buf, NULL);
        Abc_ObjAddFanin(tmp1, Abc_ObjChild0Copy(Abc_NtkPo(pNtkMiter, 0)));
        Abc_AigCleanup((Abc_Aig_t*)sts->pManFunc);
        
        showBdd(sts, "qdump/i"+to_string(iter));
        
        // get next minterm
        next = getNextMinterm(sts, minterm);
        
        cout << "iter " << iter << endl;
        Abc_NtkDelete(pNtkMiter);
        assert(Abc_NtkPoNum(sts) == ++iter);
        
        
        if(iter == 3) exit(1);
    }
    
    Abc_AigCleanup((Abc_Aig_t*)sts->pManFunc);
    assert(Abc_NtkCheck(sts));
    return sts;
}

Abc_Ntk_t* getPartNtk(Abc_Ntk_t *pNtk, const size_t start, const size_t end)
{
    assert(start < end);
    assert(start < Abc_NtkPoNum(pNtk));
    assert(end <= Abc_NtkPoNum(pNtk));
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    int i;  Abc_Obj_t *pObj, *pObjNew;
    
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtkNew->pName = "part";
    
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachCi(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    
    for(i=start; i<end; ++i) {
        pObj = Abc_NtkPo(pNtk, i);
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    
    Abc_AigForEachAnd(pNtk, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    for(i=start; i<end; ++i) {
        pObj = Abc_NtkPo(pNtk, i);
        pObjNew = Abc_ObjChild0Copy(pObj);
        Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, i-start), pObjNew);
    }
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));
    
    return pNtkNew;
}

Abc_Ntk_t* concat(Abc_Ntk_t **pNtks, const size_t& nNtks)
{
    if(!nNtks || !pNtks) return NULL;
    
    int i;  char charBuf[1000]; string strBuf;
    Abc_Ntk_t *pNtkNew;
    Abc_Obj_t *pObj, *pObjNew;
    
    for(i=0; i<nNtks; ++i) {
        assert(Abc_NtkIsStrash(pNtks[i]));
        assert(Abc_NtkIsDfsOrdered(pNtks[i]));
    }
    
    // start the new network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    
    for(i=0; i<nNtks; ++i)
        strBuf += string(pNtks[i]->pName) + string("_");
    sprintf(charBuf, strBuf.c_str(), "concat");
    pNtkNew->pName = Extra_UtilStrsav(charBuf);
    
    for(i=0; i<nNtks; ++i)
        Abc_AigConst1(pNtks[i])->pCopy = Abc_AigConst1(pNtkNew);
    
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachCi(pNtks[0], pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        for(size_t j=1; j<nNtks; ++j) {
            pObj = Abc_NtkCi(pNtks[j], i);  
            pObj->pCopy = pObjNew;
        }
        // add name
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    
    // create POs for each Ntk
    for(size_t j=0; j<nNtks; ++j) Abc_NtkForEachCo(pNtks[j], pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        pObj->pCopy = pObjNew;
        strBuf = string("concat") + to_string(j+1) + string("_");
        strcpy(charBuf, strBuf.c_str()); 
        Abc_ObjAssignName(pObjNew, charBuf, Abc_ObjName(pObj));
    }
    
    
    // AddOne for each Ntk
    for(size_t j=0; j<nNtks; ++j) Abc_AigForEachAnd(pNtks[j], pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    
    // collect POs for each Ntk
    for(size_t j=0; j<nNtks; ++j) Abc_NtkForEachCo(pNtks[j], pObj, i) {
        pObjNew = Abc_ObjChild0Copy(pObj);
        Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, i), pObjNew);
    }
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    
    if (!Abc_NtkCheck(pNtkNew))
    {
        printf( "concat: The network check has failed.\n" );
        Abc_NtkDelete(pNtkNew);
        return NULL;
    }
    
    return pNtkNew;
}

int tFold_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    int i;  Abc_Obj_t *tmp1, *tmp2;
    // usage: timefold <nTimeframe> <kissFile>
    // get args
    assert(argc == 3);
    const size_t nTimeframe = atoi(argv[1]);
    const char *kissFn = argv[2];
    
    // get pNtk
    Abc_Ntk_t *pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL) {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if(!Abc_NtkIsStrash(pNtk)) pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    
    
    // get basic settings
    const size_t nCo = Abc_NtkCoNum(pNtk);         // total circuit output after time expansion
    const size_t nPo = nCo / nTimeframe;           // #POs of the original seq. ckt
    const size_t initVarSize = Abc_NtkCiNum(pNtk);
    const size_t nVar = initVarSize / nTimeframe;  // #variable for each t (#PI of the original seq. ckt)
    assert(nPo * nTimeframe == nCo);
    assert(initVarSize % nTimeframe == 0);
    
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtk);
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtkDup->pManFunc;
    Abc_ObjAssignName(Abc_NtkCreatePo(pNtkDup), "hyper", NULL);
    
    DdManager *dd = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS, 0);
    
    // add signature var
    const size_t nb = nPo ? (size_t)ceil(log2(double(nPo))) : 0;
    Abc_Obj_t **b = new Abc_Obj_t*[nb];
    for(i=0; i<nb; ++i)
        b[i] = getNtkIthVar(pNtkDup, initVarSize+i);
    assert(initVarSize+nb == Abc_NtkPiNum(pNtkDup));
    
    
    // collecting states at each timeframe
    vector<string> stg;
    size_t stsSum = 1;
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;  // dummies
    
    string spc = string(" "), uds = string("_"), sHead = string("S");
    string cstName, nstName;
    
    Abc_Ntk_t *csts, *nsts = createDummyState();
    size_t cCnt = 0, nCnt;
    
    
    DdNode *G, *btmp1, *btmp2;
    DdNode **oFuncs = new DdNode*[nPo];
    DdNode *path;
    
    for(i=nTimeframe-1; i>=0; --i) {
        vector<clock_t> tVec;
        
        if(i) {
            tVec.push_back(clock());  // t0
            
            // copy nsts into pNtkDup
            size_t j, nFuncs = Abc_NtkPoNum(nsts);
            Abc_Obj_t **pFuncs = new Abc_Obj_t*[nFuncs];
            Abc_AigConst1(nsts)->pCopy = Abc_AigConst1(pNtkDup);
            for(j=0; j<Abc_NtkPiNum(nsts); ++j)
                Abc_NtkPi(nsts, j)->pCopy = Abc_NtkPi(pNtkDup, j);
            j = 0;
            Abc_AigForEachAnd(nsts, tmp1, j)
                tmp1->pCopy = Abc_AigAnd(pMan, Abc_ObjChild0Copy(tmp1), Abc_ObjChild1Copy(tmp1));
            for(j=0; j<nFuncs; ++j)
                pFuncs[j] = Abc_ObjChild0Copy(Abc_NtkPo(nsts, j));            
            if(nFuncs == 1) assert(pFuncs[0] == Abc_AigConst1(pNtkDup));
            
            size_t na = (size_t)ceil(log2(double(nFuncs))) + 2;
            Abc_Obj_t **a = new Abc_Obj_t*[na];
            for(j=0; j<na; ++j) a[j] = getNtkIthVar(pNtkDup, initVarSize+nb+j);
            
            Abc_Obj_t *ft = Abc_ObjNot(Abc_AigConst1(pNtkDup));
            for(j=0; j<nPo; ++j) {
                tmp1 = Abc_ObjFanin0(Abc_NtkPo(pNtkDup, i*nPo+j));
                tmp2 = bitsToCube(pNtkDup, j, b, nb);
                
                ft = Abc_AigOr(pMan, ft, Abc_AigAnd(pMan, tmp1, tmp2));
            }
            ft = Abc_AigAnd(pMan, ft, Abc_ObjNot(a[0]));
            
            tmp1 = Abc_ObjNot(Abc_AigConst1(pNtkDup));
            for(j=0; j<nFuncs; ++j) {
                tmp2 = bitsToCube(pNtkDup, j, a+1, na-1);
                tmp1 = Abc_AigOr(pMan, tmp1, Abc_AigAnd(pMan, pFuncs[j], tmp2));
            }
            tmp1 = Abc_AigAnd(pMan, tmp1, a[0]);
            
            ft = Abc_AigOr(pMan, ft, tmp1);
            j = Abc_NtkPoNum(pNtkDup) - 1;
            Abc_ObjAddFanin(Abc_NtkPo(pNtkDup, j), ft);
            
            tVec.push_back(clock());  // t1
            
            tmp1 = Abc_NtkPo(pNtkDup, j);
            //csts = getStates(Abc_NtkCreateCone(pNtkDup, ft, "hyper", 1), nVar*i);
            csts = getStates(Abc_NtkCreateCone(pNtkDup, Abc_ObjFanin0(tmp1), Abc_ObjName(tmp1), 1), nVar*i);
            
            tVec.push_back(clock());  // t2
            
            delete [] a;
            delete [] pFuncs;
        }
        else csts = createDummyState();  // i==0
        
        if(tVec.empty()) for(size_t j=0; j<3; ++j) tVec.push_back(clock());
        
        stsSum += Abc_NtkPoNum(csts);
        cout << setw(7) << Abc_NtkPoNum(csts) << " states: ";
        
        if(Abc_NtkPiNum(nsts) < Abc_NtkPiNum(csts))
            for(size_t j=Abc_NtkPiNum(nsts); j<Abc_NtkPiNum(csts); ++j)
                assert(getNtkIthVar(nsts, j));
        Abc_Ntk_t *ntkToCat[3];
        ntkToCat[0] = getPartNtk(pNtkDup, i*nPo, (i+1)*nPo);
        ntkToCat[1] = csts;  ntkToCat[2] = nsts;
        Abc_Ntk_t *pNtkCat = concat(ntkToCat, 3);
        Abc_NtkDelete(ntkToCat[0]);
        
        for(size_t j=0; j<nPo; ++j) {
            char buf[100];  sprintf(buf, "PO%d", j);
            Abc_ObjAssignName(Abc_NtkCreatePo(pNtkCat), buf, NULL);
        }
        
        for(cCnt=0; cCnt<Abc_NtkPoNum(csts); ++cCnt) {
            cstName = sHead + to_string(i) + uds + to_string(cCnt);
            
            tmp1 = Abc_ObjFanin0(Abc_NtkPo(pNtkCat, nPo+cCnt));
            for(size_t j=0; j<nPo; ++j) {
                tmp2 = Abc_ObjFanin0(Abc_NtkPo(pNtkCat, j));
                tmp2 = Abc_AigAnd((Abc_Aig_t*)pNtkCat->pManFunc, tmp1, tmp2);
                Abc_Obj_t *po = Abc_NtkPo(pNtkCat, Abc_NtkPoNum(pNtkCat)-1-nPo+i);
                Abc_ObjRemoveFanins(po);
                Abc_ObjAddFanin(po, tmp2);
            }
            
            Abc_Ntk_t *pNtkOut = getPartNtk(pNtkCat, Abc_NtkPoNum(pNtkCat)-nPo, Abc_NtkPoNum(pNtkCat));
            for(size_t j=0; j<nVar*i; ++j)
                assert(Abc_NtkQuantify(pNtkOut, 0, j, 0));
            
            DdManager *_dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtkOut, 10000000, 1, 0, 0, 0);
            for(size_t j=0; j<nPo; ++j) {
                tmp1 = Abc_NtkPo(pNtkOut, j);
                oFuncs[j] = Cudd_bddTransfer(_dd, dd, (DdNode *)Abc_ObjGlobalBdd(tmp1));
                Cudd_Ref(oFuncs[j]);
            }
            
            Abc_NtkFreeGlobalBdds(pNtkOut, 0);
            Cudd_Quit(_dd);
            Abc_NtkDelete(pNtkOut);
            
            
            for(nCnt=0; cCnt<Abc_NtkPoNum(csts); ++cCnt) {
                
                if(i == nTimeframe-1) nstName = string("*");
                else nstName = sHead + to_string(i+1) + uds + to_string(nCnt);
                
                tmp1 = Abc_NtkPo(pNtkCat, nPo+cCnt);
                tmp2 = Abc_NtkPo(pNtkCat, nPo+Abc_NtkPoNum(csts)+nCnt);
                tmp1 = Abc_AigAnd((Abc_Aig_t*)pNtkCat->pManFunc, tmp1, tmp2);
                tmp2 = Abc_NtkPo(pNtkCat, Abc_NtkPoNum(pNtkCat)-1);
                Abc_ObjRemoveFanins(tmp2);
                Abc_ObjAddFanin(tmp2, tmp1);
                
                pNtkOut = getPartNtk(pNtkCat, Abc_NtkPoNum(pNtkCat)-1, Abc_NtkPoNum(pNtkCat));
                for(size_t j=0; j<nVar*i; ++j)
                    assert(Abc_NtkQuantify(pNtkOut, 0, j, 0));
                
                _dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtkOut, 10000000, 1, 0, 0, 0);
                path = Cudd_bddTransfer(_dd, dd, (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtkOut, 0)));
                Cudd_Ref(path);
                
                Abc_NtkFreeGlobalBdds(pNtkOut, 0);
                Cudd_Quit(_dd);
                Abc_NtkDelete(pNtkOut);
                
                
                if(path != b0) {  // transition exist
                    
                    DdNode **a = new DdNode*[nb+1];
                    for(size_t j=0; j<nb+1; ++j)
                        a[j] = Cudd_bddIthVar(dd, (i+1)*nVar+j);
                    
                    G = b0;  Cudd_Ref(G);
                    
                    
                    for(size_t k=0; k<2; ++k) {
                        bddNotVec(oFuncs, nPo);   // 2*negation overall
                        btmp1 = Extra_bddEncodingBinary(dd, oFuncs, nPo, a+1, nb);  Cudd_Ref(btmp1);
                        btmp2 = Cudd_bddAnd(dd, btmp1, k ? a[0] : Cudd_Not(a[0]));  Cudd_Ref(btmp2);
                        Cudd_RecursiveDeref(dd, btmp1);
                        
                        btmp1 = Cudd_bddOr(dd, G, btmp2);  Cudd_Ref(btmp1);
                        Cudd_RecursiveDeref(dd, G);
                        G = btmp1;
                    }
                    
                    btmp1 = Cudd_bddAnd(dd, G, path);  Cudd_Ref(btmp1);
                    Cudd_RecursiveDeref(dd, G);
                    G = btmp1;
                    
                    while(G != b0) {
                        gen = Cudd_FirstCube(dd, G, &cube, &val);
                        
                        string trans("");
                            
                        // input bits
                        size_t vStart = i * nVar;
                        size_t vEnd = (i+1) * nVar;
                        for(size_t m=0; m<Cudd_ReadSize(dd); ++m) {
                            if((m>=vStart) && (m<vEnd))
                                trans += (cube[m]==2) ? string("-") : to_string(cube[m]);
                            else
                                cube[m] = 2;
                        }
                        
                        // states
                        trans += spc + cstName + spc + nstName + spc;
                        
                        // output bits
                        btmp1 = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(btmp1);
                        for(size_t k=0; k<nPo; ++k) {
                            btmp2 = Cudd_Cofactor(dd, oFuncs[k], btmp1);  Cudd_Ref(btmp2);
                            if(btmp2 == b0) trans += "0";
                            else trans += "1";
                            Cudd_RecursiveDeref(dd, btmp2);
                        }
                        
                        stg.push_back(trans);
                        
                        btmp2 = Cudd_bddAnd(dd, G, Cudd_Not(btmp1));  Cudd_Ref(btmp2);
                        
                        Cudd_RecursiveDeref(dd, btmp1);
                        Cudd_RecursiveDeref(dd, G);
                        
                        G = btmp2;
                        
                        Cudd_GenFree(gen);
                    }
                    
                    Cudd_RecursiveDeref(dd, G);
                    delete [] a;
                }
                
                
                Cudd_RecursiveDeref(dd, path);
            }
            
            bddDerefVec(dd, oFuncs, nPo);
        }
        
        
        tVec.push_back(clock()); // t3
        
        Abc_AigCleanup((Abc_Aig_t*)pNtkDup->pManFunc);
        Abc_NtkDelete(pNtkCat);
        Abc_NtkDelete(nsts);
        csts = nsts;
        
        for(size_t j=1; j<tVec.size(); ++j)
            cout << setw(7) << double(tVec[j]-tVec[j-1])/CLOCKS_PER_SEC << " ";
        
    }
    
    // write kiss
    ofstream fp;
    fp.open(kissFn);
    fp << ".i " << nVar << "\n";
    fp << ".o " << nPo << "\n";
    fp << ".p " << stg.size() << "\n";
    fp << ".s " << stsSum << "\n";
    fp << ".r " << "S0_0" << "\n";
    for(string trans: stg) fp << trans << "\n";
    fp << ".e\n";
    fp.close();
    
    delete [] oFuncs;
    delete [] b;
    Cudd_Quit(dd);
    Abc_NtkDelete(csts);
    Abc_NtkDelete(pNtkDup);
    
    return 0;
    

    /* debug: AND signature to every PO
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtk);
    size_t nB = nPo ? (size_t)ceil(log2(double(nPo*2))) : 0;
    Abc_Obj_t **B = computeSign(pNtkDup, nPo*2);
    
    Abc_Obj_t *pAnd, *pCo;
    for(size_t t=0; t<nTimeframe; ++t) for(size_t p=0; p<nPo; ++p) {
        //cout << t << "_" << p << endl;
        pCo = Abc_NtkCo(pNtkDup, nPo*t+p);
        pAnd = Abc_AigAnd((Abc_Aig_t*)pNtkDup->pManFunc, Abc_ObjChild0(pCo), B[p]);
        Abc_ObjRemoveFanins(pCo);
        Abc_ObjAddFanin(pCo, pAnd);
    }
    */

    /* here!!
    // init new pNtk
    Abc_Obj_t *pObj, *pObjNew;
    Abc_Ntk_t *pNtkUni = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkUni);
    
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachCi(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkUni);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    
    // create a new PO
    //Abc_NtkCreatePo(pNtkUni);
    
    // AddOne for each Ntk
    Abc_AigForEachAnd(pNtk, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkUni->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
    
    
    // compute signature
    //size_t nB = nPo ? (size_t)ceil(log2(double(nPo))) : 0;
    Abc_Obj_t **B = computeSign(pNtkUni, nPo, false);
    Abc_Obj_t **A = new Abc_Obj_t*[nPo];
    for(i=0; i<nPo; ++i)
        A[i] = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, nPo*2+i));
    pObj = aigDot(pNtkUni, A, B, nPo);
    Abc_ObjAddFanin(Abc_NtkCreatePo(pNtkUni), pObj);
    //Abc_ObjAddFanin(Abc_NtkCreatePo(pNtkUni), A[0]);
    Abc_ObjAssignName(Abc_NtkPo(pNtkUni, 0), "yo", NULL);
    delete [] B;
    delete [] A;
    
    //Abc_AigCleanup((Abc_Aig_t*)pNtkUni->pManFunc);
    assert(Abc_NtkCheck(pNtkUni));
    
    //Abc_NtkDelete(pNtkDup);
    //Abc_NtkDelete(pNtk);
    //Abc_AigCleanup((Abc_Aig_t*)pNtkDup->pManFunc);
    //Abc_FrameReplaceCurrentNetwork(pAbc, pNtkDup);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkUni);
    cout << "done..." << endl;
    return 0;
    */
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "tFold", "timefold", tFold_Command, 0);
}

// called during ABC termination
void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer);
    }
} fold_registrar;

} // unnamed namespace