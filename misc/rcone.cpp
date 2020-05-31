#include "ext-folding/utils/utils.h"

using namespace std;

namespace rcone
{

int RCone_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    int st = -1, ed = -1, nPo;
    Abc_Ntk_t *pNtk;


    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "h")) != EOF) {
        switch(c) {
        case 'h': default:
            goto usage;
        }
    }
    
    // retrieve start
    if(globalUtilOptind >= argc) {
        cerr << "Please specify the starting index." << endl;
        goto usage;
    }
    st = atoi(argv[globalUtilOptind++]);
    if(st < 0) goto usage;

    // retrieve end
    if(globalUtilOptind >= argc) ed = st + 1;
    else ed = atoi(argv[globalUtilOptind]);
    if((ed < 0) || (st >= ed)) goto usage;

    // get pNtk
    pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return 1;
    }

    nPo = Abc_NtkPoNum(pNtk);
    if((st >= nPo) || (ed > nPo)) {
        cerr << "index out of range." << endl;
        goto usage;
    }

    // taking cone
    pNtk = utils::aigUtils::aigCone(pNtk, st, ed);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);

    return 0;

usage:
    Abc_Print(-2, "usage: rcone [-h] <start> <end>\n");
    Abc_Print(-2, "\t        replaces the current network by logic cone of POs from indices [start, end)\n");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    Abc_Print(-2, "\tstart : the PO index to start with (inclusive)\n");
    Abc_Print(-2, "\tend   : (optional) the PO index to end with (exclusive), if not specified, it will be set to start+1 as default\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "rcone", RCone_Command, 1);
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
} rcone_registrar;

} // end namespace rcone