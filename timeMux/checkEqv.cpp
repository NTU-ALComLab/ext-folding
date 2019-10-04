#include "ext-folding/timeMux/timeMux.h"

#include <unistd.h>
#include <cstdio>

extern "C"
{
void Abc_NtkCecFraig(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, int nSeconds, int fVerbose);
}

namespace timeMux
{

// dump tmp kiss file and perform minimization
void dumpSTG(Abc_Ntk_t *pNtk, cuint nTimeFrame, STG *stg, cuint pid)
{
    char buf1[100], buf2[1000];
    //cuint nPi = Abc_NtkCiNum(pNtk) / nTimeFrame;
    //cuint nPo = Abc_NtkCoNum(pNtk);
    
    sprintf(buf1, "%u-stg.kiss", pid);
    ofstream fp = ofstream(buf1);
    stg->write(fp);
    //fileWrite::writeKiss(nPi, nPo, nSts, stg, fp);
    fp.close();

    sprintf(buf2, "./bin/MeMin %s %u-mstg.kiss > /dev/null", buf1, pid);
    system(buf2);
    remove(buf1);
}

// convert STG to circuit in BLIF format
void dumpBLIF(cuint pid)
{
    char buf1[100], buf2[1000];
    sprintf(buf1, "%u-mstg.kiss", pid);
    sprintf(buf2, "./bin/kiss2blif %s %u-mstg.blif", buf1, pid);
    system(buf2);
    remove(buf1);
}

// prepare network for CEC
static Abc_Ntk_t* prepNtkToCheck(cuint nTimeFrame, int *perm, cuint pid)
{
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

    // retrieve the outputs from the last time-frame
    uint nPo = Abc_NtkCoNum(pNtk) / nTimeFrame;
    pNtkStr = aigUtils::aigCone(pNtk, Abc_NtkCoNum(pNtk)-nPo, Abc_NtkCoNum(pNtk));
    Abc_NtkDelete(pNtk);

    // permute PIs
    if(!perm) return pNtkStr;
    pNtk = aigUtils::aigPermCi(pNtkStr, perm);
    Abc_NtkDelete(pNtkStr);

    return pNtk;
}

void checkEqv(Abc_Ntk_t *pNtk, int *perm, cuint nTimeFrame, STG *stg)
{
    uint pid = (uint)getpid();  // name the tmp dump files with pid to avoid name collision

    dumpSTG(pNtk, nTimeFrame, stg, pid);
    dumpBLIF(pid);
    Abc_Ntk_t *pNtkCheck = prepNtkToCheck(nTimeFrame, perm, pid);
    
    //cout << "prep done, start checking..." << endl;
    //if(perm) for(uint i=0; i<Abc_NtkCiNum(pNtk); ++i)
    //    cout << i << " -> " << perm[i] << endl;;

    // perform CEC and print message
    cout << "EC status: ";
    Abc_NtkShortNames(pNtk);
    Abc_NtkShortNames(pNtkCheck);
    Abc_NtkCecFraig(pNtk, pNtkCheck, 50, 0);
    Abc_NtkDelete(pNtkCheck);
}

} // end namespace timeMux