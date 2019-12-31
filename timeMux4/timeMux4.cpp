#include "ext-folding/timeMux4/timeMux4.h"

using namespace timeMux4;

namespace timeMux4
{

int tMux4_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk, *pNtkRes;
    bool cec = false, verbose = true;
    char *logFileName = NULL, *outFileName = NULL;

    int nTimeFrame = -1;
    uint nCi, nPi, nCo;
    int *oPerm, *iPerm = NULL;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tlopcvh")) != EOF) {
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
        case 'o':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-o\" should be followed by a string.\n");
                goto usage;
            }
            outFileName = Extra_UtilStrsav(argv[globalUtilOptind++]);
            break;
        case 'p':
            iPerm = (int*)1;
            break;
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

    if(iPerm) {
        iPerm = new int[nCi];
        pNtk = permPi(pNtk, iPerm, verbose);
    }
    oPerm = new int[nCo];
    pNtkRes = aigStrMux(pNtk, nTimeFrame, oPerm, verbose, logFileName);
    
    if(pNtk) {
        if(outFileName) Io_Write(pNtkRes, outFileName, IO_FILE_BLIF); // write file
        if(cec) checkEqv(pNtk, pNtkRes, oPerm, nTimeFrame); 
    } else cerr << "Something went wrong in time_mux4!!" << endl;

    
    if(logFileName) ABC_FREE(logFileName);
    if(outFileName) ABC_FREE(outFileName);
    Abc_NtkDelete(pNtk);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes);
    if(iPerm) delete [] iPerm;
    delete [] oPerm;

    return 0;

usage:
    Abc_Print(-2, "usage: time_mux4 [-t <num>] [-l <log_file>] [-o <out_file>] [-pcvh]\n");
    Abc_Print(-2, "\t             time multiplexing with structural approach and pin sharing\n");
    Abc_Print(-2, "\t-t         : number of time-frames\n");
    Abc_Print(-2, "\t-l         : (optional) toggles logging of the runtime [default = %s]\n", logFileName ? "on" : "off");
    Abc_Print(-2, "\t-o         : (optional) toggles whether to write the circuit into the specified file [default = %s]\n", outFileName ? "on" : "off");
    Abc_Print(-2, "\t-p         : toggles the permutation of circuit inputs [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-c         : toggles equivalence checking with the original circuit [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-v         : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "time_mux4", tMux4_Command, 0);
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
} timeMux4_registrar;

} // end namespace timeMux4