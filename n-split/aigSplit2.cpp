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
    if(currNode->iTemp / 2 == travId) return;
    assert(Abc_ObjIsCi(currNode) ||
        ((Abc_ObjFanin0(currNode)->iTemp / 2 == travId) && (Abc_ObjFanin1(currNode)->iTemp / 2 == travId)));

    currNode->iTemp = 2 * travId + 1;
    visited.push_back(currNode);

    uint i;  Abc_Obj_t *pFanout;
    Abc_ObjForEachFanout(currNode, pFanout, i) {
        if(Abc_ObjIsCo(pFanout)) continue;
        if((Abc_ObjFanin0(pFanout)->iTemp / 2 == travId) && (Abc_ObjFanin1(pFanout)->iTemp / 2 == travId)) {
            //assert(pFanout->iTemp / 2 != travId);
            //pFanout->iTemp = 2 * travId + 1;
            //visited.push_back(pFanout);
            aigTravUp(visited, pFanout, travId);
        }
    }
}

// mark nodes whose fanouts have all been visited with 2 * travId
// returns the number of expected POs
uint unmarkFrontier(vector<Abc_Obj_t*> &visited, cuint travId)
{
    Abc_Obj_t *pFanout;  uint i, nPo = 0;
    for(Abc_Obj_t *pObj: visited) {
        assert(!Abc_ObjIsCo(pObj));
        if(pObj->iTemp == 2 * travId) continue;
        assert(pObj->iTemp == 2 * travId + 1);

        Abc_ObjForEachFanout(pObj, pFanout, i) 
            if(pFanout->iTemp / 2 != travId)
                break;
        
        if(i == Abc_ObjFanoutNum(pObj))
            pObj->iTemp = 2 * travId;
        else if(!Abc_ObjIsCi(pObj))
            ++nPo;
    }
    return nPo;
}

static inline void backtrace(vector<Abc_Obj_t*> &visited, cuint n)
{
    for(uint i=0; i<n; ++i) {
        visited.back()->iTemp = 0;
        visited.pop_back();
    }
}

static inline void backtraceAll(vector<Abc_Obj_t*> &visited)
{
    for(Abc_Obj_t *pObj: visited) pObj->iTemp = 0;
    visited.clear();
}

static inline void aigTravUpFromCi(Abc_Ntk_t *pNtk, vector<Abc_Obj_t*> &visited, const SupVec &sv, cuint travId)
{
    //assert(!visited.empty());
    //Abc_Ntk_t *pNtk = visited.front()->pNtk;
    assert(Abc_NtkPiNum(pNtk) == sv.size());

    for(uint i=0; i<sv.size(); ++i) if(sv[i])
        aigTravUp(visited, Abc_NtkPi(pNtk, i), travId);
}



// select an AIG node that incurs 1) least additional PIs 2) most(/least) additional nodes
// update visited nodes accordingly
bool selectOneNode(const SupVec &asv, vector<Abc_Obj_t*> &visited, SupVec &sv, cuint travId)
{
    vector<Abc_Obj_t*> nVec;
    uint nPi;
    for(Abc_Obj_t *pObj: visited) if(pObj->iTemp == 2 * travId + 1) {
        Abc_Obj_t *pFanout;  uint i;
        Abc_ObjForEachFanout(pObj, pFanout, i) if(!Abc_ObjIsCo(pFanout) && (pFanout->iTemp / 2 != travId)) {
            const SupVec csv = sv | (*(SupVec*)(pFanout->pData));
            if((csv & asv).any()) continue;

            if(nVec.empty()) {
                nPi = csv.count();
                assert(nPi > sv.count());
                nVec.push_back(pFanout);
            } else {
                cuint _nPi = csv.count();
                if(_nPi == nPi) nVec.push_back(pFanout);
                else if(_nPi < nPi) {
                    nPi = _nPi;
                    nVec.clear();
                    nVec.push_back(pFanout);
                }
            }
        }
    }

    if(nVec.empty()) return false;

    assert(!visited.empty());
    Abc_Ntk_t *pNtk = visited.front()->pNtk;
    Abc_Obj_t *bestNode;  uint nNode;
    for(uint i=0; i<nVec.size(); ++i) {
        vector<Abc_Obj_t*> newVst;
        Abc_Obj_t *pObj = nVec[i];
        const SupVec csv = sv | (*(SupVec*)(pObj->pData));
        aigTravUpFromCi(pNtk, newVst, csv, travId);
        if(i == 0) {
            bestNode = pObj;
            nNode = newVst.size();
        //} else if(newVst.size() > nNode) {  // most
        } else if(newVst.size() < nNode) {  // least
            bestNode = pObj;
            nNode = newVst.size();
        }
        backtraceAll(newVst);
    }

    sv |= (*(SupVec*)(bestNode->pData));
    aigTravUpFromCi(pNtk, visited, sv, travId);
    return true;
}


bool markOne(Abc_Ntk_t *pNtk, SupVec &asv, vector<Abc_Obj_t*> &visited, cuint travId, cuint mPi, cuint mPo, cuint mNode)
{
    Abc_Obj_t *pObj;
    uint i, nPo = 0;
    SupVec sv(Abc_NtkPiNum(pNtk));
    
    // choose starting PI
    Abc_NtkForEachPi(pNtk, pObj, i) if(pObj->iTemp == 0) {
        pObj->iTemp = 2 * travId + 1;
        visited.push_back(pObj);
        sv[i] = 1;
        break;
    }

    if(i == Abc_NtkPiNum(pNtk)) return false;
    
    while((sv.count() < mPi) && (nPo < mPo) && (visited.size() < mNode)) {  // circuit size within limits
        bool stat = selectOneNode(asv, visited, sv, travId);
        if(!stat) {
            backtraceAll(visited);
            return false;
        }

        nPo = unmarkFrontier(visited, travId);

        //for(Abc_Obj_t *_pObj: visited) cout << _pObj->Id << " ";
        //cout << endl;
    }

    asv |= sv;

    return true;
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

static inline void printSupVec(const SupVec &sv)
{
    for(uint i=0; i<sv.size(); ++i)
        cout << sv[i];
}

static inline void printObjSupVec(Abc_Obj_t *pObj)
{
    cout << pObj->Id << ": ";
    printSupVec(*((SupVec*)(pObj->pData)));
    cout << endl;
}

static inline void printSupVecInfo(Abc_Ntk_t *pNtk)
{
    Abc_Obj_t *pObj; uint i;
    Abc_NtkForEachPi(pNtk, pObj, i)
        printObjSupVec(pObj);
    Abc_AigForEachAnd(pNtk, pObj, i)
        printObjSupVec(pObj);
}

static Abc_Ntk_t* buildNtk(vector<Abc_Obj_t*> visited, cuint travId)
{
    assert(!visited.empty());    

    Abc_Ntk_t *pNtk = visited.front()->pNtk;
    Abc_Ntk_t *pNtkRes = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    
    char buf[100];
    sprintf(buf, "%s-%u", pNtk->pName, travId);
    pNtkRes->pName = Extra_UtilStrsav(buf);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkRes);

    for(Abc_Obj_t *pObj: visited) {
        assert(pObj->iTemp / 2 == travId);
        Abc_Obj_t *pObjNew = NULL;

        if(Abc_ObjIsPi(pObj)) {
            pObjNew = Abc_NtkCreatePi(pNtkRes);
            pObj->pCopy = pObjNew;
            Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        } else {
            if(pObj->iTemp == 2 * travId + 1)
                pObjNew = Abc_NtkCreatePo(pNtkRes);
            pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
            if(pObjNew) {
                Abc_ObjAddFanin(pObjNew, pObj->pCopy);
                //Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
            }
        }
    }

    for(Abc_Obj_t *pObj: visited)
        pObj->iTemp = 2 * travId;

    Abc_NtkAddDummyPoNames(pNtkRes);
    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    return pNtkRes;
    
}

vector<Abc_Ntk_t*> aigSplit2(Abc_Ntk_t *pNtk, cuint mPi, cuint mPo, cuint mNode)
{
    vector<Abc_Ntk_t*> ret;
    Abc_NtkCleanCopy(pNtk);
    Abc_NtkCleanData(pNtk);

    initSupVec(pNtk);
    //printSupVecInfo(pNtk);

    Abc_Obj_t *pObj7;  uint i;
    Abc_NtkForEachObj(pNtk, pObj7, i) if(pObj7->Id == 7) break;

    SupVec asv(Abc_NtkPiNum(pNtk));
    bool stat = true;
    for(uint i=1; stat; ++i) {
        //cout << "cutting " << i << endl;
        vector<Abc_Obj_t*> visited;  visited.reserve(mNode);
        stat = markOne(pNtk, asv, visited, i, mPi, mPo, mNode);
        if(stat) {
            Abc_Ntk_t *pNtkRes =  buildNtk(visited, i);
            Abc_NtkPrintStats(pNtkRes, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            ret.push_back(pNtkRes);
            //for(Abc_Obj_t *_pObj: visited) cout << _pObj->Id << " ";
            //cout << endl;
        }
        visited.clear();
    }

    clearSupVec(pNtk);
    return ret;
}

} // end namespace nSplit2