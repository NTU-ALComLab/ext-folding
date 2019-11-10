#include "ext-folding/timeMux3/timeMux3.h"


using namespace timeMux3;
using namespace aigUtils;

namespace timeMux3
{

void buildLatchTransWithTime(Abc_Ntk_t *pNtk, cuint nTimeFrame)
{
    assert(Abc_NtkBoxNum(pNtk) == nTimeFrame);
    Abc_Obj_t *pObj = Abc_NtkBox(pNtk, 0);
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


    

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));

    return pNtkRes;
}

} // end namespace timeMux3