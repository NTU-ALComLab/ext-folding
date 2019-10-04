#include "ext-folding/timeMux2/timeMux2.h"
#include "ext-folding/timeMux/timeMux.h"

#include <unistd.h>
#include <cstdio>

extern "C"
{
void Abc_NtkCecFraig(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, int nSeconds, int fVerbose);
}

namespace timeMux2
{

// prepare network for CEC
static Abc_Ntk_t* prepNtkToCheck(cuint nTimeFrame, cuint nCo, int *iPerm, int *oPerm, cuint pid)
{
    assert(iPerm && oPerm);

    // read the BLIF file
    char buf[100];
    sprintf(buf, "%u-mstg.blif", pid);
    Abc_Ntk_t *pNtk = Io_Read(buf, Io_ReadFileType(buf), 1, 0);
    remove(buf);

    // strashing
    Abc_Ntk_t *pNtkStr = Abc_NtkStrash(pNtk, 0, 0, 0);
    Abc_NtkDelete(pNtk);

    // perform time-frame expansion
    pNtk = Abc_NtkFrames(pNtkStr, nTimeFrame, 1, 0);
    Abc_NtkDelete(pNtkStr);

    // permute PIs
    pNtk = aigUtils::aigPermCi(pNtkStr, iPerm);
    Abc_NtkDelete(pNtkStr);

    // permute POs
    aigUtils::aigPermCo(pNtk, oPerm);

    // retrieve the outputs from the last time-frame
    pNtkStr = aigUtils::aigCone(pNtk, 0, nCo);
    Abc_NtkDelete(pNtk);

    return pNtkStr;
}

static int* extendPerm(int *perm, cuint fromSize, cuint toSize)
{
    assert(perm && (fromSize <= toSize));
    vector<bool> v(toSize, false);
    int *ret = new int[toSize];
    for(uint i=0; i<fromSize; ++i) {
        v[perm[i]] = true;
        ret[i] = perm[i];
    }
    uint pos = fromSize;
    for(uint i=0; i<toSize; ++i)
        if(!v[i]) ret[pos++] = i;
    assert((pos == toSize) && checkPerm(ret, toSize, toSize));
    return ret;
}

void checkEqv(Abc_Ntk_t *pNtk, int *iPerm, int *oPerm, cuint nTimeFrame, STG *stg)
{
    uint pid = (uint)getpid();  // name the tmp dump files with pid to avoid name collision

    timeMux::dumpSTG(pNtk, nTimeFrame, stg, pid);
    timeMux::dumpBLIF(pid);

    int *oPerm2 = extendPerm(oPerm, stg->getNCo(), stg->getNPo()*nTimeFrame);

    Abc_Ntk_t *pNtkCheck = prepNtkToCheck(nTimeFrame, stg->getNCo(), iPerm, oPerm2, pid);
    
    //cout << "prep done, start checking..." << endl;
    //if(perm) for(uint i=0; i<Abc_NtkCiNum(pNtk); ++i)
    //    cout << i << " -> " << perm[i] << endl;;

    // perform CEC and print message
    cout << "EC status: ";
    Abc_NtkShortNames(pNtk);
    Abc_NtkShortNames(pNtkCheck);
    Abc_NtkCecFraig(pNtk, pNtkCheck, 50, 0);

    Abc_NtkDelete(pNtkCheck);
    delete [] oPerm2;
}

} // end namespace timeMux2