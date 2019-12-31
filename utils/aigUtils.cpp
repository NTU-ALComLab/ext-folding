#include "ext-folding/utils/utils.h"

extern "C"
{
void Abc_NtkRemovePo(Abc_Ntk_t *pNtk, int iOutput, int fRemoveConst0);
int Abc_NtkQuantify(Abc_Ntk_t *pNtk, int fUniv, int iVar, int fVerbose);
}

namespace utils::aigUtils
{

// takes the cone of POs from indices [start, end)
// the newly-formed subcircuit will keep all PIs
Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, cuint start, cuint end, bool rm)
{
    assert(Abc_NtkIsStrash(pNtk));
    assert(start < end);
    assert(start < Abc_NtkPoNum(pNtk));
    assert(end <= Abc_NtkPoNum(pNtk));

    // single PO cone
    if(start+1 == end) return aigSingleCone(pNtk, start, rm);

    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    int i;  Abc_Obj_t *pObj, *pObjNew;
    
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    
    char buf[100];
    sprintf(buf, "%s_cone%u-%u", pNtk->pName, start, end);
    pNtkNew->pName = Extra_UtilStrsav(buf);
    
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachCi(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    
    for(i=start; i<end; ++i) {
        pObj = Abc_NtkPo(pNtk, i);
        pObjNew = Abc_NtkCreatePo(pNtkNew);
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

    if(rm) Abc_NtkDelete(pNtk);
    
    return pNtkNew;
}

// takes the cone of single PO, with more efficient circuit construction
Abc_Ntk_t* aigSingleCone(Abc_Ntk_t *pNtk, cuint n, bool rm)
{
    assert(Abc_NtkIsStrash(pNtk));
    assert(n < Abc_NtkPoNum(pNtk));
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);

    Abc_Obj_t *pNodeCo = Abc_NtkCo(pNtk, n);
    Abc_Ntk_t *pNtkRes = Abc_NtkCreateCone(pNtk, Abc_ObjFanin0(pNodeCo), Abc_ObjName(pNodeCo), 1);
    if(Abc_ObjFaninC0(pNodeCo))
        Abc_NtkPo(pNtkRes, 0)->fCompl0 ^= 1;
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    if(rm) Abc_NtkDelete(pNtk);

    return pNtkRes;
}

// permutates the order of CIs with the given "perm"
Abc_Ntk_t* aigPermCi(Abc_Ntk_t *pNtk, int *perm, bool rm)
{
    assert(Abc_NtkIsStrash(pNtk));

    int i;  char buf[100];
    Abc_Obj_t *pObj, *pObjNew;
    Abc_Ntk_t *pNtkPerm = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    
    sprintf(buf, "%s_perm", pNtk->pName);
    pNtkPerm->pName = Extra_UtilStrsav(buf);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkPerm);
    for(i=0; i<Abc_NtkCiNum(pNtk); ++i) {
        pObjNew = Abc_NtkCreatePi(pNtkPerm);
        pObj = Abc_NtkCi(pNtk, perm[i]);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    Abc_AigForEachAnd(pNtk, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkPerm->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
    Abc_NtkForEachCo(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkPerm);
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkPerm->pManFunc);
    assert(Abc_NtkCheck(pNtkPerm));
    
    if(rm) Abc_NtkDelete(pNtk);

    return pNtkPerm;
}

// permutates the order of COs with the given "perm"
// in-place permutation, no new AIG will be created
void aigPermCo(Abc_Ntk_t *pNtk, int *perm)
{
    assert(Abc_NtkIsStrash(pNtk));

    cuint nCo = Abc_NtkCoNum(pNtk);
    Abc_Obj_t **poVec = new Abc_Obj_t*[nCo];
    char **nVec = new char*[nCo];
    uint i;  Abc_Obj_t *pObj;

    //Abc_NtkCollect
    Abc_NtkForEachCo(pNtk, pObj, i) {
        poVec[i] = Abc_ObjChild0(pObj);
        nVec[i] = Extra_UtilStrsav(Abc_ObjName(pObj));
        Nm_ManDeleteIdName(pObj->pNtk->pManName, pObj->Id);
        Abc_ObjRemoveFanins(pObj);
    }
    assert(i == nCo);

    Abc_NtkForEachCo(pNtk, pObj, i) {
        Abc_ObjAddFanin(pObj, poVec[perm[i]]);
        Abc_ObjAssignName(pObj, nVec[i], NULL);
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    assert(Abc_NtkCheck(pNtk));
    delete [] poVec;

    for(uint i=0; i<nCo; ++i) ABC_FREE(nVec[i]);
    delete [] nVec;
}

// concatenates the given array of circuits, each circuit should have the same #CIs
// side effects: for sequential circuit, FFs will be converted to PI/POs
Abc_Ntk_t* aigConcat(Abc_Ntk_t **pNtks, cuint nNtks, bool rm)
{
    int i;  char charBuf[1000];
    Abc_Ntk_t *pNtkNew;
    Abc_Obj_t *pObj, *pObjNew;
    
    if(!nNtks || !pNtks) return NULL;
    for(i=0; i<nNtks; ++i) {
        assert(Abc_NtkCheck(pNtks[i]));
        assert(Abc_NtkIsStrash(pNtks[i]));
        assert(Abc_NtkIsDfsOrdered(pNtks[i]));
    }
    
    // start the new network
    pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    
    sprintf(charBuf, "%u_concat", nNtks);
    pNtkNew->pName = Extra_UtilStrsav(charBuf);
    
    for(i=0; i<nNtks; ++i)
        Abc_AigConst1(pNtks[i])->pCopy = Abc_AigConst1(pNtkNew);
    
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachCi(pNtks[0], pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        for(uint j=1; j<nNtks; ++j) {
            pObj = Abc_NtkCi(pNtks[j], i);  
            pObj->pCopy = pObjNew;
        }
        // add name
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    
    // AddOne for each Ntk
    for(uint j=0; j<nNtks; ++j) Abc_AigForEachAnd(pNtks[j], pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    // create and collect POs for each Ntk
    for(uint j=0; j<nNtks; ++j) Abc_NtkForEachCo(pNtks[j], pObj, i) {
        pObjNew = Abc_ObjChild0Copy(pObj);
        pObj = Abc_NtkCreatePo(pNtkNew);
        Abc_ObjAddFanin(pObj, pObjNew);
        
        sprintf(charBuf, "cc%u", j);
        Abc_ObjAssignName(pObj, charBuf, Abc_ObjName(pObj));
    }
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));

    if(rm) for(uint j=0; j<nNtks; ++j) Abc_NtkDelete(pNtks[j]);
    
    return pNtkNew;
}

// computes inner product(dot) of 2 given arrays of AIG nodes
Abc_Obj_t* aigDot(Abc_Ntk_t* pNtk, Abc_Obj_t** v1, Abc_Obj_t** v2, cuint len)
{
    assert(Abc_NtkIsStrash(pNtk));
    Abc_Obj_t *ret = Abc_ObjNot(Abc_AigConst1(pNtk));
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtk->pManFunc;
    for(uint i=0; i<len; ++i)
        ret = Abc_AigOr(pMan, ret, Abc_AigAnd(pMan, v1[i], v2[i]));
    return ret;
}

// constructs the miter of the 2 given circuits
Abc_Ntk_t* aigMiter(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, const bool fCompl, bool rm)
{
    assert(Abc_NtkIsStrash(pNtk1) && Abc_NtkIsStrash(pNtk2));
    assert(Abc_NtkPoNum(pNtk1) == Abc_NtkPoNum(pNtk1));
    assert(Abc_NtkPiNum(pNtk1) == Abc_NtkPiNum(pNtk1));
    assert(Abc_NtkPoNum(pNtk1) == 1);
    
    int i;
    Abc_Ntk_t *pNtkNew;
    Abc_Obj_t *pObj, *pObjNew;
    
    // start the new network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkNew->pName = Extra_UtilStrsav("miter");
    
    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkNew);
    
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachCi(pNtk1, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        pObj = Abc_NtkCi(pNtk2, i);  
        pObj->pCopy = pObjNew;
        // add name
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL );
    }
    
    assert(Abc_NtkIsDfsOrdered(pNtk1));
    assert(Abc_NtkIsDfsOrdered(pNtk2));
    
    // AddOne for Ntk1
    Abc_AigForEachAnd(pNtk1, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
    // AddOne for Ntk2                          
    Abc_AigForEachAnd(pNtk2, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
    
    pObjNew = Abc_AigXor((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(Abc_NtkPo(pNtk1, 0)), Abc_ObjChild0Copy(Abc_NtkPo(pNtk2, 0)));
    if(fCompl) pObjNew = Abc_ObjNot(pObjNew);
    
    pObj = Abc_NtkCreatePo(pNtkNew);
    Abc_ObjAssignName(pObj, "MO", NULL);
    Abc_ObjAddFanin(pObj, pObjNew);
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));
    
    if(rm) {
        Abc_NtkDelete(pNtk1);
        Abc_NtkDelete(pNtk2);
    }

    return pNtkNew;
}

// computes the cube of AIG variables corresponding to bits it the bit-code
Abc_Obj_t* aigBitsToCube(Abc_Ntk_t *pNtk, cuint n, Abc_Obj_t **pVars, cuint nVars)
{
    assert(Abc_NtkIsStrash(pNtk));
    assert((1<<nVars) >= n);

    uint k = n;
    Abc_Obj_t *var, *cube = Abc_AigConst1(pNtk);
    for(uint i=0; i<nVars; ++i) {
        var = Abc_ObjNotCond(pVars[i], !(k & 1));
        cube = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, cube, var);
        k >>= 1;
    }
    assert(k == 0);
    
    return cube;
}

// returns the i^th variable(Pi) of current AIG network if it already exists, 
// or creates a new AIG variable.
Abc_Obj_t* aigIthVar(Abc_Ntk_t *pNtk, cuint i)
{
    assert(Abc_NtkIsStrash(pNtk));

    char buf[1000];
    for(uint x=Abc_NtkPiNum(pNtk); x<=i; ++x) {
        sprintf(buf, "new_%u", x);
        Abc_ObjAssignName(Abc_NtkCreatePi(pNtk), buf, NULL);
    }

    return Abc_NtkPi(pNtk, i);
}

// removes the idx^th Po from the current AIG network
void aigRemovePo(Abc_Ntk_t* pNtk, cuint idx)
{
    assert(Abc_NtkIsStrash(pNtk));
    cuint initPoNum = Abc_NtkPoNum(pNtk);
    assert(idx < initPoNum);

    Abc_Obj_t *pObj = Abc_NtkPo(pNtk, idx);
    Abc_ObjRemoveFanins(pObj);
    Abc_ObjAddFanin(pObj, Abc_AigConst1(pNtk));
    Abc_NtkRemovePo(pNtk, idx, 0);
    //Abc_NtkDeleteObjPo(Abc_NtkPo(pNtk, 3));
    
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    assert(Abc_NtkCheck(pNtk));
    assert(initPoNum == Abc_NtkPoNum(pNtk)+1);
}

Abc_Ntk_t* aigCreateDummyState()
{
    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = Extra_UtilStrsav("states");
    Abc_Obj_t *pObj = Abc_NtkCreatePo(pNtk);
    Abc_ObjAssignName(pObj, "S0", NULL);
    Abc_ObjAddFanin(pObj, Abc_AigConst1(pNtk));
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    assert(Abc_NtkCheck(pNtk));
    return pNtk;
}

// computes signatures for hyper-function encoding
Abc_Obj_t** aigComputeSign(Abc_Ntk_t *pNtk, cuint range, bool fAddPo)
{
    // introduce variables: alpha(a)
    // #a = log2(ceil(range))
    // a = [ a_n-1, a_n-2 , ... , a_1, a_0 ]
    uint initVarSize = Abc_NtkCiNum(pNtk);
    uint na = range ? (uint)ceil(log2(double(range))) : 0;
    
    char buf[1000];
    for(uint j=0; j<na; ++j) {
        sprintf(buf, "a_%u", j);
        Abc_Obj_t *pObj = Abc_NtkCreatePi(pNtk);
        Abc_ObjAssignName(pObj, buf, NULL);
    }
    assert(initVarSize + na == Abc_NtkCiNum(pNtk));
    
    // compute signature for each number within range
    Abc_Obj_t **A = new Abc_Obj_t*[range];
    Abc_Obj_t *cube, *var;
    for(uint j=0; j<range; ++j) {
        // TODO: change this part to bitsToCube??
        cube = Abc_AigConst1(pNtk);
        uint k = j;
        for(uint m=0; m<na; ++m) {
            var = Abc_NtkCi(pNtk, initVarSize+m);
            if(!(k & 1)) var = Abc_ObjNot(var);
            cube = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, cube, var);
            k >>= 1;
        }
        assert(k == 0);
        
        A[j] = cube;
        if(fAddPo) {
            sprintf(buf, "A_%u", j);
            var = Abc_NtkCreatePo(pNtk);
            Abc_ObjAssignName(var, buf, NULL);
            Abc_ObjAddFanin(var, cube);
        }
    }
    return A;
}

// 1. make the network purely combinational by converting latches to PI/POs
// 2. introduce dummy PIs to make #PI be the multiples of "mult"
Abc_Ntk_t* aigToComb(Abc_Ntk_t *pNtk, cuint mult, bool rm)
{
    bool fDel = !Abc_NtkIsStrash(pNtk);
    Abc_Ntk_t *pNtkDup = fDel ? Abc_NtkStrash(pNtk, 0, 0, 0) : pNtk;
    Abc_AigCleanup((Abc_Aig_t*)pNtkDup->pManFunc);
    
    int i;  Abc_Obj_t *pObj, *pObjNew;
    Abc_Ntk_t *pNtkRes = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtkRes->pName = Extra_UtilStrsav(pNtkDup->pName);
    
    // copy network
    Abc_AigConst1(pNtkDup)->pCopy = Abc_AigConst1(pNtkRes);
    Abc_NtkForEachCi(pNtkDup, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkRes);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    Abc_AigForEachAnd(pNtkDup, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachCo(pNtkDup, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    if(fDel) Abc_NtkDelete(pNtkDup);

    // rounding up #PI
    uint n = Abc_NtkPiNum(pNtkRes) % mult;
    char buf[1000];
    if(n) for(uint i=0; i<mult-n; ++i) {
        sprintf(buf, "dummy_%u", i);
        Abc_ObjAssignName(Abc_NtkCreatePi(pNtkRes), buf, NULL);
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    if(rm) Abc_NtkDelete(pNtk);

    return pNtkRes;
}

// create a new latch
Abc_Obj_t* aigNewLatch(Abc_Ntk_t *pNtk, cuint initVal, char *latchName, char *inName, char *outName)
{
    Abc_Obj_t *pLatch, *pLatchInput, *pLatchOutput;
    pLatch = Abc_NtkCreateLatch(pNtk);
    pLatchInput = Abc_NtkCreateBi(pNtk);
    pLatchOutput = Abc_NtkCreateBo(pNtk);
    Abc_ObjAddFanin(pLatch, pLatchInput);
    Abc_ObjAddFanin(pLatchOutput, pLatch);

    if(latchName) Abc_ObjAssignName(pLatch, latchName, NULL);
    if(inName) Abc_ObjAssignName(pLatchInput, inName, NULL );
    if(outName) Abc_ObjAssignName(pLatchOutput, outName, NULL );

    switch(initVal) {
    case 0:
        Abc_LatchSetInit0(pLatch);
        break;
    case 1:
        Abc_LatchSetInit1(pLatch);
        break;
    default:
        Abc_LatchSetInitDc(pLatch);
        break;
    }

    return pLatch;
}

Abc_Ntk_t* aigInitNtk(cuint nPi, cuint nPo, cuint nLatch, const char *name)
{
    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = Extra_UtilStrsav(name ? name : "newAIG");

    for(uint i=0; i<nPi; ++i) Abc_NtkCreatePi(pNtk);
    for(uint i=0; i<nPo; ++i) Abc_NtkCreatePo(pNtk);
    for(uint i=0; i<nLatch; ++i) aigNewLatch(pNtk, 0);

    Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtk);
    Abc_NtkAddDummyBoxNames(pNtk);

    //assert(Abc_NtkCheck(pNtk));  // will fail, since no fanin for POs
    return pNtk;
}

} // end namespace utils::aigUtils

