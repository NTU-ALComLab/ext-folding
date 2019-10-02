#include "ext-folding/timeMux2/timeMux2.h"
#undef b1

#include <algorithm>
#include <boost/dynamic_bitset.hpp>

typedef boost::dynamic_bitset<> SupVec;
typedef vector<SupVec> SupVecs;

namespace timeMux2
{

static bool checkPerm(int *perm, cuint size, cuint cap)
{
    SupVec sv(cap);
    for(size_t i=0; i<size; ++i) {
        if((perm[i] < 0) || (perm[i] >= cap) || sv[perm[i]]) return false;
        sv[perm[i]] = 1;
    }
    return true;
}

// initializes the support vector of each circuit output
static void initSupVecs(Abc_Ntk_t *pNtk, DdManager *dd, DdNode **nodeVec, SupVecs &sVecs, cuint nCi, TimeLogger *logger)
{
    int i;  Abc_Obj_t *pObj;  DdNode *sup;
    int *arr = new int[nCi];

    SupVec sVec(nCi);
    Abc_NtkForEachCo(pNtk, pObj, i) {
        nodeVec[i] = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
        sup = Cudd_Support(dd, nodeVec[i]);  Cudd_Ref(sup);
        assert(Cudd_BddToCubeArray(dd, sup, arr));
        for(size_t j=0; j<nCi; ++j)
            sVec[j] = (arr[j] != 2);
        sVecs.push_back(sVec);
        Cudd_RecursiveDeref(dd, sup);
    }
    delete [] arr;

    if(logger) logger->log("init SupVecs");
}

// comparison function for SupVec
class SupVecCompFunc
{
  public:
    SupVecCompFunc(SupVecs& svs): sVecs(svs) {}
    bool operator() (cuint x, cuint y) {
        return (sVecs[x].count() < sVecs[y].count());
    }
  private:
    SupVecs &sVecs;
};

// reorders the circuit output, returns the configuration of each time-slot
// remember to free the returned "slots" array afterwards
static int* reordPO(SupVecs &sVecs, cuint nTimeFrame, cuint nCi, cuint nCo, cuint nPi, size_t &nPo, int *oPerm, TimeLogger *logger)
{
    // sort each CO by #1s in its support vector
    size_t *sIdx = new size_t[nCo];
    for(size_t i=0; i<nCo; ++i) sIdx[i] = i;
    sort(sIdx, sIdx+nCo, SupVecCompFunc(sVecs));
  /*  print sorted info.
    for(size_t i=0; i<nCo; ++i) {
        cout << sIdx[i] << ": ";
        for(size_t j=0; j<sVecs[sIdx[i]].size(); ++j)
            cout << sVecs[sIdx[i]][j];
        cout << " -> " << sVecs[sIdx[i]].count() << endl;
    }
  */

    int *slots = NULL;
    size_t nSlot;
    bool legal = false;
    do {
        SupVec sv(nCi);
        nSlot = nPo * nTimeFrame;
        cuint cap = nSlot - nCo;
        size_t cnt = 0, ct1 = 0, ct2 = 0, t;
        slots = new int[nSlot];
        for(size_t i=0; i<nSlot; ++i) slots[i] = -1;

        for(size_t i=0; i<nCo; ++i) {
            assert((ct1 < nTimeFrame) && (ct2 < nPo));
            sv |= sVecs[sIdx[i]];
            
            // current CO can be put at t^th time-frame
            t = (size_t)ceil(float(sv.count()) / float(nPi));
            if(t) --t;

            if(t <= ct1) {
                slots[ct1 * nPo + ct2] = sIdx[i];
                if((++ct2 == nPo) && ++ct1) ct2 = 0;
            } else { // t > ct1
                cnt += nPo * (t - ct1);
                cnt -= ct2;
                if(cnt > cap) break;

                slots[t*nPo] = sIdx[i];
                ct1 = t;  ct2 = 1;
            }
            
            if(i == nCo-1) legal = true;
        }

        if(!legal) { delete [] slots;  slots = NULL; }
    } while(!legal && ++nPo);
    assert(legal && slots && (nSlot == nPo * nTimeFrame));

    // update oPerm
    //for(size_t i=0; i<nSlot; ++i) cout << slots[i] << " ";
    //cout << endl;
    for(size_t i=0; i<nSlot; ++i) if(slots[i] != -1)
        oPerm[slots[i]] = i;
    assert(checkPerm(oPerm, nCo, nSlot));

    if(logger) logger->log("reord PO");

    //delete [] slots;
    delete [] sIdx;
    return slots;
}

//static size_t reordPO2()  // better pin sharing

// places PIs at correct positions and reduces BDD size
static void reordPI(DdManager *dd, SupVecs &sVecs, int *slots, cuint nTimeFrame, cuint nCi, cuint nPi, cuint nPo, int *iPerm, TimeLogger *logger)
{
    size_t pos = 0;  // current PI position
    vector<size_t> sNums;  sNums.reserve(nTimeFrame);  // for reordering
    SupVec sv1(nCi);  // accumulated supports
    for(size_t t=0; t<nTimeFrame; ++t) {
        size_t ppos = pos;
        SupVec sv2(nCi);  // supports of POs at current time-frame
        SupVec sv3(sv1);  // a copy of sv1
        for(size_t i=0; i<nPo; ++i) if(slots[t*nTimeFrame + i] != -1)
            sv2 |= sVecs[slots[t*nTimeFrame + i]];
        
        sv1 |= sv2;  // upadate sv1
        sv3 ^= sv1;  // PIs needed at current time-frame

        // store invperm info. in iPerm
        for(size_t i=0; i<nCi; ++i) if(sv3[i])
            //iPerm[i] = pos++;  // perm
            iPerm[pos++] = i;  // invperm
        sNums.push_back(pos - ppos);
    }
    assert(checkPerm(iPerm, nCi, nCi) && (pos == nCi));

    // apply iPerm
    assert(Cudd_ShuffleHeap(dd, iPerm));

    // reord to minimize #nodes
    pos = 0;
    for(size_t& sz: sNums) if(sz) {
        bddUtils::bddReordRange(dd, pos, sz);
        pos += sz;
    }
    assert(pos == nCi);
    
    // update iPerm
    for(size_t i=0; i<nCi; ++i)
        iPerm[i] = dd->perm[i];
    assert(checkPerm(iPerm, nCi, nCi));
}

size_t reordIO(Abc_Ntk_t *pNtk, DdManager *dd, cuint nTimeFrame, int *iPerm, int *oPerm, TimeLogger *logger, const bool verbosity)
{
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nCo = Abc_NtkCoNum(pNtk);

    DdNode **nodeVec = new DdNode*[nCo];
    SupVecs sVecs;  sVecs.reserve(nCo);
    
    initSupVecs(pNtk, dd, nodeVec, sVecs, nCi, logger);
    
    // determine the lower bound of #pins
    size_t nPo = (size_t)ceil(float(nCo) / float(nTimeFrame));
    size_t nPi = nCi / nTimeFrame;

    int *slots = reordPO(sVecs, nTimeFrame, nCi, nCo, nPi, nPo, oPerm, logger);


    delete [] slots;
    delete [] nodeVec;
    return 0;
}

} // end namespace timeMux2

#define b1 (dd)->one