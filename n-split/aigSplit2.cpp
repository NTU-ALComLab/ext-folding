#include "ext-folding/n-split/split.h"
#include "ext-folding/utils/supVec.h"

using namespace SupVech;

namespace nSplit2
{

static inline uint countPI(Abc_Ntk_t *pNtk, cuint travId)
{
    uint ret = 0, i;
    Abc_Obj_t *pObj;
    Abc_NtkForEachPi(pNtk, pObj, i) if(pObj->iTemp / 2 == travId) ret++;
    return ret;
}

static inline uint countPO(const vector<Abc_Obj_t*> visited, cuint travId)
{
    uint ret = 0;
    for(Abc_Obj_t *pObj: visited) if((pObj->iTemp == 2 * travId + 1) && !Abc_ObjIsPi(pObj)) ret++;
    return ret;
}

// mark visited: 2 * travId + 1
void aigTravUp(vector<Abc_Obj_t*> &visited, Abc_Obj_t *currNode, cuint travId)
{
    assert(currNode->iTemp / 2 == travId);
    assert(Abc_ObjIsCi(currNode) ||
        ((Abc_ObjFanin0(currNode)->iTemp / 2 == travId) && (Abc_ObjFanin0(currNode)->iTemp / 2 == travId)));

    uint i;  Abc_Obj_t *pFanout;
    Abc_ObjForEachFanout(currNode, pFanout, i) {
        if((Abc_ObjFanin0(pFanout)->iTemp / 2 == travId) && (Abc_ObjFanin1(pFanout)->iTemp / 2 == travId)) {
            assert(pFanout->iTemp / 2 != travId);
            pFanout->iTemp = 2 * travId + 1;
            visited.push_back(pFanout);
            aigTravUp(visited, pFanout, travId);
        }
    }
}

// mark nodes whose fanouts have all been visited with 2 * travId
void unmarkFrontier(vector<Abc_Obj_t*> &visited, cuint travId)
{
    
}

void backtrace(vector<Abc_Obj_t*> &visited, cuint n)
{
    for(uint i=0; i<n; ++i) {
        visited.back()->iTemp = 0;
        visited.pop_back();
    }
}

// select an AIG node that incurs least additional 1)PIs 2)POs
void selectOneNode(vector<Abc_Obj_t*> &visited, SupVec &sv, )
{
    
}



static inline bool checkLim(const vector<Abc_Obj_t*> &visited, const SupVec &sv, cuint travId, cuint mPi, cuint mPo, cuint mNode)
{
    return (sv.count() < mPi) && (countPO(visited, travId) < mPo) && (visited.size() < mNode);
    //return (countPI(pNtk, travId) < mPi) && (countPO(visited, travId) < mPo) && (visited.size() < mNode);
}

void markOne(Abc_Ntk_t *pNtk, vector<Abc_Obj_t*> &visited, cuint travId, cuint mPi, cuint mPo, cuint mNode)
{
    Abc_Obj_t *pObj;  uint i;
    SupVec sv(Abc_NtkPiNum(pNtk));

    // choose starting PI
    Abc_NtkForEachPi(pNtk, pObj, i) if(pObj->iTemp == 0) {
        pObj->iTemp = 2 * travId + 1;
        visited.push_back(pObj);
        sv[i] = 1;
        break;
    }

    while(checkLim(visited, sv, travId, mPi, mPo, mNode)) {  // circuit size within limits
        ;
    }


}

static inline void initSupVec(Abc_Ntk_t *pNtk)
{
    Abc_Obj_t *pObj; uint i;
    cuint nPi = Abc_NtkPiNum(pNtk);

    Abc_NtkForEachPi(pNtk, pObj, i) {
        pObj->pData = (void*)(new SupVec(nPi));
        (*(SupVec*)(pObj->pData))[i] = 1;
    }

    Abc_AigForEachAnd(pNtk, pObj, i) {
        pObj->pData = (void*)(new SupVec(nPi));
        *(SupVec*)(pObj->pData) |= *(SupVec*)(Abc_ObjFanin0(pObj)->pData);
        *(SupVec*)(pObj->pData) |= *(SupVec*)(Abc_ObjFanin1(pObj)->pData);
    }
}

static inline void clearSupVec(Abc_Ntk_t *pNtk)
{
    Abc_Obj_t *pObj; uint i;
    Abc_NtkForEachPi(pNtk, pObj, i) {
        delete (SupVec*)(pObj->pData);
        pObj->pData = NULL;
    }
    Abc_AigForEachAnd(pNtk, pObj, i) {
        delete (SupVec*)(pObj->pData);
        pObj->pData = NULL;
    }
}

vector<Abc_Ntk_t*> aigSplit2(Abc_Ntk_t *pNtk, cuint mPi, cuint mPo, cuint mNode)
{
    vector<Abc_Ntk_t*> ret;
    Abc_NtkCleanCopy(pNtk);
    initSupVec(pNtk);


    clearSupVec(pNtk);
    return ret;
}

} // end namespace nSplit2