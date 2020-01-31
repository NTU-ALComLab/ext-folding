#include "ext-folding/n-split/split.h"
#include <unordered_map>
#include <unordered_set>

namespace nSplit
{

static inline vector<uint> iSplit(cuint i, cuint n)
{
    vector<uint> ret;  ret.reserve(n);
    cuint q = i / n;
    cuint r = i - n * q;
    assert(r < n);

    for(uint j=0; j<n; ++j) {
        if(j < r) ret.push_back(q + 1);
        else ret.push_back(q);
    }

    return ret;
}

static inline void initQue(vector<Abc_Obj_t*> &que, Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew, cuint i1, cuint i2)
{
    que.reserve(2 * (i2 - i1) + 1);
    que.push_back(Abc_AigConst1(pNtk));
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    for(uint i=i1; i<i2; ++i) {
        Abc_Obj_t *pObj = Abc_NtkPi(pNtk, i);
        pObj->pCopy = Abc_NtkPi(pNtkNew, i-i1);
        que.push_back(pObj);
    }    
}

static inline void initQue(vector<Abc_Obj_t*> &que, Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew, const vector<Abc_Obj_t*> &vObjs)
{
    que.reserve(2 * vObjs.size() + 1);
    que.push_back(Abc_AigConst1(pNtk));
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    for(uint i=0; i<vObjs.size(); ++i) {
        vObjs[i]->pCopy = Abc_NtkPi(pNtkNew, i);
        que.push_back(vObjs[i]);
    }    
}

static inline void processQue(vector<Abc_Obj_t*> &que, Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew, unordered_map<Abc_Obj_t*, Abc_Obj_t*> &vMap)
{
    uint i;
    while(!que.empty()) {
        Abc_Obj_t *pObj = que.back(), *pFanout;
        que.pop_back();

        Abc_ObjForEachFanout(pObj, pFanout, i) if(!pFanout->pCopy) {
            if(vMap.find(pFanout) == vMap.end()) { // not found
                vMap[pFanout] = pObj;
            } else {
                assert(vMap[pFanout] != pObj);
                pFanout->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pFanout), Abc_ObjChild1Copy(pFanout));
                que.push_back(pFanout);
                vMap.erase(pFanout);
            }
        }
    }
}

static inline Abc_Ntk_t* aigTrav(Abc_Ntk_t *pNtk, cuint i1, cuint i2, vector<Abc_Obj_t*> &vObjs)
{
    Abc_NtkCleanCopy(pNtk);

    char buf[100];
    sprintf(buf, "%s_i%u-i%u", pNtk->pName, i1, i2);

    Abc_Ntk_t *pNtkRes = aigUtils::aigInitNtk(i2-i1, 0, 0, buf);

    vector<Abc_Obj_t*> que;
    initQue(que, pNtk, pNtkRes, i1, i2);

    unordered_map<Abc_Obj_t*, Abc_Obj_t*> vMap;
    processQue(que, pNtk, pNtkRes, vMap);

    unordered_set<Abc_Obj_t*> vSet;
    for(auto p: vMap) if(vSet.find(p.second) == vSet.end()) {
        Abc_Obj_t *pObj = p.second;
        Abc_Obj_t *pPo = Abc_NtkCreatePo(pNtkRes);
        sprintf(buf, "internal_%lu", vObjs.size());
        Abc_ObjAssignName(pPo, buf, NULL);
        Abc_ObjAddFanin(pPo, pObj->pCopy);
        vObjs.push_back(pObj);
        vSet.insert(pObj);
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));
    return pNtkRes;
}

static inline Abc_Ntk_t* collectRemain(Abc_Ntk_t *pNtk, vector<Abc_Obj_t*> &vObjs)
{
    Abc_NtkCleanCopy(pNtk);

    char buf[100];
    sprintf(buf, "%s_top", pNtk->pName);

    Abc_Ntk_t *pNtkRes = aigUtils::aigInitNtk(vObjs.size(), Abc_NtkPoNum(pNtk), 0, buf);

    vector<Abc_Obj_t*> que;
    initQue(que, pNtk, pNtkRes, vObjs);

    unordered_map<Abc_Obj_t*, Abc_Obj_t*> vMap;
    processQue(que, pNtk, pNtkRes, vMap);

  /*
    uint cnt = 0;
    for(auto p: vMap) {
        if(Abc_ObjType(p.first) == ABC_OBJ_PO) cnt++;
        else {
            cout << p.first << " " << Abc_ObjType(p.first) << " , ";
            cout << p.second << " " << Abc_ObjType(p.second) << endl;
        }
    }
    cout << vMap.size() << " / " << Abc_NtkPoNum(pNtk) << endl;
    cout << cnt << " / " << Abc_NtkPoNum(pNtk) << endl;
    assert(vMap.size() == Abc_NtkPoNum(pNtk));
  */

    Abc_Obj_t *pObj;  uint i;
    Abc_NtkForEachPo(pNtk, pObj, i) {
        Abc_Obj_t *pObjNew = Abc_NtkPo(pNtkRes, i);
        //Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    return pNtkRes;
}

vector<Abc_Ntk_t*> aigSplit(Abc_Ntk_t *pNtk, cuint n)
{
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    vector<Abc_Ntk_t*> ret;  ret.reserve(n+1);
    const vector<uint> iVec = iSplit(Abc_NtkPiNum(pNtk), n);

    vector<Abc_Obj_t*> objTrack;
    for(uint i=0,j=0; i<n; ++i) {
        ret.push_back(aigTrav(pNtk, j, j+iVec[i], objTrack));
        j += iVec[i];
        assert(j <= Abc_NtkPiNum(pNtk));
    }
    ret.push_back(collectRemain(pNtk, objTrack));
    assert(ret.size() == (n + 1));
    return ret;
}

} 