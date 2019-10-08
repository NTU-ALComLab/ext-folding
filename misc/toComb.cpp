#include "ext-folding/utils/utils.h"

using namespace std;

namespace toComb
{

int ToComb_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    //bool verbosity = false;
    int num = 1;
    Abc_Ntk_t *pNtk;


    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "h")) != EOF) {
        switch(c) {
        //case 'v':
        //    verbosity = !verbosity;
        //    break;
        case 'h': default:
            goto usage;
        }
    }
    
    // retrieve num
    if(globalUtilOptind < argc) num =  atoi(argv[globalUtilOptind]);
    if(num <= 0) {
        cerr << "num should be a positive integer." << endl;
        goto usage;
    }

    // get pNtk
    pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return 1;
    }

    // taking cone
    pNtk = utils::aigUtils::aigToComb(pNtk, num);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);

    return 0;

usage:
    Abc_Print(-2, "usage: to_comb [-h] <num>\n");
    Abc_Print(-2, "\t        transform current network into a purely combinational circuit, latches will be converted to PI/POs\n");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    //Abc_Print(-2, "\t-v    : toggles verbosity [default = %s]\n", verbosity ? "on" : "off");
    Abc_Print(-2, "\tnum   : (optional) the base of #PI being rounded up to [default = %lu]\n", num);

    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "to_comb", ToComb_Command, 0);
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
} toComb_registrar;

} // end namespace toComb