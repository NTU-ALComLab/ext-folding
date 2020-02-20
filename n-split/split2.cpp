#include "ext-folding/n-split/split.h"

using namespace nSplit2;

namespace nSplit2
{

int Split2_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    Abc_Ntk_t *pNtk;
    int mPi = 100, mPo = 30, mNode = 1000;
    bool verbose = true, cec = false;
    string folder;
    char buf[100];
    vector<Abc_Ntk_t*> splitNtks;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "ioncvh")) != EOF) {
        switch(c) {
        case 'i':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-i\" should be followed by an integer.\n");
                goto usage;
            }
            mPi = atoi(argv[globalUtilOptind++]);
            if(mPi <= 0) goto usage;
            break;
        case 'o':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-i\" should be followed by an integer.\n");
                goto usage;
            }
            mPo = atoi(argv[globalUtilOptind++]);
            if(mPo <= 0) goto usage;
            break;
        case 'n':
            if(globalUtilOptind >= argc) {
                Abc_Print(-1, "Command line switch \"-n\" should be followed by an integer.\n");
                goto usage;
            }
            mNode = atoi(argv[globalUtilOptind++]);
            if(mNode <= 0) goto usage;
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

    if(globalUtilOptind < argc) {
        folder =  string(argv[globalUtilOptind]);
        if(folder.back() != '/') folder.push_back('/');
    }

    // get pNtk
    pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return 1;
    }
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);

    splitNtks = aigSplit2(pNtk, mPi, mPo, mNode);
    for(Abc_Ntk_t *_pNtk: splitNtks) {
        sprintf(buf, "%s%s.blif", folder.c_str(), _pNtk->pName);
        cout << "writing " << buf << endl;
        Io_Write(_pNtk, buf, IO_FILE_BLIF);
    }

    //if(cec) checkEqv(pNtk, splitNtks);

    Abc_NtkDelete(pNtk);
    for(Abc_Ntk_t *_pNtk: splitNtks) Abc_NtkDelete(_pNtk);
    
    return 0;

usage:
    Abc_Print(-2, "usage: split2 [-i <num>] [-o <num>] [-n <num>] [-cvh] <blif_prefix>\n");
    Abc_Print(-2, "\t             splits the circuit into pieces, each within size limit\n");
    Abc_Print(-2, "\t-i         : maximum number of PIs [default = %d]\n", mPi);
    Abc_Print(-2, "\t-o         : maximum number of POs [default = %d]\n", mPo);
    Abc_Print(-2, "\t-n         : maximum number of nodes [default = %d]\n", mNode);
    Abc_Print(-2, "\t-c         : toggles equivalence checking with the original circuit [default = %s]\n", cec ? "on" : "off");
    Abc_Print(-2, "\t-v         : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    Abc_Print(-2, "\tfolder     : (optional) output blif file name prefix, if not specified, uses circuit name as default\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "split2", Split2_Command, 0);
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
} Split2_registrar;

} // end namespace nSplit2