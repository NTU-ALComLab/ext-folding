#include "ext-folding/utils/utils.h"

namespace utils::aigUtils
{

// take the cone of POs from indices "start" to "end"
// the newly-formed subcircuit will keep all PIs
Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, cuint start, cuint end, bool rm)
{
    assert(start < end);
    assert(start < Abc_NtkPoNum(pNtk));
    assert(end <= Abc_NtkPoNum(pNtk));

    // single PO cone
    if(start+1 == end) return aigCone(pNtk, start, rm);

    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    int i;  Abc_Obj_t *pObj, *pObjNew;
    
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    
    char buf[100];
    sprintf(buf, "%s_cone%lu-%lu", pNtk->pName, start, end);
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

// take the cone of single PO, with more efficient circuit construction
Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, cuint n, bool rm)
{
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

// permutate the order of PIs with the given "perm"
Abc_Ntk_t* aigPerm(Abc_Ntk_t *pNtk, size_t *perm, bool rm)
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

/* fix sprintf
// concatenate the given array of circuits, each circuit should have the same #PIs
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
    
    for(i=0; i<nNtks; ++i)
        sprintf(charBuf, "%s%s_", charBuf, pNtks[i]->pName);
    sprintf(charBuf, "%s%s", charBuf, "concat");
    pNtkNew->pName = Extra_UtilStrsav(charBuf);
    sprintf(charBuf, "");  // reset buffer
    
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
    
    // AddOne for each Ntk
    for(size_t j=0; j<nNtks; ++j) Abc_AigForEachAnd(pNtks[j], pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    // create and collect POs for each Ntk
    for(size_t j=0; j<nNtks; ++j) Abc_NtkForEachCo(pNtks[j], pObj, i) {
        pObjNew = Abc_ObjChild0Copy(pObj);
        pObj = Abc_NtkCreatePo(pNtkNew);
        Abc_ObjAddFanin(pObj, pObjNew);
        
        sprintf(charBuf, "concat%lu", j);
        Abc_ObjAssignName(pObj, charBuf, Abc_ObjName(pObj));
        sprintf(charBuf, "");  // reset buffer
    }
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));

    if(rm) for(size_t j=0; j<nNtks; ++j) Abc_NtkDelete(pNtks[j]);
    
    return pNtkNew;
}
*/

// compute inner product(dot) of 2 given arrays of AIG nodes
Abc_Obj_t* aigDot(Abc_Ntk_t* pNtk, Abc_Obj_t** v1, Abc_Obj_t** v2, cuint len)
{
    Abc_Obj_t *ret = Abc_ObjNot(Abc_AigConst1(pNtk));
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtk->pManFunc;
    for(size_t i=0; i<len; ++i)
        ret = Abc_AigOr(pMan, ret, Abc_AigAnd(pMan, v1[i], v2[i]));
    return ret;
}

} // end namespace utils::aigUtils

