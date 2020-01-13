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


inline vector<uint> parseInfo(const char *fileName, const string& head)
{
    vector<uint> ret;
    ifstream fp(fileName);
    string line;
    while(getline(fp, line)) if(line.substr(0, head.size()) == head) {
        line = line.substr(head.size(), line.size()-head.size());
        size_t pos = 0;
        while(pos != string::npos) {
            size_t _pos = line.find(" ", pos+1);
            string s = line.substr(pos+1, _pos-pos-1);
            if(s.empty()) break;
            uint p = atoi(s.c_str());
            ret.push_back(p);
            pos = _pos;
        }
        break;
    }
    return ret;
}

Abc_Ntk_t* readInfo(Abc_Ntk_t *pNtk, const char *splitInfo, const char *permInfo, int *iPerm, cuint nTimeFrame, const bool verbose)
{
    vector<uint> kids = parseInfo(splitInfo, "p0-i:");
    vector<uint> iPerm0 = parseInfo(permInfo, "#iPerm:");
    
    assert(kids.size() <= iPerm0.size());
    assert(iPerm0.size() % nTimeFrame == 0);
    for(uint i=kids.size(); i<iPerm0.size(); ++i) assert(iPerm0[i] == i);

    cuint nCi = Abc_NtkCiNum(pNtk);
    cuint nPi = nCi / nTimeFrame;
    cuint nPi0 = iPerm0.size() / nTimeFrame;
    vector<bool> bv(nCi, false);

    int *invPerm = new int[nCi];
    for(uint i=0; i<nCi; ++i) iPerm[i] = -1;
    for(uint i=0; i<kids.size(); ++i) {
        cuint nt0 = iPerm0[i] / nPi0, ni0 = iPerm0[i] % nPi0;
        uint newPos = nt0 * nPi + ni0;
        iPerm[kids[i]] = newPos;
        invPerm[newPos] = kids[i];
        bv[newPos] = true;
    }

    for(uint i=0, pos=0; i<nCi; ++i) if(iPerm[i] < 0) {
        while(bv[pos]) pos++;
        iPerm[i] = pos;
        invPerm[pos] = i;
        bv[pos] = true;
    }
    
    assert(timeMux2::checkPerm(iPerm, nCi, nCi));
    assert(timeMux2::checkPerm(invPerm, nCi, nCi));

    pNtk = aigUtils::aigPermCi(pNtk, invPerm, true);
    delete [] invPerm;

    if(verbose) {
        cout << "iPerm: ";
        for(uint i=0; i<nCi; ++i)
            cout << iPerm[i] << " ";
        cout << "\n";
    }

    return pNtk;
}


} // end namespace timeMux4