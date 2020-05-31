#include "ext-folding/utils/utils.h"

using namespace std;

namespace cmerge
{

int CMerge_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c, nNtks;
    Abc_Ntk_t *pNtk = Abc_FrameReadNtk(pAbc), *pNtkRes, **pNtks;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "h")) != EOF) {
        switch(c) {
        case 'h': default:
            goto usage;
        }
    }

    nNtks = argc - 1;
    if(pNtk) nNtks++;

    c = 0;
    pNtks = new Abc_Ntk_t*[nNtks];
    if(pNtk) pNtks[c++] = Abc_NtkStrash(pNtk, 0, 0, 0);

    for(uint i=1; i<argc; ++i) {
        pNtkRes = Io_Read(argv[i], Io_ReadFileType(argv[i]), 1, 0);
        pNtks[c++] = Abc_NtkStrash(pNtkRes, 0, 0, 0);
        Abc_NtkDelete(pNtkRes);
    }
    
    pNtkRes = utils::aigUtils::aigMerge(pNtks, nNtks, true);

    
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes);
    delete [] pNtks;
    return 0;

usage:
    Abc_Print(-2, "usage: cmerge [-h] [<ntk1> <ntk2> ...]\n");
    Abc_Print(-2, "\t        merge the circuits\n");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "cmerge", CMerge_Command, 1);
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
} cmerge_registrar;

} // end namespace cmerge