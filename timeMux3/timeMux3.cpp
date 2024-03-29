#include "ext-folding/timeMux3/timeMux3.h"

using namespace timeMux3;

namespace timeMux3
{

int tMux3_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk, *pNtkRes;
    bool cec = false, verbose = true;
    char *logFileName = NULL, *outFileName = NULL;

    int nTimeFrame = -1;
    uint nCi, nPi, nCo;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "tlocvh")) != EOF) {
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

    pNtkRes = aigStrMux(pNtk, nTimeFrame, verbose, logFileName);
    
    if(pNtk) {
        if(outFileName) Io_Write(pNtkRes, outFileName, IO_FILE_BLIF); // write file
        if(cec) checkEqv(pNtk, pNtkRes, nTimeFrame); 
    } else cerr << "Something went wrong in time_mux3!!" << endl;

    
    if(logFileName) ABC_FREE(logFileName);
    if(outFileName) ABC_FREE(outFileName);
    Abc_NtkDelete(pNtk);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes);

    return 0;

usage:
    Abc_Print(-2, "usage: time_mux3 [-t <num>] [-l <log_file>] [-o <out_file>] [-cvh]\n");
    Abc_Print(-2, "\t             time multiplexing via structural circuit folding (naive approach by temporarily storing inputs of the first t−1 time-frames into flip-flops and deferring all outputs at the last time-frame, w/o pin sharing)\n");
    Abc_Print(-2, "\t-t         : number of time-frames\n");
    Abc_Print(-2, "\t-l         : (optional) toggles logging of the runtime [default = %s]\n", logFileName ? "on" : "off");
    Abc_Print(-2, "\t-o         : (optional) toggles whether to write the circuit into the specified file [default = %s]\n", outFileName ? "on" : "off");
    Abc_Print(-2, "\t-c         : toggles equivalence checking with the original circuit [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-v         : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Circuit Folding (dev)", "time_mux3", tMux3_Command, 1);
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
} timeMux3_registrar;

} // end namespace timeMux3