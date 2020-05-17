#include "ext-folding/utils/utils.h"

using namespace utils;

namespace voter
{

int BuildVoter_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int n;
    Abc_Ntk_t *pNtk;

    if(argc != 2) goto usage;
    
    n = atoi(argv[1]);
    if(n <= 0) goto usage;

    pNtk = aigUtils::aigBuildVoter(n);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
    
    return 0;

usage:
    Abc_Print(-2, "usage: build_voter <num>\n");
    Abc_Print(-2, "\t         builds a n-bit majority voter circuit\n");
    Abc_Print(-2, "\tnum    : number of PI\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "build_voter", BuildVoter_Command, 0);
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