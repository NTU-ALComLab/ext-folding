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
    for(uint i=0; i<size; ++i) {
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
        for(uint j=0; j<nCi; ++j)
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
    SupVecCompFunc(const SupVecs& svs): sVecs(svs) {}
    bool operator() (cuint x, cuint y) {
        return (sVecs[x].count() < sVecs[y].count());
    }
  private:
    const SupVecs &sVecs;
};

// reorders the circuit output, returns the configuration of each time-slot
// remember to free the returned "slots" array afterwards
static int* reordPO(const SupVecs &sVecs, cuint nTimeFrame, cuint nCi, cuint nCo, cuint nPi, uint &nPo, int *oPerm, TimeLogger *logger)
{
    // sort each CO by #1s in its support vector
    uint *sIdx = new uint[nCo];
    for(uint i=0; i<nCo; ++i) sIdx[i] = i;
    sort(sIdx, sIdx+nCo, SupVecCompFunc(sVecs));
  /*  print sorted info.
    for(uint i=0; i<nCo; ++i) {
        cout << sIdx[i] << ": ";
        for(uint j=0; j<sVecs[sIdx[i]].size(); ++j)
            cout << sVecs[sIdx[i]][j];
        cout << " -> " << sVecs[sIdx[i]].count() << endl;
    }
  */

    int *slots = NULL;
    uint nSlot;
    bool legal = false;
    do {
        SupVec sv(nCi);
        nSlot = nPo * nTimeFrame;
        cuint cap = nSlot - nCo;
        uint cnt = 0, ct1 = 0, ct2 = 0, t;
        slots = new int[nSlot];
        for(uint i=0; i<nSlot; ++i) slots[i] = -1;

        for(uint i=0; i<nCo; ++i) {
            assert((ct1 < nTimeFrame) && (ct2 < nPo));
            sv |= sVecs[sIdx[i]];
            
            // current CO can be put at t^th time-frame
            t = (uint)ceil(float(sv.count()) / float(nPi));
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
    //for(uint i=0; i<nSlot; ++i) cout << slots[i] << " ";
    //cout << endl;
    for(uint i=0; i<nSlot; ++i) if(slots[i] != -1)
        oPerm[slots[i]] = i;
    assert(checkPerm(oPerm, nCo, nSlot));

    if(logger) logger->log("reord PO");

    //delete [] slots;
    delete [] sIdx;
    return slots;
}

static double getScore(const vector<SupVecs> &sVecss, cuint t, cuint i, const double ratio = 0.9)
{
    if(sVecss[t][i].count() == 0) return 0.0;
    double ret = 0.0;
    for(uint tt = 0; tt<t; ++tt)
        ret += double((sVecss[t][i] & sVecss[tt][i]).count()) * pow(ratio, t-tt-1);
    return ret;
}

// better pin sharing
static void reordPO2(DdManager *dd, const SupVecs &sVecs, int *slots, cuint nTimeFrame, cuint nCi, cuint nCo, cuint nPi, cuint nPo, int *oPerm, TimeLogger *logger)
{
    vector<SupVecs> sVecss;  sVecss.reserve(nTimeFrame);
    SupVec csv(nCi);
    for(uint t=0; t<nTimeFrame; ++t) {
        sVecss.push_back(SupVecs(nPo, SupVec(nCi)));
        for(uint i=0; i<nPi; ++i)
            csv[dd->invperm[t*nPi + i]] = 1;
        assert(csv.count() == nPi*(t+1));

        for(uint i=0; i<nPo; ++i) if(slots[t*nPo + i] != -1) {
            sVecss[t][i] = csv ^ sVecs[slots[t*nPo + i]];
            assert(sVecss[t][i].count() <= nPi);
        }
    }

    for(uint t=1; t<nTimeFrame; ++t) for(uint i=0; i<nPo; ++i) {
        double maxScore = -1.0, score;
        int maxIdx = -1;
        for(uint j=i; j<nPo; ++j) {
            // compute score
            score = getScore(sVecss, t, j);
            if(score > maxScore) {
                maxScore = score;
                maxIdx = j;
            }
        }
        assert((maxScore > 0.0) && (maxIdx > 0));

        if(maxIdx != i) {
            swap(slots[t*nPo + i], slots[t*nPo + maxIdx]);
            swap(sVecss[t][i], sVecss[t][maxIdx]);
        }
    }

    // update oPerm
    for(uint i=0; i<nPo*nTimeFrame; ++i) if(slots[i] != -1)
        oPerm[slots[i]] = i;
    assert(checkPerm(oPerm, nCo, nPo*nTimeFrame));

    if(logger) logger->log("reord PO-2");
}

// places PIs at correct positions and reduces BDD size
static void reordPI(DdManager *dd, const SupVecs &sVecs, int *slots, cuint nTimeFrame, cuint nCi, cuint nPo, int *iPerm, TimeLogger *logger)
{
    uint pos = 0;  // current PI position
    vector<uint> sNums;  sNums.reserve(nTimeFrame);  // for reordering
    SupVec sv1(nCi);  // accumulated supports
    for(uint t=0; t<nTimeFrame; ++t) {
        uint ppos = pos;
        SupVec sv2(nCi);  // supports of POs at current time-frame
        SupVec sv3(sv1);  // a copy of sv1
        for(uint i=0; i<nPo; ++i) if(slots[t*nPo + i] != -1)
            sv2 |= sVecs[slots[t*nPo + i]];
        
        sv1 |= sv2;  // upadate sv1
        sv3 ^= sv1;  // PIs needed at current time-frame

        // store invperm info. in iPerm
        for(uint i=0; i<nCi; ++i) if(sv3[i])
            //iPerm[i] = pos++;  // perm
            iPerm[pos++] = i;  // invperm
        sNums.push_back(pos - ppos);
    }
    assert(checkPerm(iPerm, nCi, nCi) && (pos == nCi));

    // apply iPerm
    assert(Cudd_ShuffleHeap(dd, iPerm));

    // reord to minimize #nodes
    pos = 0;
    for(uint& sz: sNums) if(sz) {
        bddUtils::bddReordRange(dd, pos, sz);
        pos += sz;
    }
    assert(pos == nCi);
    
    // update iPerm
    for(uint i=0; i<nCi; ++i)
        iPerm[i] = dd->perm[i];
    assert(checkPerm(iPerm, nCi, nCi));

    if(logger) logger->log("reord PI");
}

uint reordIO(Abc_Ntk_t *pNtk, DdManager *dd, cuint nTimeFrame, int *iPerm, int *oPerm, TimeLogger *logger, const bool verbosity)
{
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nCo = Abc_NtkCoNum(pNtk);

    DdNode **nodeVec = new DdNode*[nCo];
    SupVecs sVecs;  sVecs.reserve(nCo);
    
    initSupVecs(pNtk, dd, nodeVec, sVecs, nCi, logger);
    
    // determine the lower bound of #pins
    uint nPo = (uint)ceil(float(nCo) / float(nTimeFrame));
    uint nPi = nCi / nTimeFrame;

    int *slots = reordPO(sVecs, nTimeFrame, nCi, nCo, nPi, nPo, oPerm, logger);
    reordPI(dd, sVecs, slots, nTimeFrame, nCi, nPo, iPerm, logger);
    reordPO2(dd, sVecs, slots, nTimeFrame, nCi, nCo, nPi, nPo, oPerm, logger);


    delete [] slots;
    delete [] nodeVec;
    return 0;
}

} // end namespace timeMux2

#define b1 (dd)->one