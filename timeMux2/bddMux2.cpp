#include "ext-folding/timeMux2/timeMux2.h"
#include "ext-folding/timeFold/timeFold.h"

using namespace timeMux2;
using namespace bddUtils;

namespace timeMux2
{

STG* bddMux2(Abc_Ntk_t *pNtk, cuint nTimeFrame, uint &nPo, int *iPerm, int *oPerm, const bool verbose, const char *logFileName, cuint expConfig)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;
    
    // initialize bdd manger
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, ABC_INFINITY, 1, 1, 0, 0);
    if(!dd) {
        cerr << "#nodes exceeds the maximum limit." << endl;
        return NULL;
    }
    if(logger) logger->log("init bdd man");

    // get basic settings
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint nPi = nCi / nTimeFrame;
    nPo = reordIO(pNtk, dd, nTimeFrame, iPerm, oPerm, logger, verbose, expConfig);

    // init STG
    STG *stg = new STG(nCi, nCo, nPi, nPo, nTimeFrame);

    DdNode **nodeVec = new DdNode*[nPo * nTimeFrame];
    for(uint i=0; i<nPo*nTimeFrame; ++i)
        nodeVec[i] = NULL;
    for(uint i=0; i<nCo; ++i)
        nodeVec[oPerm[i]] = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk, i));  // ref = 1

    // compute signature
    uint nB = nPo ? (uint)ceil(log2(double(nPo*2))) : 0;
    DdNode **B = bddComputeSign(dd, nPo*2);
    assert(nCi+nB == Cudd_ReadSize(dd));

    // collecting states at each timeframe
    uint stsSum = 1;
    st__table *csts, *nsts = bddCreateDummyState(dd);
    st__generator *nGen;
    DdNode *nKNode, *nVNode;
    DdNode *tmp1, *tmp2;

    for(int i=nTimeFrame-1; i>=0; --i) {
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
            for(uint j=0; j<na; ++j) a[j] = Cudd_bddIthVar(dd, nCi+nB+j);

            // hyper-function encoding of output functions of i^th time-frame
            DdNode *ft = bddDot(dd, nodeVec+i*nPo, B, nPo);
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

            // cut set enumeration
            csts = Extra_bddNodePathsUnderCut(dd, tmp1, nPi*i);

            Cudd_RecursiveDeref(dd, tmp1);

            // free arrays
            delete [] a;
            delete [] pFuncs;
        }
        else csts = bddCreateDummyState(dd);  // i==0

        stsSum += st__count(csts);
        if(verbose) cout << setw(7) << st__count(csts) << " states: ";

        timeFold::buildTrans(dd, nodeVec, B, nPi, nPo, nTimeFrame, i, csts, nsts, stg);

        // replace nsts with csts, and enter next iteration
        bddFreeTable(dd, nsts);
        nsts = csts;
        
        if(verbose)
            cout << Cudd_ReadNodeCount(dd) << " nodes" << endl;
    }

    stg->setNSts(stsSum);  // update #states

    // free arrays
    bddFreeVec(dd, B, 2*nPo);
    bddFreeTable(dd, nsts);
    bddFreeVec(dd, nodeVec, nPo*nTimeFrame);
    
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


} // end namespace timeMux2


