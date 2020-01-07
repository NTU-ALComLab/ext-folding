#include "ext-folding/timeMux4/timeMux4.h"
#include "ext-folding/utils/supVec.h"
#include "ext-folding/timeMux2/timeMux2.h"

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
    Vec_IntFree(vInt);
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
            //cout << j << " -> " << cnt << endl;
            iPerm[j] = cnt;
            invPerm[cnt++] = j;
        }
    }

    // deal with dummy inputs
    for(uint j=0; j<nPi; ++j) if(!curr[j]) {
        //cout << j << " -> " << cnt << endl;
        iPerm[j] = cnt;
        invPerm[cnt++] = j;
    }

    assert(timeMux2::checkPerm(iPerm, nPi, nPi));
    assert(timeMux2::checkPerm(invPerm, nPi, nPi));

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

/*
inline vector<uint> parseInfo(const char *fileName, const string& head, cuint size)
{
    vector<uint> ret;  ret.reserve(size);
    ifstream fp(fileName);
    string line;
    while(getline(fp, line)) if(line.substr(0, head.size*()) == head) {
        line = line.substr(head.size(), line.size()-head.size());
        size_t pos = 0;
        while(pos != string::npos) {

        }

        break;
    }
    assert(ret.size() == size);
    return ret;
}

void readInfo(Abc_Ntk_t *pNtk, const char *splitInfo, const char *permInfo, int* &iPerm)
{
    iPerm = NULL;

}
*/

} // end namespace timeMux4