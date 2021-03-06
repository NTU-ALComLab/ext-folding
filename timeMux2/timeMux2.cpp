#include "ext-folding/timeMux2/timeMux2.h"

using namespace timeMux2;

namespace timeMux2
{

int tMux2_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk;
    bool mode = false, cec = false, verbose = true;
    char *logFileName = NULL;

    STG *stg = NULL;
    ostream *fp;
    int nTimeFrame = -1;
    int expConfig = 2; // 0: all heuristics, 1: reord PO, 2: reord PI, 3: none
    int *iPerm, *oPerm;
    uint nCi, nPi, nCo, nPo;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tlecvh")) != EOF) {
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
        case 'e':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-e\" should be followed by an integer.\n");
                goto usage;
            }
            expConfig = atoi(argv[globalUtilOptind++]);
            if((expConfig < 0) || (expConfig > 3)) goto usage;
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
    pNtk = aigUtils::aigToComb(pNtk, nTimeFrame);

    nCo = Abc_NtkCoNum(pNtk);     // #output
    nCi = Abc_NtkCiNum(pNtk);     // #input
    nPi = nCi / nTimeFrame;       // #Pi of the sequential circuit
    assert(nCi == nPi * nTimeFrame);

    if((expConfig == 4) || (expConfig == 5)) {
        assert(nTimeFrame == 34);
        assert(nCi == 68);
        if(expConfig == 4) assert(nCo == 39);
        else assert(nCo == 2);
    }
    
    // init i/o-Perm
    iPerm = new int[nCi];
    oPerm = new int[nCo];

    if(!mode) stg = bddMux2(pNtk, nTimeFrame, nPo, iPerm, oPerm, verbose, logFileName, expConfig);
    else cerr << "AIG mode currently not supported." << endl;
    
    if(stg) {
        stg->write(*fp, iPerm, oPerm);
        // fileWrite is deprecated
        //fileWrite::writePerm(iPerm, nCi, *fp);
        //fileWrite::writePerm(oPerm, nCo, *fp, false);
        //fileWrite::writeKiss(nPi, nPo*nTimeFrame, nSts, stg, *fp);
        if(cec) checkEqv(pNtk, iPerm, oPerm, nTimeFrame, stg); 
    } else cerr << "Something went wrong in time_mux2!!" << endl;
    
    if(fp != &cout) delete fp;
    if(logFileName) ABC_FREE(logFileName);
    Abc_NtkDelete(pNtk);
    delete [] iPerm;
    delete [] oPerm;
    if(stg) delete stg;

    return 0;

usage:
  //Abc_Print(-2, "usage: time_mux2 [-t <num>] [-l <log_file>] [-e <config>] [-mcvh] <kiss_file>\n");
    Abc_Print(-2, "usage: func_fold [-t <num>] [-l <log_file>] [-e <config>] [-cvh] <kiss_file>\n");
    Abc_Print(-2, "\t             time multiplexing via functional circuit folding (w/ PO pin sharing)\n");
    Abc_Print(-2, "\t-t         : the number of time-frames to be folded\n");
    Abc_Print(-2, "\t-l         : toggles logging of the runtime [default = %s]\n", logFileName ? "on" : "off");
    Abc_Print(-2, "\t-e         : toggles experimental configuration (0, 1, 2, 3) [default = %d]\n", expConfig);
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
    Cmd_CommandAdd(pAbc, "Circuit Folding (dev)", "time_mux2", tMux2_Command, 0);
    Cmd_CommandAdd(pAbc, "Circuit Folding", "func_fold", tMux2_Command, 0);
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
} timeMux2_registrar;

} // end namespace timeMux2