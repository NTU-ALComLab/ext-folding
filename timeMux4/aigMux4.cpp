#include "ext-folding/timeMux2/timeMux2.h"
#include "ext-folding/timeMux3/timeMux3.h"
#include "ext-folding/timeMux4/timeMux4.h"

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

extern "C"
{
void Abc_NtkCecFraig(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, int nSeconds, int fVerbose);
}

using namespace timeMux4;
using namespace aigUtils;

namespace timeMux4
{

typedef unordered_map<Abc_Obj_t*, Abc_Obj_t*> ObjFaninMap;
typedef pair<uint, Abc_Obj_t*> PoInfoPair;
typedef vector<vector<PoInfoPair> > PoInfos;
typedef unordered_map<Abc_Obj_t*, uint> PoIdxMap;

static inline void initPoIdxMap(Abc_Ntk_t *pNtk, PoIdxMap &map)
{
    uint i;  Abc_Obj_t *pObj;
    Abc_NtkForEachPo(pNtk, pObj, i)
        map[pObj] = i;
}

static void initQue(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, vector<Abc_Obj_t*> &que, cuint t, cuint nPi)
{
    assert(que.empty());
    if(!t) que.push_back(Abc_AigConst1(pNtkComb));
    for(uint i=0; i<nPi; ++i) {
        Abc_Obj_t *pObj = Abc_NtkPi(pNtkComb, t*nPi+i);
        pObj->pCopy = Abc_NtkPi(pNtkMux, i);
        que.push_back(pObj);
    }
}

static void processQue(Abc_Ntk_t *pNtkMux, cuint t, vector<Abc_Obj_t*> &que, ObjFaninMap &vMap, PoInfos &poInfos, PoIdxMap &poIdxMap)
{
    uint i;
    while(!que.empty()) {
        Abc_Obj_t *pObj = que.back(), *pFanout;
        que.pop_back();

        Abc_ObjForEachFanout(pObj, pFanout, i) {
            if(Abc_ObjIsPo(pFanout)) {
                poInfos[t].push_back(make_pair(poIdxMap[pFanout], Abc_ObjChild0Copy(pFanout)));
            } else {
                if(vMap.find(pFanout) == vMap.end()) { // not found
                    vMap[pFanout] = pObj;
                } else {
                    assert(vMap[pFanout] != pObj);
                    pFanout->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkMux->pManFunc, Abc_ObjChild0Copy(pFanout), Abc_ObjChild1Copy(pFanout));
                    que.push_back(pFanout);
                    vMap.erase(pFanout);
                }
            }
        }
    }
}

static void storeInLats(Abc_Ntk_t *pNtkMux, cuint t, ObjFaninMap &vMap)
{
    unordered_set<Abc_Obj_t*> vSet;  vSet.reserve(vMap.size());
    for(auto p: vMap) {
        Abc_Obj_t *pObj = p.second;
        if(!Abc_ObjIsBo(pObj->pCopy))
            vSet.insert(p.second);
    }
    
    uint i = 0;
    for(Abc_Obj_t *pObj: vSet) {
        char ln[100], li[100], lo[100];
        sprintf(ln, "l-tf%u_%u", t, i);
        sprintf(li, "li-tf%u_%u", t, i);
        sprintf(lo, "lo-tf%u_%u", t, i);

        Abc_Obj_t *pLat, *pCtrl, *pLatIn;
        pLat = aigNewLatch(pNtkMux, ABC_INIT_ZERO, ln, li, lo);
        pCtrl = Abc_ObjFanout0(Abc_NtkBox(pNtkMux, t));
        pLatIn = Abc_AigMux((Abc_Aig_t*)pNtkMux->pManFunc, pCtrl, pObj->pCopy, Abc_ObjFanout0(pLat));
        Abc_ObjAddFanin(Abc_ObjFanin0(pLat), pLatIn);

        pObj->pCopy = Abc_ObjFanout0(pLat);

        ++i;
    }
}

static inline uint getPoNum(PoInfos &poInfos)
{
    uint n = 0;
    for(vector<PoInfoPair> &infoPV: poInfos)
        n = max(n, (uint)infoPV.size());
    return n;
}

static void buildPoFuncs(Abc_Ntk_t *pNtkMux, PoInfos &poInfos, int *oPerm)
{
    cuint nPo = getPoNum(poInfos);
    vector<Abc_Obj_t*> poVec;  poVec.reserve(nPo);
    Abc_Obj_t *pConst0 = Abc_ObjNot(Abc_AigConst1(pNtkMux));
    for(uint i=0; i<nPo; ++i)
        poVec.push_back(pConst0);
    
    Abc_Aig_t *pMan = (Abc_Aig_t*)pNtkMux->pManFunc;

    for(uint t=0; t<poInfos.size(); ++t) {
        vector<PoInfoPair> &infoPV = poInfos[t];
        sort(infoPV.begin(), infoPV.end());

        Abc_Obj_t *pCtrl = Abc_ObjFanout0(Abc_NtkBox(pNtkMux, t));
        assert(infoPV.size() <= nPo);
        for(uint i=0; i<infoPV.size(); ++i) {
            poVec[i] = Abc_AigOr(pMan, poVec[i], Abc_AigAnd(pMan, pCtrl, infoPV[i].second));
            oPerm[infoPV[i].first] = t*nPo + i;
        }
    }

    // create PO and add fanin
    for(uint i=0; i<nPo; ++i)
        Abc_ObjAddFanin(Abc_NtkCreatePo(pNtkMux), poVec[i]);
    Abc_NtkAddDummyPoNames(pNtkMux);
}

void buildTM(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, int *oPerm, cuint nTimeFrame, cuint nPi, TimeLogger *logger)
{
    ObjFaninMap vMap;
    vector<Abc_Obj_t*> que;
    
    PoInfos poInfos;  poInfos.resize(nTimeFrame);
    PoIdxMap poIdxMap;
    initPoIdxMap(pNtkComb, poIdxMap);

    Abc_AigConst1(pNtkComb)->pCopy = Abc_AigConst1(pNtkMux);

    for(uint t=0; t<nTimeFrame; ++t) {
        initQue(pNtkComb, pNtkMux, que, t, nPi);
        processQue(pNtkMux, t, que, vMap, poInfos, poIdxMap);
        storeInLats(pNtkMux, t, vMap);
    }
    assert(poInfos.size() == nTimeFrame);

    buildPoFuncs(pNtkMux, poInfos, oPerm);

    assert(timeMux2::checkPerm(oPerm, Abc_NtkPoNum(pNtkComb), Abc_NtkPoNum(pNtkMux)*nTimeFrame));
}

Abc_Ntk_t* aigStrMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, int *oPerm, const bool verbose, const char *logFileName)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;

    // get basic settings
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nPi = nCi / nTimeFrame;

    char buf[100];
    sprintf(buf, "%s-tm%u", pNtk->pName, nTimeFrame);
    Abc_Ntk_t *pNtkRes = aigInitNtk(nPi, 0, nTimeFrame, buf);

    timeMux3::buildLatchTransWithTime(pNtkRes, nTimeFrame, logger);
    buildTM(pNtk, pNtkRes, oPerm, nTimeFrame, nPi, logger);

    if(verbose) {
        cout << "oPerm: ";
        for(uint i=0; i<Abc_NtkCoNum(pNtk); ++i)
            cout << oPerm[i] << " ";
        cout << "\n";
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    if(logger) {
        logger->log("free memory");
        delete logger;
    }

    return pNtkRes;
}

void checkEqv(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, int *oPerm, cuint nTimeFrame)
{
    Abc_Ntk_t *pNtk1 = Abc_NtkDup(pNtkComb);
    Abc_Ntk_t *pNtk2 = Abc_NtkFrames(pNtkMux, nTimeFrame, 1, 0);

    int *oPerm2 = timeMux2::extendPerm(oPerm, Abc_NtkPoNum(pNtk1), Abc_NtkPoNum(pNtk2));
    aigPermCo(pNtk2, oPerm2);
    pNtk2 = aigCone(pNtk2, 0, Abc_NtkPoNum(pNtk1), true);

    cout << "EC status: ";
    Abc_NtkShortNames(pNtk1);
    Abc_NtkShortNames(pNtk2);
    Abc_NtkCecFraig(pNtk1, pNtk2, 50, 0);

    delete [] oPerm2;
    Abc_NtkDelete(pNtk1);
    Abc_NtkDelete(pNtk2);
}

} // end namespace timeMux4