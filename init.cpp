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
Abc_Obj_t** computeSign(Abc_Ntk_t *pNtk, const size_t& range)
{
    // introduce variable alpha(a)
    // #a = log2(ceil(range))
    // a = [ a_n-1, a_n-2 , ... , a_1, a_0 ]
    size_t initVarSize = Abc_NtkCiNum(pNtk);
    size_t na = range ? (size_t)ceil(log2(double(range))) : 0;
    //Abc_Obj_t **a = new Abc_Obj_t*[na];
    for(size_t j=0; j<na; ++j) Abc_NtkCreatePi(pNtk);
    assert(initVarSize + na == Abc_NtkCiNum(pNtk));
    
    // compute signature for each number within range
    Abc_Obj_t **A = new Abc_Obj_t*[range];
    Abc_Obj_t *cube, *var;
    for(size_t j=0; j<range; ++j) {
        cube = Abc_AigConst1(pNtk);
        size_t k = j;
        for(size_t m=0; m<na; ++m) {
            var = Abc_NtkCi(pNtk, initVarSize+m);
            if(k & 1)
                cube = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, cube, var);
            else
                cube = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, cube, Abc_ObjNot(var));
            k >>= 1;
        }
        assert(k == 0);
        
        A[j] = cube;
    }
    
    return A;
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
    
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtk);
    size_t nB = nPo ? (size_t)ceil(log2(double(nPo*2))) : 0;
    Abc_Obj_t **B = computeSign(pNtkDup, nPo*2);
    
    /* debug: AND signature to every PO
    Abc_Obj_t *pAnd, *pCo;
    for(size_t t=0; t<nTimeframe; ++t) for(size_t p=0; p<nPo; ++p) {
        //cout << t << "_" << p << endl;
        pCo = Abc_NtkCo(pNtkDup, nPo*t+p);
        pAnd = Abc_AigAnd((Abc_Aig_t*)pNtkDup->pManFunc, Abc_ObjChild0(pCo), B[p]);
        Abc_ObjRemoveFanins(pCo);
        Abc_ObjAddFanin(pCo, pAnd);
    }
    */
    
    
    
    
    
    
    //Abc_NtkDelete(pNtkDup);
    //Abc_NtkDelete(pNtk);
    Abc_AigCleanup((Abc_Aig_t*)pNtkDup->pManFunc);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkDup);
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