#include "ext-folding/utils/utils.h"

using namespace utils;

namespace voter
{

int BuildVoter_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c, n;
    Abc_Ntk_t *pNtk;
    bool tieVal = true;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "vh")) != EOF) {
        switch(c) {
        case 'v':
            tieVal = !tieVal;
            break;
        case 'h': default:
            goto usage;
        }
    }

    if(globalUtilOptind >= argc) goto usage;
    n = atoi(argv[globalUtilOptind]);
    if(n <= 0) goto usage;

    pNtk = aigUtils::aigBuildVoter(n, tieVal);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
    
    return 0;

usage:
    Abc_Print(-2, "usage: build_voter [-vh] <num>\n");
    Abc_Print(-2, "\t         builds a n-bit majority voter circuit\n");
    Abc_Print(-2, "\t-v     : (optional) toggles the output value to break tie when there are plural primary inputs [default = %s]\n", tieVal ? "1" : "0");
    Abc_Print(-2, "\t-h     : print the command usage\n");
    Abc_Print(-2, "\tnum    : number of PI\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "build_voter", BuildVoter_Command, 1);
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
} buildVoter_registrar;

} // end namespace voter