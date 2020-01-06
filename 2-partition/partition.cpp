#include "ext-folding/utils/utils.h"
#include "ext-folding/2-partition/fm.h"

using namespace fmPart2;

namespace fmPart2
{

int partition2_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk;
    double ratio = 0.5;
    int timeLim = 600, failLim = 16, iterLim = 1000000;
    bool verbose = true;
    char *prefix = NULL;
    CirMgr *cirMgr;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "rtfivh")) != EOF) {
        switch(c) {
        case 'r':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-r\" should be followed by a floating point number.\n");
                goto usage;
            }
            ratio = atof(argv[globalUtilOptind++]);
            if(ratio < 0) goto usage;
            break;
        case 't':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-t\" should be followed by an integer.\n");
                goto usage;
            }
            timeLim = atoi(argv[globalUtilOptind++]);
            if(timeLim < 0) goto usage;
            break;
        case 'f':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-f\" should be followed by an integer.\n");
                goto usage;
            }
            failLim = atoi(argv[globalUtilOptind++]);
            if(failLim < 0) goto usage;
            break;
        case 'i':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-i\" should be followed by an integer.\n");
                goto usage;
            }
            iterLim = atoi(argv[globalUtilOptind++]);
            if(iterLim < 0) goto usage;
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
    pNtk = Abc_NtkDup(pNtk);

    prefix = (globalUtilOptind < argc) ? argv[globalUtilOptind] : pNtk->pName;

    cirMgr = new CirMgr(ratio, timeLim, failLim, iterLim, verbose);
    cirMgr->readCircuit(pNtk);
    cirMgr->partition();
    //cirMgr->reportPartition();
    cirMgr->writeSolution(prefix);
    
    Abc_NtkDelete(pNtk);
    delete cirMgr;
    return 0;

usage:
    Abc_Print(-2, "usage: 2_partition [-r <num>] [-t <num>] [-f <num>] [-i <num>] [-vh] <blif_prefix>\n");
    Abc_Print(-2, "\t             performs 2-way partition on the circuit outputs with FM heuristic to minimize the overlapping input supports\n");
    Abc_Print(-2, "\t-r         : (optional) partitioning ratio [default = %f]\n", ratio);
    Abc_Print(-2, "\t-t         : (optional) timeout limit (in seconds) [default = %d]\n", timeLim);
    Abc_Print(-2, "\t-f         : (optional) fail limit [default = %d]\n", failLim);
    Abc_Print(-2, "\t-i         : (optional) iteration limit [default = %d]\n", iterLim);
    Abc_Print(-2, "\t-v         : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    Abc_Print(-2, "\tblif_prefix: (optional) output blif file name prefix, if not specified, uses circuit name as default\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "2_partition", partition2_Command, 0);
}

// called during ABC termination
void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer = {init, destroy};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer);
    }
} partition2_registrar;

} // end namespace fmPart2