#include "ext-folding/timeFold/timeFold.h"
#include "ext-folding/timeMux/timeMux.h"

using namespace timeFold;

namespace timeFold
{

int tFold_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk;
    bool mode = false, verbose = true, cec = false;
    char *logFileName = NULL;
    STG *stg;
    ostream *fp;
    int nTimeFrame = -1;
    uint nCi, nPi, nCo, nPo;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tlcvh")) != EOF) {
        switch(c) {
        case 't':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-t\" should be followed by an integer.\n");
                goto usage;
            }
            nTimeFrame = atoi(argv[globalUtilOptind++]);
            if(nTimeFrame <= 0) goto usage;
            break;
        case 'l':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-l\" should be followed by a string.\n");
                goto usage;
            }
            logFileName = Extra_UtilStrsav(argv[globalUtilOptind++]);
            break;
        //case 'm':
        //    mode = !mode;
        //    break;
        case 'c':
            cec = !cec;
            break;
        case 'v':
            verbose = !verbose;
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

    if(!mode) stg = bddFold(pNtk, nTimeFrame, verbose, logFileName);
    else cerr << "AIG mode currently not supported." << endl;
    //else nSts = aigUtils::aigFold(pNtk, nTimeFrame, stg, verbose);
    
    if(stg) {
        stg->write(*fp);  //fileWrite::writeKiss(nPi, nPo, nSts, stg, *fp);
        if(cec) timeMux::checkEqv(pNtk, NULL, nTimeFrame, stg, false); 
    } else cerr << "Something went wrong in time_fold!!" << endl;
    
    if(fp != &cout) delete fp;
    Abc_NtkDelete(pNtk);
    if(stg) delete stg;

    return 0;

usage:
    Abc_Print(-2, "usage: time_fold [-t <num>] [-l <log_file>] [-cvh] <kiss_file>\n");
    Abc_Print(-2, "\t             performs time-frame folding on the given iterative (time-frame expanded) circuit and transform it into a FSM in KISS format\n");
    Abc_Print(-2, "\t-t         : the number of time-frames to be folded\n");
    Abc_Print(-2, "\t-l         : toggles logging of the runtime [default = %s]\n", logFileName ? "on" : "off");
  //Abc_Print(-2, "\t-m         : toggles methods for cut set enumeration [default = %s]\n", mode ? "AIG" : "BDD");
    Abc_Print(-2, "\t-c         : toggles equivalence checking with the original circuit [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-v         : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h         : prints the command usage\n");
    Abc_Print(-2, "\tkiss_file  : (optional) output KISS file name [default = stdout]\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Circuit Folding", "time_fold", tFold_Command, 0);
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

} // end namespace timeFold