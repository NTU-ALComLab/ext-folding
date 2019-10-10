#include "ext-folding/timeFold/timeFold.h"

using namespace timeFold;
using namespace bddUtils;

namespace timeFold
{

// build transitions of the states from consecutive time-frames
void buildTrans(DdManager *dd, DdNode **pNodeVec, DdNode **B, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, st__table *csts, st__table *nsts, STG *stg)
{
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;

    st__generator *cGen, *nGen;
    DdNode *cKNode, *cVNode, *nKNode, *nVNode;
    uint cCnt = 0, nCnt = 0;
    
    DdNode **oFuncs  = new DdNode*[nPo];
    DdNode *F, *G, *path, *bCube, *tmp1, *tmp2;

    st__foreach_item(csts, cGen, (const char**)&cKNode, (char**)&cVNode) {
        // find a path to this state, store it in cube
        gen = Cudd_FirstCube(dd, cVNode, &cube, &val);
        
        // get output function of current state
        // oFunc = cofactor(f_k, cube)
        bCube = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(bCube);  // bdd of the cube
        for(uint k=0; k<nPo; ++k) {
            if(pNodeVec[i*nPo+k]) {
                oFuncs[k] = Cudd_Cofactor(dd, pNodeVec[i*nPo+k], bCube);
                Cudd_Ref(oFuncs[k]);
            } else oFuncs[k] = NULL;
        }
        
        Cudd_GenFree(gen);

        // encode output on/off-sets
        F = b0;  Cudd_Ref(F);
        for(uint k=0; k<2; ++k) {
            bddNotVec(oFuncs, nPo);   // 2*negation overall
            tmp1 = bddDot(dd, oFuncs, B+k*nPo, nPo);
            tmp2 = Cudd_bddOr(dd, F, tmp1);  Cudd_Ref(tmp2);
            
            Cudd_RecursiveDeref(dd, F);
            Cudd_RecursiveDeref(dd, tmp1);
            F = tmp2;
        }

        
        nCnt = 0;
        st__foreach_item(nsts, nGen, (const char**)&nKNode, (char**)&nVNode) {
            path = Cudd_bddAnd(dd, cVNode, nVNode);  Cudd_Ref(path);

            if(path != b0) {  // transition exist
                tmp1 = Cudd_Cofactor(dd, nVNode, bCube);  Cudd_Ref(tmp1);
                Cudd_RecursiveDeref(dd, path);
                path = tmp1;

                // ANDing transition constraint
                G = Cudd_bddAnd(dd, F, path);  Cudd_Ref(G);

                // add transition (cst->nst) to STG
                stg->addOneTrans(dd, G, oFuncs, i, cCnt, nCnt);  // G is deref by addOneTrans()
                //fileWrite::addOneTrans(dd, G, oFuncs, nPi, nPo, nTimeFrame, i, cCnt, nCnt, stg);  // deprecated

            }

            Cudd_RecursiveDeref(dd, path);
            ++nCnt;
        }

        Cudd_RecursiveDeref(dd, F);
        Cudd_RecursiveDeref(dd, bCube);
        bddDerefVec(dd, oFuncs, nPo);
        ++cCnt;
    }

    delete [] oFuncs;
}

STG* bddFold(Abc_Ntk_t *pNtk, cuint nTimeFrame, const bool verbose, const char *logFileName)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;

    // initialize bdd manger (disable var. reordering)
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, ABC_INFINITY, 1, 0, 0, 0);
    if(!dd) {
        cerr << "#nodes exceeds the maximum limit." << endl;
        return NULL;
    }
    if(logger) logger->log("init bdd man");
    
    // get basic settings
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint nPo = nCo / nTimeFrame;
    cuint initVarSize = Cudd_ReadSize(dd);
    cuint nVar = initVarSize / nTimeFrame;

    STG *stg = new STG(initVarSize, nCo, nVar, nPo, nTimeFrame);
    
    // get PO at each timeframe
    int i; Abc_Obj_t *pObj;
    DdNode **pNodeVec = new DdNode*[nCo];
    Abc_NtkForEachCo(pNtk, pObj, i)
        pNodeVec[i] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
    
    // compute signature
    uint nB = nPo ? (uint)ceil(log2(double(nPo*2))) : 0;
    DdNode **B = bddComputeSign(dd, nPo*2);
    assert(initVarSize+nB == Cudd_ReadSize(dd));
    if(logger) logger->log("prepare sign");
    
    // collecting states at each timeframe
    uint stsSum = 1;
    st__table *csts, *nsts = bddCreateDummyState(dd);
    st__generator *nGen;
    DdNode *nKNode, *nVNode;
    DdNode *tmp1, *tmp2;
    
    for(i=nTimeFrame-1; i>=0; --i) {
        // cutting bdd, starting from last timeframe
        // F = A_t * f_t + A_t-1 * f_t-1 + ...
        // f_k = B_1 * f_k_1 + B_2 * f_k_2 + ... + f_k_nPo
        if(i) {
            // states from (i+1)^th time-frame (pFuncs)
            uint pCnt = 0, nFuncs = st__count(nsts);
            DdNode **pFuncs = new DdNode*[nFuncs];
            st__foreach_item(nsts, nGen, (const char**)&nKNode, (char**)&nVNode)
                pFuncs[pCnt++] = nVNode;
            assert(pCnt == nFuncs);
            if(pCnt == 1) assert(pFuncs[0] == b1);

            // variables to encode pFuncs
            uint na = (uint)ceil(log2(double(nFuncs))) + 2;
            DdNode **a = new DdNode*[na];
            for(uint j=0; j<na; ++j) a[j] = Cudd_bddIthVar(dd, initVarSize+nB+j);

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
            if(logger) logger->log("hyper-" + to_string(i));
            
            // cut set enumeration
            csts = Extra_bddNodePathsUnderCut(dd, tmp1, nVar*i);

            if(logger) logger->log("cut-" + to_string(i));
            
            Cudd_RecursiveDeref(dd, tmp1);

            // free arrays
            delete [] a;
            delete [] pFuncs;
        }
        else csts = bddCreateDummyState(dd);  // i==0

        stsSum += st__count(csts);
        
        if(verbose) cout << setw(7) << st__count(csts) << " states: ";

        buildTrans(dd, pNodeVec, B, nVar, nPo, nTimeFrame, i, csts, nsts, stg);
        if(logger) logger->log("trans-" + to_string(i));
        
        // replace nsts with csts, and enter next iteration
        bddFreeTable(dd, nsts);
        nsts = csts;
        if(verbose) cout << Cudd_ReadNodeCount(dd) << " nodes" << endl;
        if(logger) logger->log("next-" + to_string(i));
    }
    
    stg->setNSts(stsSum);  // update #states

    // free arrays
    bddFreeVec(dd, B, 2*nPo);
    bddFreeTable(dd, nsts);
    bddFreeVec(dd, pNodeVec, nCo);
    
    if(verbose) {
        cout << "remaining BDD nodes: " << Cudd_ReadNodeCount(dd) << endl;
        cout << "--------------------------------------------------" << endl;
    }

    // free bdd manger
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);

    if(logger) {
        logger->log("free memory");
        delete logger;
    }

    return stg;
}


} // end namespace timeFold
