#include "ext-folding/utils/utils.h"

using namespace utils;

namespace parity
{

int BuildParity_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c, n;
    Abc_Ntk_t *pNtk;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "h")) != EOF) {
        switch(c) {
        case 'h': default:
            goto usage;
        }
    }

    if(globalUtilOptind >= argc) goto usage;
    n = atoi(argv[globalUtilOptind]);
    if(n <= 0) goto usage;

    pNtk = aigUtils::aigBuildParity(n);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
    
    return 0;

usage:
    Abc_Print(-2, "usage: build_parity [-h] <num>\n");
    Abc_Print(-2, "\t         builds a n-bit parity circuit\n");
    Abc_Print(-2, "\t-h     : print the command usage\n");
    Abc_Print(-2, "\tnum    : number of PI\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "build_parity", BuildParity_Command, 1);
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
} buildParity_registrar;

} // end namespace parity