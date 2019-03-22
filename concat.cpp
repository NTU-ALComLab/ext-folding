#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iomanip>

using namespace std;

namespace
{
Abc_Ntk_t* concat(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2)
{
    int i;
    char Buffer[1000];
    Abc_Ntk_t *pNtkNew;
    Abc_Obj_t *pObj, *pObjNew;
    
    assert(Abc_NtkIsStrash(pNtk1));
    assert(Abc_NtkIsStrash(pNtk2));
    
    // start the new network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_%s_concat", pNtk1->pName, pNtk2->pName );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);
    
    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkNew);
    
    // create new PIs and remember them in the old PIs
    Abc_NtkForEachCi(pNtk1, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
        pObj = Abc_NtkCi(pNtk2, i);  
        pObj->pCopy = pObjNew;
        // add name
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL );
    }
    
    // create POs for Ntk1
    Abc_NtkForEachCo(pNtk1, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, "concat1_", Abc_ObjName(pObj));
    }
    
    // create POs for Ntk2
    Abc_NtkForEachCo(pNtk2, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, "concat2_", Abc_ObjName(pObj));
    }
    
    assert(Abc_NtkIsDfsOrdered(pNtk1));
    assert(Abc_NtkIsDfsOrdered(pNtk2));
    
    // AddOne for Ntk1
    Abc_AigForEachAnd(pNtk1, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, 
                                 Abc_ObjChild0Copy(pObj), 
                                 Abc_ObjChild1Copy(pObj));
    // AddOne for Ntk2                          
    Abc_AigForEachAnd(pNtk2, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, 
                                 Abc_ObjChild0Copy(pObj), 
                                 Abc_ObjChild1Copy(pObj));
    
    // collect POs for Ntk1
    Abc_NtkForEachCo(pNtk1, pObj, i) {
        pObjNew = Abc_ObjChild0Copy(pObj);
        Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, i), pObjNew);
    }
    
    // collect POs for Ntk1
    Abc_NtkForEachCo(pNtk2, pObj, i) {
        pObjNew = Abc_ObjChild0Copy(pObj);
        Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, Abc_NtkCoNum(pNtk1)+i), pObjNew);
    }
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    
    if (!Abc_NtkCheck(pNtkNew))
    {
        printf( "concat: The network check has failed.\n" );
        Abc_NtkDelete(pNtkNew);
        return NULL;
    }
    
    return pNtkNew;
}


int Concat_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    // usage: concat <ckt1> <ckt2>
    // the 2 circuits will be merged together
    // they should have the same number of PIs
    // PIs in both circuits will be built
    
    if(argc != 3) return 1;
    
    Abc_Ntk_t *pNtk[2], *pNtkNew = NULL;
    int i, fCheck = 1, fBarBufs = 0;
    int fRemove[2];
    
    for(i=0; i<2; ++i) {
        pNtk[i] = Io_Read( argv[i+1], Io_ReadFileType(argv[i+1]), fCheck, fBarBufs );
        assert(Abc_NtkHasOnlyLatchBoxes(pNtk[i]));
        fRemove[i] = (!Abc_NtkIsStrash(pNtk[i]) || Abc_NtkGetChoiceNum(pNtk[i])) && (pNtk[i] = Abc_NtkStrash(pNtk[i], 0, 0, 0));
    }
    
    assert(Abc_NtkCiNum(pNtk[0]) == Abc_NtkCiNum(pNtk[1]));
    
    if(pNtk[0] && pNtk[1])
        pNtkNew = concat(pNtk[0], pNtk[1]);
    
    
    // delete
    for(i=0; i<2; ++i) if(fRemove[i])
        Abc_NtkDelete(pNtk[i]);
    
    // replace the current network
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
    //Abc_FrameCopyLTLDataBase(pAbc, pNtkNew);
    //Abc_FrameClearVerifStatus(pAbc);
    
    return 0;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "tFold", "concat", Concat_Command, 0);
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
} fold_registrar;

} // unnamed namespace