#include "ext-folding/timeMux3/timeMux3.h"
#include "ext-folding/timeMux4/timeMux4.h"

#include <unordered_map>
#include <unordered_set>

extern "C"
{
void Abc_NtkCecFraig(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, int nSeconds, int fVerbose);
}

using namespace timeMux4;
using namespace aigUtils;

namespace timeMux4
{

void buildTM(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, cuint nTimeFrame, cuint nPi)
{
    unordered_map<Abc_Obj_t*, Abc_Obj_t*> vMap;
    vector<Abc_Obj_t*> que;
    vector<vector<Abc_Obj_t*> > poFuncs;
    poFuncs.resize(nTimeFrame);
    //Abc_Obj_t *pConst0 = Abc_ObjNot(Abc_AigConst1(pNtkMux));

    Abc_AigConst1(pNtkComb)->pCopy = Abc_AigConst1(pNtkMux);

    for(uint t=0; t<nTimeFrame; ++t) {
        assert(que.empty());
        for(uint i=0; i<nPi; ++i) {
            Abc_Obj_t *pObj = Abc_NtkPi(pNtkComb, t*nPi+i);
            pObj->pCopy = Abc_NtkPi(pNtkMux, i);
            que.push_back(pObj);
        }

        while(!que.empty()) {
            uint i;
            Abc_Obj_t *pObj = que.back(), *pFanout;
            que.pop_back();

            Abc_ObjForEachFanout(pObj, pFanout, i) {
                if(Abc_ObjIsPo(pFanout)) {
                    poFuncs[t].push_back(Abc_ObjChild0Copy(pFanout));
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

        // store things in latches
        unordered_set<Abc_Obj_t*> vSet;  vSet.reserve(vMap.size());
        for(auto p: vMap)
            vSet.insert(p.second);
        for(Abc_Obj_t *pObj: vSet) {
            ;
        }
    }
}

void buildPoFuncs(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, cuint nTimeFrame, cuint nPi, cuint nPo, TimeLogger *logger)
{
    assert(Abc_NtkPoNum(pNtkComb) == Abc_NtkPoNum(pNtkMux));
    Abc_Obj_t *pObj;  uint i;

    Abc_AigConst1(pNtkComb)->pCopy = Abc_AigConst1(pNtkMux);
    for(int i=0; i<(nTimeFrame-1)*nPi; ++i)
        Abc_NtkPi(pNtkComb, i)->pCopy = Abc_ObjFanout0(Abc_NtkBox(pNtkMux, i+nTimeFrame));
    for(int i=0; i<nPi; ++i)
        Abc_NtkPi(pNtkComb, (nTimeFrame-1)*nPi+i)->pCopy = Abc_NtkPi(pNtkMux, i);
    
    Abc_AigForEachAnd(pNtkComb, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkMux->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachPo(pNtkMux, pObj, i) {
        Abc_Obj_t *pPoComb, *pConst0, *pCtrl, *pPoMux;
        pPoComb = Abc_ObjChild0Copy(Abc_NtkPo(pNtkComb, i));
        pConst0 = Abc_ObjNot(Abc_AigConst1(pNtkMux));
        pCtrl = Abc_ObjFanout0(Abc_NtkBox(pNtkMux, nTimeFrame-1));
        pPoMux = Abc_AigMux((Abc_Aig_t*)pNtkMux->pManFunc, pCtrl, pPoComb, pConst0);
        Abc_ObjAddFanin(pObj, pPoMux);
    }

    if(logger) logger->log("buildPoFuncs");
}

Abc_Ntk_t* aigStrMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, const bool verbose, const char *logFileName)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;

    // get basic settings
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nPi = nCi / nTimeFrame;

    char buf[100];
    sprintf(buf, "%s-tm%u", pNtk->pName, nTimeFrame);
    Abc_Ntk_t *pNtkRes = aigInitNtk(nPi, 0, nTimeFrame, buf);

    timeMux3::buildLatchTransWithTime(pNtkRes, nTimeFrame, logger);
    //buildPiStorage(pNtkRes, nPi, nTimeFrame, logger);
    //buildPoFuncs(pNtk, pNtkRes, nTimeFrame, nPi, nCo, logger);

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    if(logger) {
        logger->log("free memory");
        delete logger;
    }

    return pNtkRes;
}

void checkEqv(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, cuint nTimeFrame)
{
    Abc_Ntk_t *pNtk1 = Abc_NtkDup(pNtkComb);
    Abc_Ntk_t *pNtk2 = Abc_NtkFrames(pNtkMux, nTimeFrame, 1, 0);

    // retrieve the outputs from the last time-frame
    cuint nPo = Abc_NtkPoNum(pNtkMux);
    assert(Abc_NtkPoNum(pNtk2) == nPo*nTimeFrame);
    pNtk2 = aigCone(pNtk2, Abc_NtkPoNum(pNtk2)-nPo, Abc_NtkPoNum(pNtk2), true);

    cout << "EC status: ";
    Abc_NtkShortNames(pNtk1);
    Abc_NtkShortNames(pNtk2);
    Abc_NtkCecFraig(pNtk1, pNtk2, 50, 0);

    Abc_NtkDelete(pNtk1);
    Abc_NtkDelete(pNtk2);
}

} // end namespace timeMux4