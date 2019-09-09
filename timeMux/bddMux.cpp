#include "ext-folding/timeMux/timeMux.h"

using namespace timeMux;
using namespace bddUtils;

namespace timeMux
{

// build transitions of the states from consecutive time-frames
void buildTrans(DdManager *dd, DdNode **pNodeVec, DdNode **B, cuint nPi, cuint nCo, cuint nTimeFrame, cuint i, st__table *csts, st__table *nsts, vector<string>& stg)
{
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;

    st__generator *cGen, *nGen;
    DdNode *cKNode, *cVNode, *nKNode, *nVNode;
    size_t cCnt = 0, nCnt = 0;
    
    DdNode **oFuncs  = (i==nTimeFrame-1) ? new DdNode*[nCo] : NULL;
    DdNode *F, *G, *path, *bCube, *tmp1, *tmp2;

    st__foreach_item(csts, cGen, (const char**)&cKNode, (char**)&cVNode) {
        // find a path to this state, store it in cube
        gen = Cudd_FirstCube(dd, cVNode, &cube, &val);
        
        // get output function of current state
        // oFunc = cofactor(f_k, cube)
        bCube = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(bCube);  // bdd of the cube
        if(oFuncs) for(size_t k=0; k<nCo; ++k) {
            oFuncs[k] = Cudd_Cofactor(dd, pNodeVec[k], bCube);
            Cudd_Ref(oFuncs[k]);
        }
        
        Cudd_GenFree(gen);

        // encode output on/off-sets
        if(oFuncs) {
            F = b0;  Cudd_Ref(F);
            for(size_t k=0; k<2; ++k) {
                bddNotVec(oFuncs, nCo);   // 2*negation overall
                tmp1 = bddDot(dd, oFuncs, B+k*nCo, nCo);
                tmp2 = Cudd_bddOr(dd, F, tmp1);  Cudd_Ref(tmp2);
                
                Cudd_RecursiveDeref(dd, F);
                Cudd_RecursiveDeref(dd, tmp1);
                F = tmp2;
            }
        } else {
            F = b1; Cudd_Ref(F);
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
                fileWrite::addOneTrans(dd, G, oFuncs, nPi, nCo, nTimeFrame, i, cCnt, nCnt, stg);  // G is deref by addOneTrans()

            }

            Cudd_RecursiveDeref(dd, path);
            ++nCnt;
        }

        Cudd_RecursiveDeref(dd, F);
        Cudd_RecursiveDeref(dd, bCube);
        if(oFuncs) bddDerefVec(dd, oFuncs, nCo);
        ++cCnt;
    }

    if(oFuncs) delete [] oFuncs;
}

int bddMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, size_t *perm, const bool verbosity, const char *logFileName, const Cudd_ReorderingType rt)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;

    // initialize bdd manger
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, ABC_INFINITY, 1, (int)(perm!=NULL), 0, 0);
    //DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, ABC_INFINITY, 1, 0, 0, 0);
    if(!dd) {
        cerr << "#nodes exceeds the maximum limit." << endl;
        return 0;
    }
    
    if(logger) logger->log("init bdd man");

    // get basic settings
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint initVarSize = Cudd_ReadSize(dd);
    cuint nVar = initVarSize / nTimeFrame;
    
    int i; Abc_Obj_t *pObj;
    DdNode **pNodeVec = new DdNode*[nCo];
    Abc_NtkForEachCo(pNtk, pObj, i)
        pNodeVec[i] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1

    
    // compute signature
    size_t nB = nCo ? (size_t)ceil(log2(double(nCo*2))) : 0;
    DdNode **B = computeSign(dd, nCo*2);
    assert(initVarSize+nB == Cudd_ReadSize(dd));
    if(logger) logger->log("prepare sign");

    // compute hyper-function
    DdNode *H = bddDot(dd, pNodeVec, B, nCo);
    if(logger) logger->log("build hyper");

    // reorder bdd
    if(perm) {
/*
        Cudd_AutodynEnable(dd, rt);
        // fixed B var. order
        for(i=initVarSize; i<Cudd_ReadSize(dd); ++i)
            assert(Cudd_bddBindVar(dd, i));
        Cudd_ReduceHeap(dd, rt, 1);
        Cudd_AutodynDisable(dd);
*/

        // build the group tree for reordering and free it afterwards
        if(dd->tree) Cudd_FreeTree(dd);
        dd->tree = Mtr_InitGroupTree(0, initVarSize);
        dd->tree->index = dd->invperm[0];
        Cudd_ReduceHeap(dd, rt, 1);
        if(dd->tree) Cudd_FreeTree(dd);

        // check var. order
        //for(i=0; i<Cudd_ReadSize(dd); ++i)
        //    cout << i << " -> " << cuddI(dd, i) << endl;
        for(i=initVarSize; i<Cudd_ReadSize(dd); ++i)
            assert(i == cuddI(dd, i));

        // update perm
        for(i=0; i<initVarSize; ++i)
            perm[i] = cuddI(dd, i);

        if(logger) logger->log("reord");

    }
    
    // collecting states at each timeframe
    size_t stsSum = 1;
    st__table *csts = createDummyState(dd), *nsts;
    
    for(i=0; i<nTimeFrame; ++i) {
        // cut set enumeration
        if(i != nTimeFrame-1)
            nsts = Extra_bddNodePathsUnderCut(dd, H, nVar*(i+1));
        else
            nsts = createDummyState(dd);
        if(logger) logger->log("cut-" + to_string(i));

        stsSum += st__count(csts);
        
        if(verbosity) cout << "@t=" << i << ":" << setw(7) << st__count(csts) << " states,";

        buildTrans(dd, pNodeVec, B, nVar, nCo, nTimeFrame, i, csts, nsts, stg);
        if(logger) logger->log("trans-" + to_string(i));

        // replace csts with nsts, and enter next iteration
        bddFreeTable(dd, csts);
        csts = nsts;
        
        if(verbosity) cout << Cudd_ReadNodeCount(dd) << " nodes" << endl;
        if(logger) logger->log("next-" + to_string(i));
    }
    

    // free arrays
    bddFreeVec(dd, B, 2*nCo);
    bddFreeTable(dd, csts);
    bddFreeVec(dd, pNodeVec, nCo);
    
    if(verbosity) {
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

    return stsSum;
}


} // end namespace timeMux::bddUtils
