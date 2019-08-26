#include "ext-folding/timeMux/utils.h"

namespace timeMux
{

// TODO: verify equivalence
int tMux_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk;
    bool mode = false, reOrd = false, verbosity = true;
    vector<string> stg;
    ostream *fp;
    int nTimeFrame = -1, nSts = -1;
    size_t nCi, nPi, nCo;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tmrvh")) != EOF) {
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
            reOrd = !reOrd;
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

    nCo = Abc_NtkCoNum(pNtk);     // #output
    nCi = Abc_NtkCiNum(pNtk);     // #input
    nPi = nCi / nTimeFrame;       // #Pi of the sequential circuit
    assert(nCi == nPi * nTimeFrame);

    if(!mode) nSts = timeMux::bddUtils::bddMux(pNtk, nTimeFrame, stg, reOrd, verbosity);
    else cerr << "AIG mode currently not supported." << endl;
    //else nSts = aigUtils::aigFold(pNtk, nTimeFrame, stg, verbosity);
    
    if(nSts > 0) timeFold::fileWrite::writeKiss(nPi, nCo, nSts, stg, *fp);
    else cerr << "Something went wrong in time_mux!!" << endl;
    
    if(fp != &cout) delete fp;
    Abc_NtkDelete(pNtk);

    return 0;

usage:
    Abc_Print(-2, "usage: time_mux [-t <num>] [-mrv] <file>\n");
    Abc_Print(-2, "\t        todo\n");
    Abc_Print(-2, "\t-t    : number of time-frames to be folded\n");
    Abc_Print(-2, "\t-m    : toggles methods for cut set enumeration [default = %s]\n", mode ? "AIG" : "BDD");
    Abc_Print(-2, "\t-r    : toggles reordering of circuit inputs [default = %s]\n", reOrd ? "on" : "off");
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