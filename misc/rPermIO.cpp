#include "ext-folding/utils/utils.h"
#include <algorithm>

using namespace std;
using namespace utils;

namespace rPermIO
{

static inline int* initPerm(cuint size)
{
    int *perm = new int[size];
    for(uint i=0; i<size; ++i)
        perm[i] = i;
    random_shuffle(perm, perm + size);
    return perm;
}

static inline void printPerm(int *perm, cuint size)
{
    for(uint i=0; i<size; ++i)
        cout << perm[i] << ((i==size-1) ? "\n" : " ");
}

int RPermIO_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    int *iPerm = (int*)1;
    int *oPerm = (int*)1;
    bool verbose = false;
    Abc_Ntk_t *pNtk;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "iovh")) != EOF) {
        switch(c) {
        case 'i':
            iPerm = (int*)((size_t)iPerm ^ 1);
            break;
        case 'o':
            oPerm = (int*)((size_t)oPerm ^ 1);
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
    if(Abc_NtkLatchNum(pNtk)) {
        cout << "Error: This command can only be applied to a purely combinational network. Please run command \"to_comb\" first.\n";
        return 1;
    }
    if(!(iPerm || oPerm)) return 0;

    
    pNtk = Abc_NtkStrash(pNtk, 0, 1, 0);

    // permute PI
    if(iPerm) {
        cuint nPi = Abc_NtkPiNum(pNtk);
        iPerm = initPerm(nPi);
        pNtk = aigUtils::aigPermCi(pNtk, iPerm, true);
        if(verbose) {
            cout << "iPerm: ";
            printPerm(iPerm, nPi);
        }
    }

    // permute PO
    if(oPerm) {
        cuint nPo = Abc_NtkPoNum(pNtk);
        oPerm = initPerm(nPo);
        aigUtils::aigPermCo(pNtk, oPerm);
        if(verbose) {
            cout << "oPerm: ";
            printPerm(oPerm, nPo);
        }
    }

    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
    if(iPerm) delete [] iPerm;
    if(oPerm) delete [] oPerm;
    return 0;

usage:
    Abc_Print(-2, "usage: rand_perm_io [-iovh]\n");
    Abc_Print(-2, "\t        permutates the order of PI/PO randomly\n");
    Abc_Print(-2, "\t-i    : toggles permutation of PI [default = %s]\n", iPerm ? "on" : "off");
    Abc_Print(-2, "\t-o    : toggles permutation of PO [default = %s]\n", oPerm ? "on" : "off");
    Abc_Print(-2, "\t-v    : toggles verbosity [default = %s]\n", verbose ? "on" : "off");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "rand_perm_io", RPermIO_Command, 1);
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
} rPermIO_registrar;

} // end namespace rPermIO