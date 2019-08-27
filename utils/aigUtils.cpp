#include "ext-folding/utils/utils.h"

namespace utils::aigUtils
{

Abc_Ntk_t* aigCone(Abc_Ntk_t *pNtk, const size_t start, const size_t end)
{
    assert(start < end);
    assert(start < Abc_NtkPoNum(pNtk));
    assert(end <= Abc_NtkPoNum(pNtk));
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
    
    return pNtkNew;
}

Abc_Ntk_t* aigPerm(Abc_Ntk_t *pNtk, size_t *perm)
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
    
    return pNtkPerm;
}

} // end namespace utils::aigUtils

