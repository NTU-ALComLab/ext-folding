#include "ext-folding/timeMux2/timeMux2.h"

using namespace timeMux2;
using namespace bddUtils;

namespace timeMux2
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
            F = dd->one; Cudd_Ref(F);
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


int bddMux2(Abc_Ntk_t *pNtk, cuint nTimeFrame, int *iPerm, int *oPerm, vector<string>& stg, const bool verbosity, const char *logFileName)
{
    TimeLogger *logger = logFileName ? (new TimeLogger(logFileName)) : NULL;
    
    // initialize bdd manger
    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtk, ABC_INFINITY, 1, 1, 0, 0);
    if(!dd) {
        cerr << "#nodes exceeds the maximum limit." << endl;
        return 0;
    }
    if(logger) logger->log("init bdd man");

    reordIO(pNtk, dd, nTimeFrame, iPerm, oPerm, logger, verbosity);

    // get basic settings
    cuint nCo = Abc_NtkCoNum(pNtk);
    cuint initVarSize = Cudd_ReadSize(dd);
    cuint nVar = initVarSize / nTimeFrame;
    

    // free bdd manger
    Abc_NtkFreeGlobalBdds(pNtk, 0);
    Cudd_Quit(dd);
    
    if(logger) {
        logger->log("free memory");
        delete logger;
    }

    return 0;
}


} // end namespace timeMux2


