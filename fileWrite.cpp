#include "ext-folding/utils.h"

namespace fileWrite
{
    
void writeKiss(cuint nPi, cuint nPo, cuint nSts, const vector<string>& stg, ostream& fp)
{
    fp << ".i " << nPi << "\n";
    fp << ".o " << nPo << "\n";
    fp << ".p " << stg.size() << "\n";
    fp << ".s " << nSts << "\n";
    fp << ".r " << "S0_0" << "\n";
    for(string trans: stg) fp << trans << "\n";
    fp << ".e\n";
}

void addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, cuint cCnt, cuint nCnt, vector<string> stg)
{
    string cstName = "S" + to_string(i) + "_" + to_string(cCnt);
    string nstName = (i == nTimeFrame-1) ? string("*") : ("S"+to_string(i+1)+"_"+to_string(nCnt));
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;
    DdNode *tmp1, *tmp2;

    while(G != b0) {
        gen = Cudd_FirstCube(dd, G, &cube, &val);
        
        string trans("");
            
        // input bits
        size_t vStart = i * nPi;
        size_t vEnd = (i+1) * nPi;
        for(size_t m=0; m<Cudd_ReadSize(dd); ++m) {
            if((m>=vStart) && (m<vEnd))
                trans += (cube[m]==2) ? string("-") : to_string(cube[m]);
            else
                cube[m] = 2;
        }
        
        // states
        trans += " " + cstName + " " + nstName + " ";
        
        // output bits
        tmp1 = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(tmp1);
        for(size_t k=0; k<nPo; ++k) {
            tmp2 = Cudd_Cofactor(dd, oFuncs[k], tmp1);  Cudd_Ref(tmp2);
            if(tmp2 == b0) trans += "0";
            else trans += "1";
            Cudd_RecursiveDeref(dd, tmp2);
        }
        
        stg.push_back(trans);
        
        tmp2 = Cudd_bddAnd(dd, G, Cudd_Not(tmp1));  Cudd_Ref(tmp2);
        
        Cudd_RecursiveDeref(dd, tmp1);
        Cudd_RecursiveDeref(dd, G);
        
        G = tmp2;
        
        Cudd_GenFree(gen);
    }
}

} // end namespace fileWrite

    