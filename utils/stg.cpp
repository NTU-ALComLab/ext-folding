#include "ext-folding/utils/utils.h"

namespace utils
{

// caution: G will be dereferenced
void STG::addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint i, cuint cCnt, cuint nCnt)
{
    string cstName = "S" + to_string(i) + "_" + to_string(cCnt);
    string nstName = (i == nTimeFrame-1) ? string("*") : ("S"+to_string(i+1)+"_"+to_string(nCnt));
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;
    DdNode *tmp1, *tmp2;

    uint vStart = i * nPi;
    uint vEnd = (i+1) * nPi;

    while(G != b0) {
        gen = Cudd_FirstCube(dd, G, &cube, &val);
        
        string trans(nPi, '-');

        // input bits
        for(uint m=0; m<Cudd_ReadSize(dd); ++m) {
            uint n = cuddI(dd, m);
            if((n>=vStart) && (n<vEnd) && (cube[m]!=2))
                trans[n-vStart] = cube[m] + '0';
                //trans += (cube[m]==2) ? string("-") : to_string(cube[m]);
            else
                cube[m] = 2;
        }
        
        // states
        trans += " " + cstName + " " + nstName + " ";
        
        // output bits
        tmp1 = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(tmp1);
        for(uint k=0; k<nPo; ++k) {
            if(oFuncs && oFuncs[k]) {
                tmp2 = Cudd_Cofactor(dd, oFuncs[k], tmp1);  Cudd_Ref(tmp2);
                assert(Cudd_Regular(tmp2) == b1);
                if(tmp2 == b0) trans += "0";
                else trans += "1";
                Cudd_RecursiveDeref(dd, tmp2);
            } else trans += "-";
        }

        //cout << trans << endl;
        transs.push_back(trans);
        
        tmp2 = Cudd_bddAnd(dd, G, Cudd_Not(tmp1));  Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(dd, tmp1);
        Cudd_RecursiveDeref(dd, G);
        G = tmp2;
        Cudd_GenFree(gen);
    }
    Cudd_RecursiveDeref(dd, G);
}

void STG::writePerm(int *perm, ostream& fp, bool isPi)
{
    cuint &n = isPi ? nCi : nCo;
    fp << "#" << (isPi ? "i" : "o") << "Perm: ";
    for(uint i=0; i<n; ++i) fp << perm[i] << " ";
    fp << "\n";
}

void STG::write(ostream& fp, int *iPerm, int *oPerm)
{
    if(iPerm) writePerm(iPerm, fp, true);
    if(oPerm) writePerm(oPerm, fp, false);

    fp << ".i " << nPi << "\n";
    fp << ".o " << nPo << "\n";
    fp << ".p " << transs.size() << "\n";
    fp << ".s " << nSts << "\n";
    fp << ".r " << "S0_0" << "\n";
    for(string trans: transs) fp << trans << "\n";
    fp << ".e\n";
    fp.flush();
}

} // end namespace utils
