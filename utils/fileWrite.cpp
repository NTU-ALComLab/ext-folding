#include "ext-folding/utils/utils.h"

namespace utils::fileWrite
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
    fp.flush();
}

// caution: G will be dereferenced
void addOneTrans(DdManager *dd, DdNode *G, DdNode **oFuncs, cuint nPi, cuint nPo, cuint nTimeFrame, cuint i, cuint cCnt, cuint nCnt, vector<string>& stg)
{
    string cstName = "S" + to_string(i) + "_" + to_string(cCnt);
    string nstName = (i == nTimeFrame-1) ? string("*") : ("S"+to_string(i+1)+"_"+to_string(nCnt));
    int *cube;  CUDD_VALUE_TYPE val;  DdGen *gen;
    DdNode *tmp1, *tmp2;

    size_t vStart = i * nPi;
    size_t vEnd = (i+1) * nPi;

    while(G != b0) {
        gen = Cudd_FirstCube(dd, G, &cube, &val);
        
        string trans(nPi, '-');

        /*
        cout << "\ncube: ";
        for(size_t m=0; m<Cudd_ReadSize(dd); ++m)
            cout << cube[m];
        cout << endl;

        cout << "pube: ";
        for(size_t m=0; m<Cudd_ReadSize(dd); ++m)
            cout << cube[dd->invperm[m]];
        cout << endl;
        */
        
        //tmp1 = Cudd_CubeArrayToBdd(dd, cube);  Cudd_Ref(tmp1);
        //bddUtils::showBdd(dd, &tmp1, 1, "yolo");

        //cout << "(" << vStart << ", " << vEnd << ")" << endl;


        // input bits
        for(size_t m=0; m<Cudd_ReadSize(dd); ++m) {
            size_t n = cuddI(dd, m);
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

        for(size_t k=0; k<nPo; ++k) {
            if(oFuncs) {
                tmp2 = Cudd_Cofactor(dd, oFuncs[k], tmp1);  Cudd_Ref(tmp2);
                assert(Cudd_Regular(tmp2) == b1);
                if(tmp2 == b0) trans += "0";
                else trans += "1";
                Cudd_RecursiveDeref(dd, tmp2);
            } else trans += "-";
        }

        stg.push_back(trans);
        
        tmp2 = Cudd_bddAnd(dd, G, Cudd_Not(tmp1));  Cudd_Ref(tmp2);
        
        Cudd_RecursiveDeref(dd, tmp1);
        Cudd_RecursiveDeref(dd, G);
        
        G = tmp2;
        
        Cudd_GenFree(gen);
    }
    Cudd_RecursiveDeref(dd, G);
}

} // end namespace utils::fileWrite

    