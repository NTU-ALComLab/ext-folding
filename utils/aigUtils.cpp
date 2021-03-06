#include "ext-folding/utils/utils.h"

extern "C"
{
void Abc_NtkRemovePo(Abc_Ntk_t *pNtk, int iOutput, int fRemoveConst0);
//int Abc_NtkQuantify(Abc_Ntk_t *pNtk, int fUniv, int iVar, int fVerbose);
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
    Abc_Ntk_t *pNtkDup = fDel ? Abc_NtkStrash(pNtk, 0, 1, 0) : pNtk;
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
    case ABC_INIT_ZERO:
        Abc_LatchSetInit0(pLatch);
        break;
    case ABC_INIT_ONE:
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
    for(uint i=0; i<nLatch; ++i) aigNewLatch(pNtk, ABC_INIT_ZERO);

    Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtk);
    Abc_NtkAddDummyBoxNames(pNtk);

    //assert(Abc_NtkCheck(pNtk));  // will fail, since no fanin for POs
    return pNtk;
}

Abc_Ntk_t* aigMerge(Abc_Ntk_t **pNtks, cuint nNtks, bool rm)
{
    int i;  char charBuf[100];
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
    
    sprintf(charBuf, "%u_merge", nNtks);
    pNtkNew->pName = Extra_UtilStrsav(charBuf);
    
    for(uint j=0; j<nNtks; ++j) {
        vector<Abc_Obj_t*> vLats;  vLats.reserve(Abc_NtkBoxNum(pNtks[j]));

        Abc_AigConst1(pNtks[j])->pCopy = Abc_AigConst1(pNtkNew);
        Abc_NtkForEachPi(pNtks[j], pObj, i)
            pObj->pCopy = aigIthVar(pNtkNew, i);

        Abc_NtkForEachBox(pNtks[j], pObj, i) {
            pObjNew = aigNewLatch(pNtkNew, Abc_LatchInit(pObj));
            Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);
            vLats.push_back(pObjNew);
        }
        assert(vLats.size() == Abc_NtkBoxNum(pNtks[j]));

        Abc_AigForEachAnd(pNtks[j], pObj, i)
            pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

        Abc_NtkForEachBox(pNtks[j], pObj, i) {
            pObjNew = vLats[i];
            Abc_ObjAddFanin(Abc_ObjFanin0(pObjNew), Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));
        }
        Abc_NtkForEachPo(pNtks[j], pObj, i) {
            pObjNew = Abc_NtkCreatePo(pNtkNew);
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        }       
    }

    //Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtkNew);
    Abc_NtkAddDummyBoxNames(pNtkNew);    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));

    if(rm) for(uint j=0; j<nNtks; ++j) Abc_NtkDelete(pNtks[j]);
    
    return pNtkNew;
}

Abc_Ntk_t* aigReadFromFile(char *fileName)
{
    Abc_Ntk_t *pNtk = Io_Read(fileName, Io_ReadFileType(fileName), 1, 0);
    Abc_Ntk_t *pNtkRes = Abc_NtkStrash(pNtk, 0, 1, 0);
    Abc_NtkDelete(pNtk);
    return pNtkRes;
}

Abc_Ntk_t* aigReadFromFile(const string &fileName)
{
    char *name = Extra_UtilStrsav(fileName.c_str());
    Abc_Ntk_t *pNtk = aigReadFromFile(name);
    //Abc_Ntk_t *pNtk = Io_Read(name, Io_ReadFileType(name), 1, 0);
    ABC_FREE(name);
    return pNtk;
}

Abc_Ntk_t* aigConstructDummyNtk(cuint n)
{
    char buf[100];
    sprintf(buf, "dummy_%u", n);

    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = Extra_UtilStrsav(buf);    
    
    for(uint i=0; i<n; ++i) {
        Abc_Obj_t *pPi = Abc_NtkCreatePi(pNtk);
        Abc_Obj_t *pPo = Abc_NtkCreatePo(pNtk);
        Abc_ObjAddFanin(pPo, pPi);
    }
    
    Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtk);
    assert(Abc_NtkCheck(pNtk));
    
    return pNtk;
}

Abc_Ntk_t* aigBuildParity(cuint n)
{
    char buf[100];
    sprintf(buf, "parity_%u", n);

    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = Extra_UtilStrsav(buf);    
    
    Abc_Obj_t *pObj = Abc_ObjNot(Abc_AigConst1(pNtk));

    for(uint i=0; i<n; ++i)
        pObj = Abc_AigXor((Abc_Aig_t*)pNtk->pManFunc, Abc_NtkCreatePi(pNtk), pObj);
    
    Abc_ObjAddFanin(Abc_NtkCreatePo(pNtk), pObj);
    
    Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtk);
    assert(Abc_NtkCheck(pNtk));
    
    return pNtk;
}

Abc_Obj_t* aigGenVoter3(Abc_Ntk_t *pNtk, const vector<Abc_Obj_t*> &vObj)
{
    assert(vObj.size() == 3);
    Abc_Obj_t *p1, *p2, *p3, *ret;
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtk->pManFunc;
    p1 = Abc_AigAnd(pMan, vObj[0], vObj[1]);
    p2 = Abc_AigAnd(pMan, vObj[0], vObj[2]);
    p3 = Abc_AigAnd(pMan, vObj[1], vObj[2]);
    ret = Abc_AigOr(pMan, p1, p2);
    ret = Abc_AigOr(pMan, p3, ret);
    return ret;
}

Abc_Ntk_t* aigGenVoterN_seq(cuint n, const bool tieVal)
{
    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = Extra_UtilStrsav("voter");
    
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtk->pManFunc;
    Abc_Obj_t *pPi = Abc_NtkCreatePi(pNtk);

    Abc_Obj_t *pLat_prev = NULL;

    cuint nn = n/2 + (n%2 ? 1 : (1-tieVal));
    for(uint i=0; i<nn; ++i) {
    //for(uint i=0; i<n; ++i) {
        Abc_Obj_t *pLat = aigNewLatch(pNtk, ABC_INIT_ZERO), *pLatIn;
        pLatIn = (i == 0) ? pPi : Abc_AigAnd(pMan, Abc_ObjFanout0(pLat_prev), pPi);
        pLatIn = Abc_AigOr(pMan, Abc_ObjFanout0(pLat), pLatIn);
        Abc_ObjAddFanin(Abc_ObjFanin0(pLat), pLatIn);
        pLat_prev = (i == nn-1) ? pLatIn : pLat;
    }

    /*
    Abc_Obj_t *pPo = Abc_ObjNot(Abc_AigConst1(pNtk));
    for(uint i=n/2; i<n; ++i) {
        Abc_Obj_t *pObj = Abc_ObjFanout0(Abc_NtkBox(pNtk, i));
        pPo = Abc_AigOr(pMan, pObj, pPo);
    }
    Abc_ObjAddFanin(Abc_NtkCreatePo(pNtk), pPo);
    */
    Abc_ObjAddFanin(Abc_NtkCreatePo(pNtk), pLat_prev);

    Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtk);
    Abc_NtkAddDummyBoxNames(pNtk);
    Abc_AigCleanup(pMan);
    assert(Abc_NtkCheck(pNtk));
    return pNtk;
}

// replace a primary input with constant 0 or 1
// currently only supports combinational circuits (no flip-flops)
Abc_Ntk_t* aigPiCofactor(Abc_Ntk_t *pNtk, cuint iPi, bool val, bool rm)
{
    char buf[100];
    sprintf(buf, "%s_cofi%u", pNtk->pName, iPi);

    Abc_Ntk_t *pNtkRes = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtkRes->pName = Extra_UtilStrsav(buf);

    uint i;  Abc_Obj_t *pObj;
    return pNtkRes;
}

Abc_Ntk_t* aigBuildVoter(cuint n, const bool tieVal)
{
    //assert(n % 2 == 1);
    Abc_Ntk_t *pNtkSeq = aigGenVoterN_seq(n, tieVal);
    Abc_Ntk_t *pNtkExp = Abc_NtkFrames(pNtkSeq, n, 1, 0);
    Abc_Ntk_t *pNtkRes = aigSingleCone(pNtkExp, Abc_NtkPoNum(pNtkExp)-1, true);
    Abc_NtkDelete(pNtkSeq);
    assert(Abc_NtkCheck(pNtkRes));
    return pNtkRes;
    //return NULL;
}

/* wrong implementaion of voter
Abc_Obj_t* aigGenVoterN(Abc_Ntk_t *pNtk, const vector<Abc_Obj_t*> &vObj)
{
    if(vObj.size() == 1) return vObj[0];
    assert(vObj.size() % 2 == 1);
    cuint q = vObj.size() / 3;
    cuint r = vObj.size() % 3;
    vector<Abc_Obj_t*> vObj2;
    vObj2.reserve(q + r);
    for(uint i=0; i<r; ++i)
        vObj2.push_back(vObj[q*3+i]);
    for(uint i=0; i<q; ++i) {
        vector<Abc_Obj_t*> vObj3;  vObj3.reserve(3);
        for(uint j=0; j<3; ++j)
            vObj3.push_back(vObj[i*3+j]);
        vObj2.push_back(aigGenVoter3(pNtk, vObj3));
    }
    return aigGenVoterN(pNtk, vObj2);
}

Abc_Ntk_t* aigBuildVoter(cuint n)
{
    assert(n % 2 == 1);
    char buf[100];
    sprintf(buf, "voter_%u", n);

    Abc_Ntk_t *pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtk->pName = Extra_UtilStrsav(buf);    
    
    vector<Abc_Obj_t*> vObj;
    vObj.reserve(n);
    for(uint i=0; i<n; ++i)
        vObj.push_back(Abc_NtkCreatePi(pNtk));
    
    Abc_Obj_t *pObj = aigGenVoterN(pNtk, vObj);
    Abc_ObjAddFanin(Abc_NtkCreatePo(pNtk), pObj);
    
    Abc_NtkAddDummyPiNames(pNtk);
    Abc_NtkAddDummyPoNames(pNtk);
    assert(Abc_NtkCheck(pNtk));
    
    return pNtk;
}
*/

/*
void aigTravUp(vector<Abc_Obj_t*> &visited, vector<Abc_Obj_t*> que, cuint travId)
{
    if(que.empty()) return;

    vector<Abc_Obj_t*> newQue;
    for(Abc_Obj_t *pObj: que) {
        uint i;  Abc_Obj_t *pFanout;
        Abc_ObjForEachFanout(pObj, pFanout, i) {
            if((Abc_ObjFanin0(pFanout)->iTemp == travId) && (Abc_ObjFanin1(pFanout)->iTemp == travId)) {
                assert(pFanout->iTemp != travId);
                pFanout->iTemp = travId;
                visited.push_back(pFanout);
                newQue.push_back(pFanout);
            }
        }
    }
    aigTravUp(visited, newQue, travId);
}

void aigTravUp(vector<Abc_Obj_t*> &visited, Abc_Obj_t *currNode, cuint travId)
{
    if(Abc_ObjIsCo(currNode)) return;

    assert(currNode->iTemp == travId);
    assert(Abc_ObjIsCi(currNode) ||
        ((Abc_ObjFanin0(currNode)->iTemp == travId) && (Abc_ObjFanin0(currNode)->iTemp == travId)));

    uint i;  Abc_Obj_t *pFanout;
    Abc_ObjForEachFanout(currNode, pFanout, i) {
        if((Abc_ObjFanin0(pFanout)->iTemp == travId) && (Abc_ObjFanin1(pFanout)->iTemp == travId)) {
            assert(pFanout->iTemp != travId);
            pFanout->iTemp = travId;
            visited.push_back(pFanout);
            aigTravUp(visited, pFanout, travId);
        }
    }
}
*/

} // end namespace utils::aigUtils

