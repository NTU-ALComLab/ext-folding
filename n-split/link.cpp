#include "ext-folding/n-split/split.h"

using namespace nSplit;

namespace nSplit
{

int Link_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c, nTimeFrame = -1;
    InfoPairs infos;
    string topNtk;
    bool verbose = true;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "trvh")) != EOF) {
        switch(c) {
        case 't':
            nTimeFrame = atoi(argv[globalUtilOptind++]);
            break;
        case 'r':
            infos.push_back(make_pair(string(argv[globalUtilOptind++]), string(argv[globalUtilOptind++])));
            break;
        case 'v':
            verbose = !verbose;
            break;
        case 'h': default:
            goto usage;
        }
    }

    topNtk = string(argv[globalUtilOptind++]);



    return 0;

usage:
    return 1;
}

// called during ABC startup
void init2(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "link", Link_Command, 0);
}

// called during ABC termination
void destroy2(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer2 = {init2, destroy2};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar2
{
    registrar2() 
    {
        Abc_FrameAddInitializer(&frame_initializer2);
    }
} Link_registrar;

} // end namespace nSplit