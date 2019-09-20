

#include "ext-folding/timeMux/timeMux.h"

extern "C"
{
Abc_Ntk_t* Abc_NtkDC2(Abc_Ntk_t *pNtk, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fVerbose);
}

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

// modify from base/abci/abcNtbdd.c/Abc_NodeGlobalBdds_rec
DdNode* buildGlobalBddNodes_rec(DdManager *dd, Abc_Obj_t *pNode, int nBddSizeMax, int fDropInternal, ProgressBar *pProgress, int *pCounter, int fVerbose)
{
    DdNode * bFunc, * bFunc0, * bFunc1, * bFuncC;
    int fDetectMuxes = 0;
    assert( !Abc_ObjIsComplement(pNode) );
    if ( Cudd_ReadKeys(dd)-Cudd_ReadDead(dd) > (unsigned)nBddSizeMax )
    {
        Extra_ProgressBarStop( pProgress );
        if ( fVerbose )
        printf( "The number of live nodes reached %d.\n", nBddSizeMax );
        fflush( stdout );
        return NULL;
    }
    // if the result is available return
    if ( Abc_ObjGlobalBdd(pNode) == NULL )
    {
        Abc_Obj_t * pNodeC, * pNode0, * pNode1;
        pNode0 = Abc_ObjFanin0(pNode);
        pNode1 = Abc_ObjFanin1(pNode);
        // check for the special case when it is MUX/EXOR
        if ( fDetectMuxes && 
             Abc_ObjGlobalBdd(pNode0) == NULL && Abc_ObjGlobalBdd(pNode1) == NULL &&
             Abc_ObjIsNode(pNode0) && Abc_ObjFanoutNum(pNode0) == 1 && 
             Abc_ObjIsNode(pNode1) && Abc_ObjFanoutNum(pNode1) == 1 && 
             Abc_NodeIsMuxType(pNode) )
        {
            // deref the fanins
            pNode0->vFanouts.nSize--;
            pNode1->vFanouts.nSize--;
            // recognize the MUX
            pNodeC = Abc_NodeRecognizeMux( pNode, &pNode1, &pNode0 );
            assert( Abc_ObjFanoutNum(pNodeC) > 1 );
            // dereference the control once (the second time it will be derefed when BDDs are computed)
            pNodeC->vFanouts.nSize--;

            // compute the result for all branches
            bFuncC = buildGlobalBddNodes_rec( dd, pNodeC, nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFuncC == NULL )
                return NULL;
            Cudd_Ref( bFuncC );
            bFunc0 = buildGlobalBddNodes_rec( dd, Abc_ObjRegular(pNode0), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc0 == NULL )
                return NULL;
            Cudd_Ref( bFunc0 );
            bFunc1 = buildGlobalBddNodes_rec( dd, Abc_ObjRegular(pNode1), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc1 == NULL )
                return NULL;
            Cudd_Ref( bFunc1 );

            // complement the branch BDDs
            bFunc0 = Cudd_NotCond( bFunc0, (int)Abc_ObjIsComplement(pNode0) );
            bFunc1 = Cudd_NotCond( bFunc1, (int)Abc_ObjIsComplement(pNode1) );
            // get the final result
            bFunc = Cudd_bddIte( dd, bFuncC, bFunc1, bFunc0 );   Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bFunc0 );
            Cudd_RecursiveDeref( dd, bFunc1 );
            Cudd_RecursiveDeref( dd, bFuncC );
            // add the number of used nodes
            (*pCounter) += 3;
        }
        else
        {
            // compute the result for both branches
            bFunc0 = buildGlobalBddNodes_rec( dd, Abc_ObjFanin(pNode,0), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc0 == NULL )
                return NULL;
            Cudd_Ref( bFunc0 );
            bFunc1 = buildGlobalBddNodes_rec( dd, Abc_ObjFanin(pNode,1), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc1 == NULL )
                return NULL;
            Cudd_Ref( bFunc1 );
            bFunc0 = Cudd_NotCond( bFunc0, (int)Abc_ObjFaninC0(pNode) );
            bFunc1 = Cudd_NotCond( bFunc1, (int)Abc_ObjFaninC1(pNode) );
            // get the final result
            bFunc = Cudd_bddAndLimit( dd, bFunc0, bFunc1, nBddSizeMax );
            if ( bFunc == NULL )
                return NULL;
            Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bFunc0 );
            Cudd_RecursiveDeref( dd, bFunc1 );
            // add the number of used nodes
            (*pCounter)++;
        }
        // set the result
        assert( Abc_ObjGlobalBdd(pNode) == NULL );
        Abc_ObjSetGlobalBdd( pNode, bFunc );
        // increment the progress bar
        if ( pProgress )
            Extra_ProgressBarUpdate( pProgress, *pCounter, NULL );
    }
    // prepare the return value
    bFunc = (DdNode *)Abc_ObjGlobalBdd(pNode);
    // dereference BDD at the node
    if ( --pNode->vFanouts.nSize == 0 && fDropInternal )
    {
        Cudd_Deref( bFunc );
        Abc_ObjSetGlobalBdd( pNode, NULL );
    }
    return bFunc;
}

// modify from base/abci/abcNtbdd.c/Abc_NtkBuildGlobalBdds
DdManager* buildGlobalBdd(Abc_Ntk_t *pNtk, int nBddSizeMax, int fReorder, int initVarSize, Cudd_ReorderingType rt)
{
    int fDropInternal = 1, fReverse = 0, fVerbose = 0;
    ProgressBar * pProgress;
    Abc_Obj_t * pObj, * pFanin;
    Vec_Att_t * pAttMan;
    DdManager * dd;
    DdNode * bFunc;
    int i, k, Counter;

    // remove dangling nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );

    // start the manager
    assert( Abc_NtkGlobalBdd(pNtk) == NULL );
    dd = Cudd_Init( Abc_NtkCiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    pAttMan = Vec_AttAlloc( Abc_NtkObjNumMax(pNtk) + 1, dd, (void (*)(void*))Extra_StopManager, NULL, (void (*)(void*,void*))Cudd_RecursiveDeref );
    Vec_PtrWriteEntry( pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan );

    // set reordering
    if ( fReorder ) {
        // build the group tree for reordering and free it afterwards
        if(dd->tree) Cudd_FreeTree(dd);
        dd->tree = Mtr_InitGroupTree(0, initVarSize);
        dd->tree->index = dd->invperm[0];
        Cudd_AutodynEnable( dd, rt );
    }

    // assign the constant node BDD
    pObj = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pObj) > 0 )
    {
        bFunc = dd->one;
        Abc_ObjSetGlobalBdd( pObj, bFunc );   Cudd_Ref( bFunc );
    }
    // set the elementary variables
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            bFunc = fReverse ? dd->vars[Abc_NtkCiNum(pNtk) - 1 - i] : dd->vars[i];
            Abc_ObjSetGlobalBdd( pObj, bFunc );  Cudd_Ref( bFunc );
        }

    // collect the global functions of the COs
    Counter = 0;
    // construct the BDDs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkNodeNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        bFunc = buildGlobalBddNodes_rec( dd, Abc_ObjFanin0(pObj), nBddSizeMax, fDropInternal, pProgress, &Counter, fVerbose );
        if ( bFunc == NULL )
        {
            if ( fVerbose )
            printf( "Constructing global BDDs is aborted.\n" );
            Abc_NtkFreeGlobalBdds( pNtk, 0 );
            Cudd_Quit( dd ); 

            // reset references
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
                    pObj->vFanouts.nSize = 0;
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                    Abc_ObjForEachFanin( pObj, pFanin, k )
                        pFanin->vFanouts.nSize++;
            return NULL;
        }
        bFunc = Cudd_NotCond( bFunc, (int)Abc_ObjFaninC0(pObj) );  Cudd_Ref( bFunc ); 
        Abc_ObjSetGlobalBdd( pObj, bFunc );
    }
    Extra_ProgressBarStop( pProgress );

    // reset references
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
            pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                pFanin->vFanouts.nSize++;

    // reorder one more time
    if ( fReorder )
    {
        Cudd_ReduceHeap( dd, rt, 1 );
        Cudd_AutodynDisable( dd );
        if(dd->tree) Cudd_FreeTree(dd);
    }
    return dd;
}

// returns the Id of the hyper-function output
int buildAigHyper(Abc_Ntk_t *&pNtk, cuint optRnd = 3)
{
    int i; Abc_Obj_t *pObj;
    size_t nCo = Abc_NtkCoNum(pNtk);
    size_t nCi = Abc_NtkCiNum(pNtk);
    size_t nv = nCo ? (size_t)ceil(log2(double(nCo))) : 0;
    
    Abc_Obj_t **B = aigUtils::aigComputeSign(pNtk, nCo, false);
    assert(nCi + nv == Abc_NtkCiNum(pNtk));

    Abc_Obj_t **A = new Abc_Obj_t*[nCo];
    Abc_NtkForEachCo(pNtk, pObj, i)
        A[i] = Abc_ObjFanin0(pObj);
    
    pObj = aigUtils::aigDot(pNtk, A, B, nCo);
    Abc_Obj_t *hPo = Abc_NtkCreatePo(pNtk);
    Abc_ObjAddFanin(hPo, pObj);
    Abc_ObjAssignName(hPo, "hyper-function", NULL);

    delete [] B;
    delete [] A;

    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    assert(Abc_NtkCheck(pNtk));

    for(i=0; i<optRnd; ++i) {
        Abc_Ntk_t *pNtkOpt = Abc_NtkDC2(pNtk, i%2, 0, 1, 0, 0);
        Abc_NtkDelete(pNtk);
        pNtk = pNtkOpt;
    }
    Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
    assert(Abc_NtkCheck(pNtk));

    return Abc_NtkCo(pNtk, nCo)->Id;
}

int bddMux(Abc_Ntk_t *pNtk, cuint nTimeFrame, vector<string>& stg, size_t *perm, const bool verbosity, const char *logFileName, const Cudd_ReorderingType rt)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;

/*  build hyper-function first, then collapse  */
    // get basic settings
    pNtk = Abc_NtkDup(pNtk);
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint initVarSize = Abc_NtkCiNum(pNtk);
    cuint nVar = initVarSize / nTimeFrame;

    const int hId = buildAigHyper(pNtk);
    if(logger) logger->log("build hyper");

    DdManager *dd = buildGlobalBdd(pNtk, ABC_INFINITY, (int)(perm!=NULL), initVarSize, rt);
    if(!dd) {
        cerr << "#nodes exceeds the maximum limit." << endl;
        return 0;
    }
    if(logger) logger->log("init bdd man");

    int i, j = 0;  Abc_Obj_t *pObj;
    DdNode **pNodeVec = new DdNode*[nCo], *H = NULL;
    Abc_NtkForEachCo(pNtk, pObj, i) {
        if(pObj->Id != hId) pNodeVec[j++] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
        else H = (DdNode *)Abc_ObjGlobalBdd(pObj);
    }
    assert((j == nCo) && H);

    // compute signature
    size_t nB = nCo ? (size_t)ceil(log2(double(nCo*2))) : 0;
    DdNode **B = bddComputeSign(dd, nCo*2, initVarSize);
    assert(initVarSize+nB == Cudd_ReadSize(dd));
    if(logger) logger->log("prepare sign");

    if(perm) {
        // check var. order
        for(i=initVarSize; i<Cudd_ReadSize(dd); ++i)
            assert(i == cuddI(dd, i));
        // update perm
        for(i=0; i<initVarSize; ++i)
            perm[i] = cuddI(dd, i);
    }

    
/*  collapse first, then build hyper-function on BDD
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
    DdNode **B = bddComputeSign(dd, nCo*2);
    assert(initVarSize+nB == Cudd_ReadSize(dd));
    if(logger) logger->log("prepare sign");

    // compute hyper-function
    DdNode *H = bddDot(dd, pNodeVec, B, nCo);
    if(logger) logger->log("build hyper");

    // build hyper-function with bdd extra utils by Alan
    // DdNode **b = new DdNode*[nB+1];
    // for(i=0; i<nB+1; ++i) b[i] = Cudd_bddIthVar(dd, initVarSize+i);
    // DdNode *H = Extra_bddEncodingBinary(dd, pNodeVec, nCo, b, nB+1);  Cudd_Ref(H);
    // delete [] b;

    // reorder bdd
    if(perm) {
        // Cudd_AutodynEnable(dd, rt);
        // // fixed B var. order
        // for(i=initVarSize; i<Cudd_ReadSize(dd); ++i)
        //     assert(Cudd_bddBindVar(dd, i));
        // Cudd_ReduceHeap(dd, rt, 1);
        // Cudd_AutodynDisable(dd);

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
*/

    // collecting states at each timeframe
    size_t stsSum = 1;
    st__table *csts = bddCreateDummyState(dd), *nsts;
    
    for(i=0; i<nTimeFrame; ++i) {
        // cut set enumeration
        if(i != nTimeFrame-1)
            nsts = Extra_bddNodePathsUnderCut(dd, H, nVar*(i+1));
        else
            nsts = bddCreateDummyState(dd);
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
    Abc_NtkDelete(pNtk);
    
    
    if(logger) {
        logger->log("free memory");
        delete logger;
    }

    return stsSum;
}


} // end namespace timeMux::bddUtils
