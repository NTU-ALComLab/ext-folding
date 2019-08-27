#include "ext-folding/timeFold/timeFold.h"

using namespace timeFold;
using namespace bddUtils;

namespace timeFold
{

// build transitions of the states from consecutive time-frames
void buildTrans(DdManager *dd, DdNode **pNodeVec, DdNode **B, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, st__table *csts, st__table *nsts, vector<string>& stg)
{
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;

    st__generator *cGen, *nGen;
    DdNode *cKNode, *cVNode, *nKNode, *nVNode;
    size_t cCnt = 0, nCnt = 0;
    
    DdNode **oFuncs  = new DdNode*[nPo];
    DdNode *G, *path, *bCube, *tmp1, *tmp2;

    st__foreach_item(csts, cGen, (const char**)&cKNode, (char**)&cVNode) {
        // find a path to this state, store it in cube
        gen = Cudd_FirstCube(dd, cVNode, &cube, &val);
        
        // get output function of current state
        // oFunc = cofactor(f_k, cube)
        bCube = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(bCube);  // bdd of the cube
        for(size_t k=0; k<nPo; ++k) {
            oFuncs[k] = Cudd_Cofactor(dd, pNodeVec[i*nPo+k], bCube);
            Cudd_Ref(oFuncs[k]);
        }
        
        Cudd_GenFree(gen);
        
        nCnt = 0;
        st__foreach_item(nsts, nGen, (const char**)&nKNode, (char**)&nVNode) {
            path = Cudd_bddAnd(dd, cVNode, nVNode);  Cudd_Ref(path);

            if(path != b0) {  // transition exist
                tmp1 = Cudd_Cofactor(dd, nVNode, bCube);  Cudd_Ref(tmp1);
                Cudd_RecursiveDeref(dd, path);
                path = tmp1;

                // encode output on/off-sets
                G = b0;  Cudd_Ref(G);
                for(size_t k=0; k<2; ++k) {
                    bddNotVec(oFuncs, nPo);   // 2*negation overall
                    tmp1 = bddDot(dd, oFuncs, B+k*nPo, nPo);
                    tmp2 = Cudd_bddOr(dd, G, tmp1);  Cudd_Ref(tmp2);
                    
                    Cudd_RecursiveDeref(dd, G);
                    Cudd_RecursiveDeref(dd, tmp1);
                    G = tmp2;
                }

                // ANDing transition constraint
                tmp1 = Cudd_bddAnd(dd, G, path);  Cudd_Ref(tmp1);
                Cudd_RecursiveDeref(dd, G);
                G = tmp1;

                // add transition (cst->nst) to STG
                fileWrite::addOneTrans(dd, G, oFuncs, nPi, nPo, nTimeFrame, i, cCnt, nCnt, stg);  // G is deref by addOneTrans()

            }

            Cudd_RecursiveDeref(dd, path);
            ++nCnt;
        }

        Cudd_RecursiveDeref(dd, bCube);
        bddDerefVec(dd, oFuncs, nPo);
        ++cCnt;
    }

    delete [] oFuncs;
}

int bddFold(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, const bool verbosity)
{
    // initialize bdd manger (disable var. reordering)
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, ABC_INFINITY, 1, 0, 0, 0);
    if(!dd) {
        cerr << "#nodes exceeds the maximum limit." << endl;
        return 0;
    }
    
    // get basic settings
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint nPo = nCo / nTimeFrame;
    cuint initVarSize = Cudd_ReadSize(dd);
    cuint nVar = initVarSize / nTimeFrame;
    
    // get PO at each timeframe
    int i; Abc_Obj_t *pObj;
    DdNode **pNodeVec = new DdNode*[nCo];
    Abc_NtkForEachCo(pNtk, pObj, i)
        pNodeVec[i] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
    
    // compute signature
    size_t nB = nPo ? (size_t)ceil(log2(double(nPo*2))) : 0;
    DdNode **B = computeSign(dd, nPo*2);
    assert(initVarSize+nB == Cudd_ReadSize(dd));
    
    // collecting states at each timeframe
    size_t stsSum = 1;
    st__table *csts, *nsts = createDummyState(dd);
    st__generator *nGen;
    DdNode *nKNode, *nVNode;
    DdNode *tmp1, *tmp2;
    
    for(i=nTimeFrame-1; i>=0; --i) {
        vector<clock_t> tVec;
        
        // cutting bdd, starting from last timeframe
        // F = A_t * f_t + A_t-1 * f_t-1 + ...
        // f_k = B_1 * f_k_1 + B_2 * f_k_2 + ... + f_k_nPo
        if(i) {
            tVec.push_back(clock());  // t0: init
            
            // states from (i+1)^th time-frame (pFuncs)
            size_t pCnt = 0, nFuncs = st__count(nsts);
            DdNode **pFuncs = new DdNode*[nFuncs];
            st__foreach_item(nsts, nGen, (const char**)&nKNode, (char**)&nVNode)
                pFuncs[pCnt++] = nVNode;
            assert(pCnt == nFuncs);
            if(pCnt == 1) assert(pFuncs[0] == b1);

            // variables to encode pFuncs
            size_t na = (size_t)ceil(log2(double(nFuncs))) + 2;
            DdNode **a = new DdNode*[na];
            for(size_t j=0; j<na; ++j) a[j] = Cudd_bddIthVar(dd, initVarSize+nB+j);

            // hyper-function encoding of output functions of i^th time-frame
            DdNode *ft = bddDot(dd, pNodeVec+i*nPo, B, nPo);
            tmp1 = Cudd_bddAnd(dd, Cudd_Not(a[0]), ft);  Cudd_Ref(tmp1);
            Cudd_RecursiveDeref(dd, ft);
            ft = tmp1;

            // hyper-function encoding of pFuncs
            tmp1 = Extra_bddEncodingBinary(dd, pFuncs, nFuncs, a+1, na-1);  Cudd_Ref(tmp1);
            tmp2 = Cudd_bddAnd(dd, a[0], tmp1);  Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(dd, tmp1);

            // ORing 2 hyper-functions
            tmp1 = Cudd_bddOr(dd, ft, tmp2);  Cudd_Ref(tmp1);
            
            Cudd_RecursiveDeref(dd, ft);
            Cudd_RecursiveDeref(dd, tmp2);

            tVec.push_back(clock());  // t1: after building hyper-function
            
            // cut set enumeration
            csts = Extra_bddNodePathsUnderCut(dd, tmp1, nVar*i);

            tVec.push_back(clock());  // t2: after BDD cut
            
            Cudd_RecursiveDeref(dd, tmp1);

            // free arrays
            delete [] a;
            delete [] pFuncs;
        }
        else csts = createDummyState(dd);  // i==0

        if(tVec.empty()) for(size_t j=0; j<3; ++j) tVec.push_back(clock());
        
        stsSum += st__count(csts);
        
        if(verbosity) cout << setw(7) << st__count(csts) << " states: ";

        buildTrans(dd, pNodeVec, B, nVar, nPo, nTimeFrame, i, csts, nsts, stg);

        tVec.push_back(clock()); // t3
        
        // replace nsts with csts, and enter next iteration
        bddFreeTable(dd, nsts);
        nsts = csts;
        
        if(verbosity) {
            for(size_t j=1; j<tVec.size(); ++j)
                cout << setw(7) << double(tVec[j]-tVec[j-1])/CLOCKS_PER_SEC << " ";
            cout << Cudd_ReadNodeCount(dd) << " nodes" << endl;
        }
    }
    

    // free arrays
    bddFreeVec(dd, B, 2*nPo);
    bddFreeTable(dd, nsts);
    bddFreeVec(dd, pNodeVec, nCo);
    
    if(verbosity) {
        cout << "remaining BDD nodes: " << Cudd_ReadNodeCount(dd) << endl;
        cout << "--------------------------------------------------" << endl;
    }

    // free bdd manger
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);

    return stsSum;
}


} // end namespace timeFold
