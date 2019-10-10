#include "ext-folding/utils/utils.h"

using namespace std;
using namespace utils;

namespace timeCas
{

static Abc_Ntk_t* timeCas(Abc_Ntk_t *pNtk, cuint nTimeFrame)
{
    uint i;
    Abc_Obj_t *pObj, *pObjNew;

    // get basic settings
    cuint nCo = Abc_NtkCoNum(pNtk);         // total circuit output after time expansion
    cuint nPo = nCo / nTimeFrame;           // #POs of the original seq. ckt
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nPi = nCi / nTimeFrame;
    assert(nPo * nTimeFrame == nCo);
    assert(nPi * nTimeFrame == nCi);
    
    // start the new network
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);

    // set name
    char charBuf[1000];
    sprintf(charBuf, pNtk->pName, "_tfcas");
    pNtkNew->pName = Extra_UtilStrsav(charBuf);
    
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    
    // create PIs
    cuint nXVec = (1+nPi) * nTimeFrame;
    //vector<Abc_Obj_t*> xVec;  xVec.reserve(nXVec);
    Abc_NtkForEachCi(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // add name
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        //xVec.push_back(pObjNew);
        
        if((i+1)%nPi == 0) {
            pObjNew = Abc_NtkCreatePi(pNtkNew);
            Abc_ObjAssignName(pObjNew, "rst_", (char*)to_string(i/nPi).c_str());
            //xVec.push_back(pObjNew);
        }
    }
    assert(Abc_NtkCiNum(pNtkNew) == nXVec);
    //assert(xVec.size() == nXVec);
    
    // create POs
    Abc_NtkForEachCo(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);        
    }
    
    cuint nYVec = (1+nTimeFrame) * nTimeFrame * nPo / 2;
    vector<Abc_Obj_t*> yVec;  yVec.reserve(nYVec);
    
    for(uint t=0; t<nTimeFrame; ++t) {
        for(uint j=0, k=t*(nPi+1); j<(nTimeFrame-t)*nPi; ++j) {
            assert(k < nXVec);
            // remember this PI in the old PIs
            pObj = Abc_NtkCi(pNtk, j);
            pObjNew = Abc_NtkCi(pNtkNew, k++);
            pObj->pCopy = pObjNew;
            
            if((j+1)%nPi == 0) ++k;
        }
        
        // copy ntk, *some pCopy may be wrong!!*
        Abc_AigForEachAnd(pNtk, pObj, i)
            pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
            
        // collect output
        for(uint j=0; j<(nTimeFrame-t)*nPo; ++j)
            yVec.push_back(Abc_ObjChild0Copy(Abc_NtkCo(pNtk, j)));
        
    }
    assert(yVec.size() == nYVec);
    
    for(uint j=0; j<nPo; ++j)
        Abc_ObjAddFanin(Abc_NtkCo(pNtkNew, j), yVec[j]);
    
    // add MUX control
    vector<Abc_Obj_t*> rstVec, _yVec, _oVec;
    rstVec.reserve(nTimeFrame-1);  _yVec.reserve(nPo*nTimeFrame);  _oVec.reserve(nPo);
    for(uint t=1; t<nTimeFrame; ++t) {
        rstVec.push_back(Abc_NtkCi(pNtkNew, (nPi+1)*t-1));
        for(uint j=0; j<t+1; ++j) {
            uint tt = t - j;
            uint offset = (2*nTimeFrame-j+1) * j / 2;
            for(uint k=0; k<nPo; ++k) _yVec.push_back(yVec[(offset+tt)*nPo+k]);
        }
        assert(_yVec.size() == nPo*(t+1));
        
        for(uint j=0; j<nPo; ++j) _oVec.push_back(_yVec[j]);
        for(uint j=0; j<rstVec.size(); ++j) for(uint k=0; k<nPo; ++k)
                _oVec[k] = Abc_AigMux((Abc_Aig_t*)pNtkNew->pManFunc, rstVec[j], _yVec[(j+1)*nPo+k], _oVec[k]);
        
        assert(_oVec.size() == nPo);
        for(uint j=0; j<nPo; ++j)
            Abc_ObjAddFanin(Abc_NtkCo(pNtkNew, t*nPo+j), _oVec[j]);
        
        _oVec.clear();
        _yVec.clear();
    }
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));
    
    return pNtkNew;
}

int TimeCas_Command(Abc_Frame_t * pAbc, int argc, char ** argv)
{
    Abc_Ntk_t *pNtk, *pNtkNew;
    int nTimeFrame;
    bool fRemove = false;
    int c;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "h")) != EOF) {
        switch(c) {
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

    if(globalUtilOptind >= argc) goto usage;
    nTimeFrame = atoi(argv[globalUtilOptind++]);
    if(nTimeFrame < 0) goto usage;
    assert(globalUtilOptind == argc);
    
    if(!Abc_NtkIsStrash(pNtk)) {
        pNtk = Abc_NtkStrash(pNtk, 0, 1, 0);
        fRemove = true;
    }

    pNtkNew = timeCas(pNtk, nTimeFrame);
    
    if(fRemove) Abc_NtkDelete(pNtk);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);

    return 0;

usage:
    Abc_Print(-2, "usage: time_cas [-h] <num>\n");
    Abc_Print(-2, "\t             adds an reset signal to the time-frame expanded circuit, and cascades all the outputs from each time-frame\n");
    Abc_Print(-2, "\t-h         : print the command usage\n");
    Abc_Print(-2, "\tnum        : number of time-frames the circuit has been expanded\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "Time-frame Folding", "time_cas", TimeCas_Command, 0);
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
} timeCas_registrar;

} // end namespace timeCas
