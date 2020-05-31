#include "base/main/main.h"
#include "base/main/mainInt.h"

using namespace std;

namespace kissToBlif
{

extern void natEnc(const char *inKiss, const char *outBlif, const bool dcVal);
extern void oneHotEnc(const char *inKiss, const char *outBlif, const bool dcVal);

int KissToBlif_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    bool enc = true, replace = false, dcVal = false;
    char *inKiss, *outBlif;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "derh")) != EOF) {
        switch(c) {
        case 'd':
            dcVal = !dcVal;
            break;
        case 'e':
            enc = !enc;
            break;
        case 'r':
            replace = !replace;
            break;
        case 'h': default:
            goto usage;
        }
    }
    
    // get i/o file name
    if(globalUtilOptind + 2 != argc) goto usage;
    inKiss = argv[globalUtilOptind++];
    outBlif = argv[globalUtilOptind++];
    assert(inKiss && outBlif);

    // file conversion
    if(enc) natEnc(inKiss, outBlif, dcVal);
    else oneHotEnc(inKiss, outBlif, dcVal);

    // replace current network
    if(replace)
        Abc_FrameReplaceCurrentNetwork(pAbc, Io_Read(outBlif, IO_FILE_BLIF, 1, 0));
    
    return 0;

usage:
    Abc_Print(-2, "usage: kiss_to_blif [-derh] <in.kiss> <out.blif>\n");
    Abc_Print(-2, "\t             converts a STG in kiss format into a sequential circuit in blif format\n");
    Abc_Print(-2, "\t-d         : the replacing value of don't care outputs [default = %s]\n", dcVal ? "1" : "0");
    Abc_Print(-2, "\t-e         : the encoding scheme of the states [default = %s]\n", enc ? "natural" : "one-hot");
    Abc_Print(-2, "\t-r         : toggles whether to replace current network [default = %s]\n", replace ? "on" : "off");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    Abc_Print(-2, "\tin.kiss    : input kiss file\n");
    Abc_Print(-2, "\tout.blif   : output blif file\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "kiss_to_blif", KissToBlif_Command, 1);
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
} kissToBlif_registrar;

} // end namespace kissToBlif