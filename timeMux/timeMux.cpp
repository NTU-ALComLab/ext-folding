#include "ext-folding/timeMux/timeMux.h"

using namespace timeMux;

namespace timeMux
{

// 1. make the network purely combinational by converting latches to PI/POs
// 2. introduce dummy PIs to make #PI be the mulltiples of nTimeFrame
static Abc_Ntk_t* ntkPrepro(Abc_Ntk_t *pNtk, cuint nTimeFrame)
{
    Abc_Ntk_t *pNtkDup = Abc_NtkStrash(pNtk, 0, 0, 0);
    Abc_AigCleanup((Abc_Aig_t*)pNtkDup->pManFunc);
    int i;  Abc_Obj_t *pObj, *pObjNew;
    
    Abc_Ntk_t *pNtkRes = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtkRes->pName = Extra_UtilStrsav(pNtkDup->pName);
    
    // copy network
    Abc_AigConst1(pNtkDup)->pCopy = Abc_AigConst1(pNtkRes);
    Abc_NtkForEachCi(pNtkDup, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkRes);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    Abc_AigForEachAnd(pNtkDup, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachCo(pNtkDup, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    Abc_NtkDelete(pNtkDup);

    // rounding up #PI
    size_t n = Abc_NtkPiNum(pNtkRes) % nTimeFrame;
    char buf[1000];
    if(n) for(size_t i=0; i<nTimeFrame-n; ++i) {
        sprintf(buf, "dummy_%lu", i);
        Abc_ObjAssignName(Abc_NtkCreatePi(pNtkRes), buf, NULL);
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));
    return pNtkRes;
}

int tMux_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk;
    bool mode = false, cec = false, verbosity = true;
    size_t *perm = NULL;

    vector<string> stg;
    ostream *fp;
    int nTimeFrame = -1, nSts = -1;
    size_t nCi, nPi, nCo;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tmrcvh")) != EOF) {
        switch(c) {
        case 't':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-t\" should be followed by an integer.\n");
                goto usage;
            }
            nTimeFrame = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if(nTimeFrame < 0)
                goto usage;
            break;
        case 'm':
            mode = !mode;
            break;
        case 'r':
            perm = (size_t*)((size_t)perm ^ 1);
            break;
        case 'c':
            cec = !cec;
            break;
        case 'v':
            verbosity = !verbosity;
            break;
        case 'h': default:
            goto usage;
        }
    }
    
    fp = (globalUtilOptind < argc) ? (new ofstream(argv[globalUtilOptind])) : &cout;
    
    // get pNtk
    pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return 1;
    }
    //pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    pNtk = ntkPrepro(pNtk, nTimeFrame);

    nCo = Abc_NtkCoNum(pNtk);     // #output
    nCi = Abc_NtkCiNum(pNtk);     // #input
    nPi = nCi / nTimeFrame;       // #Pi of the sequential circuit
    assert(nCi == nPi * nTimeFrame);
    if(perm) perm = new size_t[nCi];

    if(!mode) nSts = bddMux(pNtk, nTimeFrame, stg, perm, verbosity);
    else cerr << "AIG mode currently not supported." << endl;
    //else nSts = aigFold(pNtk, nTimeFrame, stg, verbosity);
    
    if(nSts > 0) {
        fileWrite::writeKiss(nPi, nCo, nSts, stg, *fp);
        if(cec) checkEqv(pNtk, perm, nTimeFrame, stg, nSts); 
    } else cerr << "Something went wrong in time_mux!!" << endl;
    
    if(fp != &cout) delete fp;
    if(perm) delete [] perm;
    Abc_NtkDelete(pNtk);

    return 0;

usage:
    Abc_Print(-2, "usage: time_mux [-t <num>] [-mrcv] <file>\n");
    Abc_Print(-2, "\t        todo\n");
    Abc_Print(-2, "\t-t    : number of time-frames to be folded\n");
    Abc_Print(-2, "\t-m    : toggles methods for cut set enumeration [default = %s]\n", mode ? "AIG" : "BDD");
    Abc_Print(-2, "\t-r    : toggles reordering of circuit inputs [default = %s]\n", perm ? "on" : "off");
    Abc_Print(-2, "\t-c    : toggles equivalence checking with the original circuit [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-v    : toggles verbosity [default = %s]\n", verbosity ? "on" : "off");
    Abc_Print(-2, "\tfile  : (optional) output kiss file name\n");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "time_mux", tMux_Command, 0);
}

// called during ABC termination
void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer);
    }
} timeMux_registrar;

} // end namespace timeMux