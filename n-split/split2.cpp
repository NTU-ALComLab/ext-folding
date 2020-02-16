#include "ext-folding/n-split/split.h"

using namespace nSplit2;

namespace nSplit2
{

int Split_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c, n = 2;
    Abc_Ntk_t *pNtk;
    bool verbose = true, cec = false;
    char *prefix = NULL, buf[100];
    vector<Abc_Ntk_t*> splitNtks;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "ncvh")) != EOF) {
        switch(c) {
        case 'n':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-n\" should be followed by an integer.\n");
                goto usage;
            }
            n = atoi(argv[globalUtilOptind++]);
            if(n < 0) goto usage;
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
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);

    splitNtks = aigSplit(pNtk, n);
    prefix = (globalUtilOptind < argc) ? argv[globalUtilOptind] : pNtk->pName;

    for(c=0; c<n; ++c) {
        sprintf(buf, "%s-%d.blif", prefix, c);
        Io_Write(splitNtks[c], buf, IO_FILE_BLIF);
    }
    sprintf(buf, "%s-top.blif", prefix);
    Io_Write(splitNtks.back(), buf, IO_FILE_BLIF);

    if(cec) checkEqv(pNtk, splitNtks);

    Abc_NtkDelete(pNtk);
    for(Abc_Ntk_t *_pNtk: splitNtks) Abc_NtkDelete(_pNtk);

    return 0;

usage:
    Abc_Print(-2, "usage: split2 [-i <num>] [-o <num>] [-n <num>] [-vh] <blif_prefix>\n");
    Abc_Print(-2, "\t             splits the circuit into n+1 pieces\n");
    Abc_Print(-2, "\t-n         : number of pieces [default = %d]\n", n);
    Abc_Print(-2, "\t-c         : toggles equivalence checking with the original circuit [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-v         : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    Abc_Print(-2, "\tblif_prefix: (optional) output blif file name prefix, if not specified, uses circuit name as default\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "split", Split_Command, 0);
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
} Split_registrar;

} // end namespace nSplit2