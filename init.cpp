#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "ext-folding/utils.h"

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

int tFold_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    /*
    bool showUsg = false;
    if(argc != 2) showUsg = true;
    else if(argv[1][0] == '-') showUsg = true;
    if(showUsg) {
        Abc_Print(-2, "usage: timefold [-h] <file>\n");
        Abc_Print(-2, "             fold the circuit after time expansion and transform it into a STG\n");
        Abc_Print(-2, "\t-h       : print the command usage\n");
        Abc_Print(-2, "\tfile     : the kiss file name\n");
        return 1;
    }
    */
    
    // usage: timefold <nTimeframe> <kissFile>
    // get args
    assert(argc == 3);
    const size_t nTimeframe = atoi(argv[1]);
    const char *kissFn = argv[2];
    
    // get pNtk
    Abc_Ntk_t *pNtk = Abc_FrameReadNtk(pAbc);
    if(pNtk == NULL) {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if(!Abc_NtkIsStrash(pNtk)) pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    
    
    // initialize bdd manger (disable var. reordering)
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, 10000000, 1, 0, 0, 0);
    
    
    // get basic settings
    const size_t nCo = Abc_NtkCoNum(pNtk);         // total circuit output after time expansion
    const size_t nPo = nCo / nTimeframe;           // #POs of the original seq. ckt
    const size_t initVarSize = Cudd_ReadSize(dd);
    const size_t nVar = initVarSize / nTimeframe;  // #variable for each t (#PI of the original seq. ckt)
    assert(nPo * nTimeframe == nCo);
    assert(initVarSize % nTimeframe == 0);
    
    
    // build bdd of PO at each timeframe: f_1_1, f_1_2, ... f_2_1, ... f_t_nPo
    int i; Abc_Obj_t *pObj;
    DdNode **pNodeVec = new DdNode*[nCo];
    Abc_NtkForEachCo(pNtk, pObj, i)
        pNodeVec[i] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
    
    
    // compute signature
    //DdNode **A = computeSign(dd, nTimeframe-1);  // foreach timeframe
    size_t nB = nPo ? (size_t)ceil(log2(double(nPo*2))) : 0;
    DdNode **B = computeSign(dd, nPo*2);           // foreach PO
    assert(initVarSize+nB == Cudd_ReadSize(dd));
    
    // collecting states at each timeframe
    vector<string> stg;
    size_t stsSum = 1;
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;  // dummies

    string spc = string(" "), uds = string("_"), sHead = string("S");
    string cstName, nstName;
    
    st__table *csts, *nsts = createDummyState(dd);
    st__generator *cGen, *nGen;
    DdNode *cKNode, *cVNode, *nKNode, *nVNode;
    size_t cCnt = 0, nCnt;
    
    
    DdNode **oFuncs  = new DdNode*[nPo];
    //DdNode *F = b0;  Cudd_Ref(F);
    DdNode *G, *tmp1, *tmp2;
    
    for(i=nTimeframe-1; i>=0; --i) {
        vector<clock_t> tVec;
        
        // cutting bdd, starting from last timeframe
        // F = A_t * f_t + A_t-1 * f_t-1 + ...
        // f_k = B_1 * f_k_1 + B_2 * f_k_2 + ... + f_k_nPo
        if(i) {
            tVec.push_back(clock());  // t0
            
            size_t pCnt = 0, nFuncs = st__count(nsts);
            DdNode **pFuncs = new DdNode*[nFuncs];
            st__foreach_item(nsts, nGen, (const char**)&nKNode, (char**)&nVNode)
                pFuncs[pCnt++] = nVNode;
            assert(pCnt == nFuncs);
            if(pCnt == 1) assert(pFuncs[0] == b1);
            
            size_t na = (size_t)ceil(log2(double(nFuncs))) + 2;
            
            DdNode **a = new DdNode*[na];
            for(size_t j=0; j<na; ++j) a[j] = Cudd_bddIthVar(dd, initVarSize+nB+j);
            
            
            DdNode *ft = bddDot(dd, pNodeVec+i*nPo, B, nPo);
            tmp1 = Cudd_bddAnd(dd, Cudd_Not(a[0]), ft);  Cudd_Ref(tmp1);
            Cudd_RecursiveDeref(dd, ft);
            ft = tmp1;

            
            tmp1 = Extra_bddEncodingBinary(dd, pFuncs, nFuncs, a+1, na-1);  Cudd_Ref(tmp1);
            tmp2 = Cudd_bddAnd(dd, a[0], tmp1);  Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(dd, tmp1);
            
            
            tmp1 = Cudd_bddOr(dd, ft, tmp2);  Cudd_Ref(tmp1);
            
            Cudd_RecursiveDeref(dd, ft);
            Cudd_RecursiveDeref(dd, tmp2);

            
            tVec.push_back(clock());  // t1
            
            csts = Extra_bddNodePathsUnderCut(dd, tmp1, nVar*i);
            
            tVec.push_back(clock());  // t2
            
            Cudd_RecursiveDeref(dd, tmp1);
            delete [] a;
            delete [] pFuncs;
        }
        else csts = createDummyState(dd);  // i==0
        
        if(tVec.empty()) for(size_t j=0; j<3; ++j) tVec.push_back(clock());
        
        stsSum += st__count(csts);
        
        cout << setw(7) << st__count(csts) << " states: ";
        
        cCnt = 0;
        st__foreach_item(csts, cGen, (const char**)&cKNode, (char**)&cVNode) {
            cstName = sHead + to_string(i) + uds + to_string(cCnt);
            
            // find a path to this state, store it in cube
            gen = Cudd_FirstCube(dd, cVNode, &cube, &val);
            
            // get output function of current state
            // oFunc = cofactor(f_k, cube)
            DdNode *_cube = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(_cube);  // bdd of the cube
            for(size_t k=0; k<nPo; ++k) {
                oFuncs[k] = Cudd_Cofactor(dd, pNodeVec[i*nPo+k], _cube);
                Cudd_Ref(oFuncs[k]);
            }
            
            Cudd_GenFree(gen);
            
            nCnt = 0;
            st__foreach_item(nsts, nGen, (const char**)&nKNode, (char**)&nVNode) {
                
                if(i == nTimeframe-1) nstName = string("*");
                else nstName = sHead + to_string(i+1) + uds + to_string(nCnt);
                
                DdNode *path = Cudd_bddAnd(dd, cVNode, nVNode);  Cudd_Ref(path);
                
                if(path != b0) {  // transition exist
                    DdNode *_path = Cudd_Cofactor(dd, nVNode, _cube);  Cudd_Ref(_path);
                    
                    G = b0;  Cudd_Ref(G);
                    for(size_t k=0; k<2; ++k) {
                        bddNotVec(oFuncs, nPo);   // 2*negation overall
                        tmp1 = bddDot(dd, oFuncs, B+k*nPo, nPo);
                        tmp2 = Cudd_bddOr(dd, G, tmp1);  Cudd_Ref(tmp2);
                        
                        Cudd_RecursiveDeref(dd, G);
                        Cudd_RecursiveDeref(dd, tmp1);
                        G = tmp2;
                    }
                    
                    tmp1 = Cudd_bddAnd(dd, G, _path);  Cudd_Ref(tmp1);
                    Cudd_RecursiveDeref(dd, G);
                    G = tmp1;
                    
                    while(G != b0) {
                        gen = Cudd_FirstCube(dd, G, &cube, &val);
                        
                        string trans("");
                            
                        // input bits
                        size_t vStart = i * nVar;
                        size_t vEnd = (i+1) * nVar;
                        for(size_t m=0; m<Cudd_ReadSize(dd); ++m) {
                            if((m>=vStart) && (m<vEnd))
                                trans += (cube[m]==2) ? string("-") : to_string(cube[m]);
                            else
                                cube[m] = 2;
                        }
                        
                        // states
                        trans += spc + cstName + spc + nstName + spc;
                        
                        // output bits
                        tmp1 = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(tmp1);
                        for(size_t k=0; k<nPo; ++k) {
                            tmp2 = Cudd_Cofactor(dd, oFuncs[k], tmp1);  Cudd_Ref(tmp2);
                            if(tmp2 == b0) trans += "0";
                            else trans += "1";
                            Cudd_RecursiveDeref(dd, tmp2);
                        }
                        
                        stg.push_back(trans);
                        
                        tmp2 = Cudd_bddAnd(dd, G, Cudd_Not(tmp1));  Cudd_Ref(tmp2);
                        
                        Cudd_RecursiveDeref(dd, tmp1);
                        Cudd_RecursiveDeref(dd, G);
                        
                        G = tmp2;
                        
                        Cudd_GenFree(gen);
                    }
                    
                    Cudd_RecursiveDeref(dd, G);
                    Cudd_RecursiveDeref(dd, _path);
                }
                
                Cudd_RecursiveDeref(dd, path);
                
                ++nCnt;
            }
            Cudd_RecursiveDeref(dd, _cube);
            bddDerefVec(dd, oFuncs, nPo);
            
            ++cCnt;
        }
        
        tVec.push_back(clock()); // t3
        
        bddFreeTable(dd, nsts);
        nsts = csts;
        
        for(size_t j=1; j<tVec.size(); ++j)
            cout << setw(7) << double(tVec[j]-tVec[j-1])/CLOCKS_PER_SEC << " ";
        cout << Cudd_ReadNodeCount(dd) << " nodes" << endl;
    }
    
    //Cudd_RecursiveDeref(dd, F);
    
    
    // write kiss
    ofstream fp;
    fp.open(kissFn);
    fp << ".i " << nVar << "\n";
    fp << ".o " << nPo << "\n";
    fp << ".p " << stg.size() << "\n";
    fp << ".s " << stsSum << "\n";
    fp << ".r " << "S0_0" << "\n";
    for(string trans: stg) fp << trans << "\n";
    fp << ".e\n";
    fp.close();
    
    
    // free array
    delete [] oFuncs;
    //bddFreeVec(dd, A, nTimeframe-1);
    bddFreeVec(dd, B, 2*nPo);
    bddFreeTable(dd, nsts);
    bddFreeVec(dd, pNodeVec, nCo);
    
    cout << "final: " << Cudd_ReadNodeCount(dd) << endl;
    
    // free bdd manger
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);
    
    return 0;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "tFold", "timefold", tFold_Command, 0);
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