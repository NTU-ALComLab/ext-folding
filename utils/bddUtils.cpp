#include "ext-folding/utils/utils.h"

namespace utils::bddUtils
{

// print Cudd manager info.
void bddShowManInfo(DdManager *dd)
{
    printf("DdManager nodes: %ld | ", Cudd_ReadNodeCount(dd));          /*Reports the number of live nodes in BDDs and ADDs*/
    printf("DdManager vars: %d | ", Cudd_ReadSize(dd));                 /*Returns the number of BDD variables in existence*/
    printf("DdManager reorderings: %d | ", Cudd_ReadReorderings(dd));   /*Returns the number of times reordering has occurred*/
    printf("DdManager memory: %ld \n", Cudd_ReadMemoryInUse(dd));       /*Returns the memory in use by the manager measured in bytes*/
}


// print bdd node info.
void bddShowNodeInfo(DdManager *dd, DdNode *pNode) { Cudd_PrintDebug(dd, pNode, 2, 4); } 


// dump a bdd into image file in .png format
void bddDumpPng(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName)
{
    FILE *dotFile = fopen((fileName+string(".dot")).c_str(), "w");
    Cudd_DumpDot(dd, nNode, pNodeVec, NULL, NULL, dotFile); // dump the function to .dot file
    fclose(dotFile); // close the file
    string command("dot -Tpng -Gdpi=400 ");
    command += fileName + string(".dot > ") + fileName + string(".png");
    system(command.c_str());
}

void bddDumpBlif(DdManager *dd, DdNode **pNodeVec, int nNode, const string& fileName)
{
    FILE * fp = fopen((fileName+".blif").c_str(), "w");
    assert(Cudd_DumpBlif(dd, nNode, pNodeVec, NULL, NULL, NULL, fp, 0));
    fclose(fp);
}

// compute signature
// caution: remember to free the array afterwards!!!
DdNode** bddComputeSign(DdManager *dd, cuint range, int stIdx)
{
    // introduce variable alpha(a)
    // #a = log2(ceil(range))
    // a = [ a_n-1, a_n-2 , ... , a_1, a_0 ]
    uint initVarSize = (stIdx < 0) ? Cudd_ReadSize(dd) : stIdx;
    uint na = range ? (uint)ceil(log2(double(range))) : 0;
    DdNode **a = new DdNode*[na];
    //for(uint j=0; j<na; ++j) a[j] = Cudd_bddNewVar(dd);
    for(uint j=0; j<na; ++j) a[j] = Cudd_bddIthVar(dd, initVarSize+j);
    assert(initVarSize + na == Cudd_ReadSize(dd));
    
    // compute signature for each number within range
    DdNode **A = new DdNode*[range];
    int *cube = new int[initVarSize+na];
    for(uint j=0; j<initVarSize; ++j) cube[j] = 2;
    
    for(uint j=0; j<range; ++j) {
        // convert to bin, TODO: maybe replace with Extra_bddBitsToCube ??
        uint k = j;
        for(uint m=0; m<na; ++m) {
            cube[initVarSize+m] = k & 1;
            k >>= 1;
        }
        assert(k == 0);
        
        A[j] = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(A[j]);
    }
    
    delete [] a;
    delete [] cube;
    
    return A;
}


// compute inner product(dot) of 2 given bdd arrays
// return a bdd node with ref = 1
DdNode* bddDot(DdManager *dd, DdNode **v1, DdNode **v2, cuint len)
{
    DdNode *ret = Cudd_ReadLogicZero(dd);  Cudd_Ref(ret);
    DdNode *tmp1, *tmp2;
    for(uint i=0; i<len; ++i) {
        if((*(v1+i) == NULL) || (*(v1+i) == NULL)) continue;  // take care of NULL pointers
        tmp1 = Cudd_bddAnd(dd, *(v1+i), *(v2+i));  Cudd_Ref(tmp1);
        tmp2 = Cudd_bddOr(dd, ret, tmp1);  Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(dd, ret);
        Cudd_RecursiveDeref(dd, tmp1);
        ret = tmp2;
    }
    return ret;
}


// negate an array of bdd nodes
void bddNotVec(DdNode **vec, cuint len)
{
    for(uint i=0; i<len; ++i)  if(vec[i])
        vec[i] = Cudd_Not(vec[i]);
}

void bddDerefVec(DdManager *dd, DdNode **v, cuint len)
{
    for(uint i=0; i<len; ++i) if(v[i])
        Cudd_RecursiveDeref(dd, v[i]);
}

void bddFreeVec(DdManager *dd, DdNode **v, cuint len)
{
    bddDerefVec(dd, v, len);
    delete [] v;
}

st__table* bddCreateDummyState(DdManager *dd)
{
    st__table *ret = st__init_table(st__ptrcmp, st__ptrhash);
    Cudd_Ref(b1);
    st__insert(ret, (char*)b1, (char*)b1);
    return ret;
}

void bddFreeTable(DdManager *dd, st__table *tb)
{
    st__generator *gen;
    DdNode *kNode, *vNode;
    
    st__foreach_item(tb, gen, (const char**)&kNode, (char**)&vNode)
        Cudd_RecursiveDeref(dd, vNode);
        
    st__free_table(tb);
}

void bddReordRange(DdManager *dd, cuint lev, cuint size, const Cudd_ReorderingType rt)
{
    MtrNode *bak = dd->tree;
    // build the group tree for reordering and free it afterwards
    dd->tree = Mtr_InitGroupTree(0, size);
    dd->tree->index = dd->invperm[lev];
    Cudd_ReduceHeap(dd, rt, 1);
    Cudd_FreeTree(dd);
    dd->tree = bak;
}

} // end namespace utils::bddUtils