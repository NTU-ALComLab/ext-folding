#include "ext-folding/timeMux4/timeMux4.h"
#include "ext-folding/utils/supVec.h"

using namespace SupVech;

namespace timeMux4
{

void getStructSup(Abc_Ntk_t *pNtk, cuint idx, SupVec &sv)
{
    Vec_Int_t *vInt = Abc_NtkNodeSupportInt(pNtk, idx);
    int i, iPi;
    Vec_IntForEachEntry(vInt, iPi, i) {
        assert(iPi < sv.size());
        sv[iPi] = 1;
        //cout << iPi << " ";
    }
    //cout << endl;
}

Abc_Ntk_t* permPi(Abc_Ntk_t *pNtk, int *iPerm, const bool verbose)
{
    assert(iPerm);
    cuint nPo = Abc_NtkPoNum(pNtk);
    cuint nPi = Abc_NtkPiNum(pNtk);
    SupVecs svs; svs.reserve(nPo);

    // init SupVecs
    for(uint i=0; i<nPo; ++i) {
        SupVec sv(nPi);
        getStructSup(pNtk, i, sv);
        svs.push_back(sv);
    }

    // sort each PO by #1s in its support vector
    uint *sIdx = new uint[nPo];
    for(uint i=0; i<nPo; ++i) sIdx[i] = i;
    sort(sIdx, sIdx+nPo, SupVecCompFunc(svs));

    SupVec curr(nPi);
    uint cnt = 0;
    int *invPerm = new int[nPi];
    for(uint i=0; i<nPo; ++i) {
        SupVec prev(curr);
        curr |= svs[sIdx[i]];
        prev ^= curr;

        for(uint j=0; j<nPi; ++j) if(prev[j]) {
            iPerm[j] = cnt;
            invPerm[cnt++] = j;
        }
    }

    pNtk = aigUtils::aigPermCi(pNtk, invPerm, true);
    delete [] sIdx;
    delete [] invPerm;

    if(verbose) {
        cout << "iPerm: ";
        for(uint i=0; i<nPi; ++i)
            cout << iPerm[i] << " ";
        cout << "\n";
    }

    return pNtk;
}

} // end namespace timeMux4