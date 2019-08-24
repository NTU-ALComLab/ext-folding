#include "ext-folding/timeFold/utils.h"

using namespace std;
using namespace bddUtils;
//using namespace aigUtils;
using namespace fileWrite;

namespace timeFold
{

int tFold_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    int c;
    Abc_Ntk_t *pNtk;
    bool mode = false, verbosity = true;
    vector<string> stg;
    ostream *fp;
    int nTimeFrame = -1, nSts = -1;
    size_t nCi, nPi, nCo, nPo;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tmvh")) != EOF) {
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
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    
    nCi = Abc_NtkCiNum(pNtk);     // #input after time-frame expansion
    nPi = nCi / nTimeFrame;       // #Pi of the original sequential circuit
    nCo = Abc_NtkCoNum(pNtk);     // #output after time-frame expansion
    nPo = nCo / nTimeFrame;       // #Po of the original sequential circuit
    assert(nCi == nPi * nTimeFrame);
    assert(nCo == nPo * nTimeFrame);

    if(!mode) nSts = bddFold(pNtk, nTimeFrame, stg, verbosity);
    //else nSts = aigFold(pNtk, nTimeFrame, stg, verbosity);
    
    if(nSts > 0) writeKiss(nPi, nPo, nSts, stg, *fp);
    else cerr << "something went wrong in timefold!!";
    
    if(fp != &cout) delete fp;
    Abc_NtkDelete(pNtk);

    return 0;

usage:
    Abc_Print(-2, "usage: timefold [-t <num>] [-mv] <file>\n");
    Abc_Print(-2, "\t        fold the time-frame expanded circuit and transform it into a STG\n");
    Abc_Print(-2, "\t-t    : number of time-frames to be folded\n");
    Abc_Print(-2, "\t-m    : toggles methods for cut set enumeration [default = %s]\n", mode ? "aig" : "bdd");
    Abc_Print(-2, "\t-v    : toggles verbosity [default = %s]\n", verbosity ? "on" : "off");
    Abc_Print(-2, "\tfile  : (optional) output kiss file name\n");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "tFold", "timefold", tFold_Command, 0);
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
} timeFold_registrar;

} // unnamed namespace