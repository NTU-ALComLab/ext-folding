#include "ext-folding/timeMux2/timeMux2.h"
#include "ext-folding/utils/supVec.h"

using namespace SupVech;

namespace timeMux2
{

bool checkPerm(int *perm, cuint size, cuint cap)
{
    SupVec sv(cap);
    for(uint i=0; i<size; ++i) {
        if(perm[i] < 0) {
            cout << "neg val" << endl;
            return false;
        } else if(perm[i] >= cap) {
            cout << "exceed cap" << endl;
            return false;
        } else if(sv[perm[i]]) {
            cout << "repeat key " << i << "->" << perm[i] << endl;
            return false;
        }
        //if((perm[i] < 0) || (perm[i] >= cap) || sv[perm[i]]) return false;
        sv[perm[i]] = 1;
    }
    return true;
}

// initializes the support vector of each circuit output
static void initSupVecs(Abc_Ntk_t *pNtk, DdManager *dd, SupVecs &sVecs, cuint nCi, TimeLogger *logger)
{
    int i;  Abc_Obj_t *pObj;
    DdNode *node, *sup;
    int *arr = new int[nCi];

    SupVec sVec(nCi);
    Abc_NtkForEachCo(pNtk, pObj, i) {
        node = (DdNode *)Abc_ObjGlobalBdd(pObj);  // ref = 1
        sup = Cudd_Support(dd, node);  Cudd_Ref(sup);
        assert(Cudd_BddToCubeArray(dd, sup, arr));
        for(uint j=0; j<nCi; ++j)
            sVec[j] = (arr[j] != 2);
        sVecs.push_back(sVec);
        Cudd_RecursiveDeref(dd, sup);
    }
    delete [] arr;

    if(logger) logger->log("init SupVecs");
}

// reorders the circuit output, returns the configuration of each time-slot
// remember to free the returned "slots" array afterwards
static int* schedulePO(const SupVecs &sVecs, cuint nTimeFrame, cuint nCi, cuint nCo, cuint nPi, uint &nPo, int *oPerm, TimeLogger *logger, const bool verbose)
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
                if((ct2 == nPo) && ++ct1) ct2 = 0;
            }

            if(i == nCo-1) legal = true;
        }

        if(!legal) { delete [] slots;  slots = NULL; }
    } while(!legal && ++nPo);
    assert(legal && slots && (nSlot == nPo * nTimeFrame));

    //for(uint i=0; i<nSlot; ++i) cout << slots[i] << " ";
    //cout << endl;
    // update oPerm
    for(uint i=0; i<nSlot; ++i) if(slots[i] != -1)
        oPerm[slots[i]] = i;
    assert(checkPerm(oPerm, nCo, nSlot));

    //delete [] slots;
    delete [] sIdx;

    if(verbose) {
        cout << "schedule PO:\n";
        cout << "-slots:\n";
        for(uint t=0; t<nTimeFrame; ++t) {
            cout << "\t";
            for(uint i=0; i<nPo; ++i) cout << setw(4) << slots[t*nPo + i] << " ";
            cout << "\n";
        }
        cout << "-oPerm:\n\t";
        for(uint i=0; i<nCo; ++i) cout << setw(4) << oPerm[i] << " ";
        cout << "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    }

    if(logger) logger->log("schedule PO");
    
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
static void reordPO(DdManager *dd, const SupVecs &sVecs, int *slots, cuint nTimeFrame, cuint nCi, cuint nCo, cuint nPi, cuint nPo, int *oPerm, TimeLogger *logger, const bool verbose)
{
  /*
    for(uint i=0; i<sVecs.size(); ++i)
        cout << i << ": " << sVecs[i] << endl;
  */

    vector<SupVecs> sVecss;  sVecss.reserve(nTimeFrame);
    SupVec psv(nCi);
    for(uint t=0; t<nTimeFrame; ++t) {
        if(t) for(uint i=0; i<nPi; ++i)
            psv[dd->invperm[(t-1)*nPi + i]] = 1;
        assert(psv.count() == nPi*t);

        sVecss.push_back(SupVecs(nPo, SupVec(nPi)));

        for(uint i=0; i<nPo; ++i) if(slots[t*nPo + i] != -1) {
            const SupVec sv =  sVecs[slots[t*nPo + i]] ^ (psv & sVecs[slots[t*nPo + i]]);
            assert(sv.count() <= nPi);
            for(uint j=0; j<sv.size(); ++j) if(sv[j]) {
                cuint pos = cuddI(dd, j) % nPi;
                assert(!sVecss[t][i][pos]);
                sVecss[t][i][pos] = 1;
            }
        }
    }

  /*
    for(SupVecs& svs: sVecss) {
        for(SupVec& sv: svs) {
            for(uint i=0; i<sv.size(); ++i) cout << sv[i];
            cout << "\n";
        }
        cout << "--------------------------\n";
    }
  */

    if(verbose) cout << "reord PO:\n-scoring:\n";
    for(uint t=1; t<nTimeFrame; ++t) for(uint i=0; i<nPo; ++i) {
        double maxScore = -1.0, score;
        int maxIdx = -1;
        if(verbose) cout << "\tround " << t << "," << i << ": ";
        for(uint j=i; j<nPo; ++j) {
            // compute score
            score = getScore(sVecss, t, j);
            if(verbose) cout << j << "(" << slots[t*nPo + j] << ")" << "->" << score << " ";
            if(score > maxScore) {
                maxScore = score;
                maxIdx = j;
            }
        }
        if(verbose) cout << "\n";
        assert((maxScore >= 0.0) && (maxIdx > -1));

        if(maxIdx != i) {
            swap(slots[t*nPo + i], slots[t*nPo + maxIdx]);
            swap(sVecss[t][i], sVecss[t][maxIdx]);
        }
    }

    // update oPerm
    for(uint i=0; i<nPo*nTimeFrame; ++i) if(slots[i] != -1)
        oPerm[slots[i]] = i;
    assert(checkPerm(oPerm, nCo, nPo*nTimeFrame));

    if(verbose) {
        cout << "-slots:\n";
        for(uint t=0; t<nTimeFrame; ++t) {
            cout << "\t";
            for(uint i=0; i<nPo; ++i) cout << setw(4) << slots[t*nPo + i] << " ";
            cout << "\n";
        }
        cout << "-oPerm:\n\t";
        for(uint i=0; i<nCo; ++i) cout << setw(4) << oPerm[i] << " ";
        cout << "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    }

    if(logger) logger->log("reord PO");
}

// places PIs at correct positions and reduces BDD size
static void schedulePI(DdManager *dd, const SupVecs &sVecs, int *slots, cuint nTimeFrame, cuint nCi, cuint nPo, int *iPerm, TimeLogger *logger, const bool verbose, cuint expConfig)
{
    if(verbose)  cout << "schedule PI:\n-slots:\n";

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
        if(verbose) cout << "\t";
        for(uint i=0; i<nCi; ++i) if(sv3[i]) {
            //iPerm[i] = pos++;  // perm
            iPerm[pos++] = i;  // invperm
            if(verbose) cout << setw(4) << i << " ";
        }
        if(verbose) cout << "\n";

        sNums.push_back(pos - ppos);
    }

    // dummy PIs
    if(pos != nCi) {
        if(verbose) cout << "\t";
        for(uint i=0; i<nCi; ++i) if(!sv1[i]) {
            iPerm[pos++] = i;
            if(verbose) cout << setw(4) << i << " ";
        }
        if(verbose) cout << "(dummies)\n";
    }
    if(verbose) cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    if(logger) logger->log("schedule PI");

    // apply iPerm
    assert(checkPerm(iPerm, nCi, nCi) && (pos == nCi));
    assert(Cudd_ShuffleHeap(dd, iPerm));

    if((expConfig == 0) || (expConfig == 2)) {
        // reord to minimize #nodes
        pos = 0;
        for(uint& sz: sNums) if(sz) {
            bddUtils::bddReordRange(dd, pos, sz);
            
            // check if PI order is shifted within specified range
            for(uint i=pos, j=pos+sz; i<j; ++i) {
                cuint lev = cuddI(dd, iPerm[i]);
                assert((lev >= pos) && (lev < j));
            }

            pos += sz;
        }

        if(verbose) {
            cout << "reord PI:\n-inv iPerm:\n";
            cuint nPi = nCi / nTimeFrame;
            for(uint t=0; t<nTimeFrame; ++t) {
                cout << "\t";
                for(uint i=0; i<nPi; ++i) cout << setw(4) << dd->invperm[t*nPi +i] << " ";
                cout << "\n";
            }
            cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        }
        if(logger) logger->log("reord PI");
    }

    // update iPerm
    for(uint i=0; i<nCi; ++i)
        iPerm[i] = dd->perm[i];
    assert(checkPerm(iPerm, nCi, nCi));
}

uint reordIO(Abc_Ntk_t *pNtk, DdManager *dd, cuint nTimeFrame, int *iPerm, int *oPerm, TimeLogger *logger, const bool verbose, cuint expConfig)
{
    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nCo = Abc_NtkCoNum(pNtk);

    SupVecs sVecs;  sVecs.reserve(nCo);
    initSupVecs(pNtk, dd, sVecs, nCi, logger);
    
    // determine the lower bound of #pins
    uint nPo = (uint)ceil(float(nCo) / float(nTimeFrame));
    uint nPi = nCi / nTimeFrame;

    if((expConfig == 4) || (expConfig == 5)) {
        manualReord(dd, iPerm, oPerm, nPo, expConfig);
    } else {
        int *slots = schedulePO(sVecs, nTimeFrame, nCi, nCo, nPi, nPo, oPerm, logger, verbose);
        schedulePI(dd, sVecs, slots, nTimeFrame, nCi, nPo, iPerm, logger, verbose, expConfig);
        if((expConfig == 0) || (expConfig == 1))
            reordPO(dd, sVecs, slots, nTimeFrame, nCi, nCo, nPi, nPo, oPerm, logger, verbose);
        delete [] slots;
    }
    return nPo;
}

} // end namespace timeMux2
