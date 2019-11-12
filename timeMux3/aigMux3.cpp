#include "ext-folding/timeMux3/timeMux3.h"

extern "C"
{
void Abc_NtkCecFraig(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, int nSeconds, int fVerbose);
}

using namespace timeMux3;
using namespace aigUtils;

namespace timeMux3
{

void buildLatchTransWithTime(Abc_Ntk_t *pNtk, cuint nTimeFrame, TimeLogger *logger)
{
    assert(Abc_NtkBoxNum(pNtk) == nTimeFrame);
    
    for(uint i=0; i<nTimeFrame; ++i) {
        Abc_Obj_t *pObj1 = Abc_NtkBox(pNtk, i), *pObj2;
        if(!i) {  // i == 0
            Abc_LatchSetInit1(pObj1);
            pObj2 = Abc_NtkBox(pNtk, nTimeFrame-1);
        } else
            pObj2 = Abc_NtkBox(pNtk, i-1);
        Abc_ObjAddFanin(Abc_ObjFanin0(pObj1), Abc_ObjFanout0(pObj2));
    }

    if(logger) logger->log("buildLatchTransWithTime");
}

static void buildPiStorage(Abc_Ntk_t *pNtk, cuint nPi, cuint nTimeFrame, TimeLogger *logger)
{
    for(uint t=0; t<nTimeFrame-1; ++t) {
        Abc_Obj_t *pCtrl = Abc_ObjFanout0(Abc_NtkBox(pNtk, t));
        for(uint i=0; i<nPi; ++i) {
            char ln[100], li[100], lo[100];
            sprintf(ln, "l-tf%u_%u", t, i);
            sprintf(li, "li-tf%u_%u", t, i);
            sprintf(lo, "lo-tf%u_%u", t, i);

            Abc_Obj_t *pLat = aigNewLatch(pNtk, 0, ln, li, lo);
            Abc_Obj_t *pPi = Abc_NtkPi(pNtk, i);

            Abc_Obj_t *pLatIn = Abc_AigMux((Abc_Aig_t*)pNtk->pManFunc, pCtrl, pPi, Abc_ObjFanout0(pLat));
            Abc_ObjAddFanin(Abc_ObjFanin0(pLat), pLatIn);
        }
    }
    if(logger) logger->log("buildPiStorage");
}

static void buildPoFuncs(Abc_Ntk_t *pNtkComb, Abc_Ntk_t *pNtkMux, cuint nTimeFrame, cuint nPi, cuint nPo, TimeLogger *logger)
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

    Abc_Obj_t *pConst0 = Abc_ObjNot(Abc_AigConst1(pNtkMux));
    Abc_Obj_t *pCtrl = Abc_ObjFanout0(Abc_NtkBox(pNtkMux, nTimeFrame-1));
    Abc_NtkForEachPo(pNtkMux, pObj, i) {
        Abc_Obj_t *pPoComb, *pPoMux;
        pPoComb = Abc_ObjChild0Copy(Abc_NtkPo(pNtkComb, i));
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
    Abc_Ntk_t *pNtkRes = aigInitNtk(nPi, nCo, nTimeFrame, buf);

    buildLatchTransWithTime(pNtkRes, nTimeFrame, logger);
    buildPiStorage(pNtkRes, nPi, nTimeFrame, logger);
    buildPoFuncs(pNtk, pNtkRes, nTimeFrame, nPi, nCo, logger);

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

} // end namespace timeMux3