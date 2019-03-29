#include "base/main/main.h"
#include "base/main/mainInt.h"
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

int tFold_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    int i;
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