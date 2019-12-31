#include "ext-folding/utils/utils.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cassert>
#include <unordered_map>


using namespace std;

namespace kissToBlif
{

//typedef unordered_map<string, unsigned> StateMap;
extern void tokenize(string x, vector<string>& toks, const unsigned& size);
extern unsigned getStateIdx(const string &name, unordered_map<string, unsigned> &stateMap);

int KissHot_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    bool enc = true, replace = false, dcVal = false;
    char *inKiss, *outBlif;

    inKiss = argv[globalUtilOptind++];
    //outBlif = argv[globalUtilOptind++];


    ifstream ifs(inKiss);
    if(!ifs.is_open()) {
        cerr << "File \"" << inKiss << "\" not found.\n";
        return 0;
    }
    
    string buf;
    vector<string> toks;
    
    unsigned ni, no, np, ns, rst;
    ni = no = np = ns = rst = 0;
    unordered_map<string, unsigned> stateMap;

    while(1) {
        getline(ifs, buf);
        if(buf[0] != '#') break;
    }

    tokenize(buf, toks, 2);
    assert(toks[0] == ".i");
    ni = stoi(toks[1]);
    
    getline(ifs, buf);  tokenize(buf, toks, 2);
    assert(toks[0] == ".o");
    no = stoi(toks[1]);
    
    getline(ifs, buf);  tokenize(buf, toks, 2);
    assert(toks[0] == ".p");
    np = stoi(toks[1]);
    
    getline(ifs, buf);  tokenize(buf, toks, 2);
    assert(toks[0] == ".s");
    ns = stoi(toks[1]);

    getline(ifs, buf);  tokenize(buf, toks, 2);
    assert(toks[0] == ".r");
    rst = getStateIdx(toks[1], stateMap);  //stoi(toks[1]);
    
    vector<vector<unsigned> > ocVec, ncVec;
    vector<vector<string> > oiVec, niVec;
    ocVec.reserve(no);  ncVec.reserve(ns);
    oiVec.reserve(no);  niVec.reserve(ns);
    for(unsigned i=0; i<no; ++i) {
        ocVec.push_back(vector<unsigned>());
        oiVec.push_back(vector<string>());
    }
    for(unsigned i=0; i<ns; ++i) {
        ncVec.push_back(vector<unsigned>());
        niVec.push_back(vector<string>());
    }
    
    for(unsigned i=0; i<np; ++i) {
        getline(ifs, buf);  tokenize(buf, toks, 4);
        assert(toks[0].size() == ni && toks[3].size() == no);
        //toks[1].erase(0, 1);  toks[2].erase(0, 1);
        //unsigned x = stoi(toks[1]), y = stoi(toks[2]);
        unsigned x = getStateIdx(toks[1], stateMap);
        unsigned y = getStateIdx(toks[2], stateMap);
        ncVec[y].push_back(x);
        niVec[y].push_back(toks[0]);
        for(unsigned j=0; j<no; ++j) 
            if((toks[3][j] == '1') || (dcVal && (toks[3][j] == '-'))) {
                ocVec[j].push_back(x);
                oiVec[j].push_back(toks[0]);
            }
    }
    
    getline(ifs, buf);  assert(buf[0] == '.');
    ifs.close();
    
    vector<string> sMap;  sMap.reserve(ns);
    for(unsigned i=0; i<ns; ++i) {
        sMap.push_back(string(ns, '0'));
        sMap[i][i] = '1';
    }
    
    Abc_Ntk_t *pNtk = utils::aigUtils::aigInitNtk(ni, no, ns, inKiss);
    //cout << ni << " " << no << " " << ns << endl;

    for(unsigned i=0; i<no; ++i) {
        //cout << "o" << i << endl;
        Abc_Obj_t *pPo = Abc_NtkPo(pNtk, i);
        unsigned n = oiVec[i].size();
        assert(n == ocVec[i].size());

        Abc_Obj_t *pObj = Abc_ObjNot(Abc_AigConst1(pNtk));

        if(!n) {
            cout << "here1!!" << endl;
            Abc_ObjAddFanin(pPo, pObj);
            continue;
        }
        //ofs << head << "o" << i << "\n";

        for(unsigned j=0; j<n; ++j) {
            Abc_Obj_t *pp = Abc_AigConst1(pNtk);
            //ofs << oiVec[i][j] << sMap[ocVec[i][j]] << " 1\n";
            for(uint k=0; k<oiVec[i][j].size(); ++k) {
                if(oiVec[i][j][k] == '0')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_NtkPi(pNtk, k)), pp);
                else if(oiVec[i][j][k] == '1')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_NtkPi(pNtk, k), pp);
            }

            for(uint k=0; k<sMap[ocVec[i][j]].size(); ++k) {
                if(sMap[ocVec[i][j]][k] == '0')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_ObjFanout0(Abc_NtkBox(pNtk, k))), pp);
                else if(sMap[ocVec[i][j]][k] == '1')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjFanout0(Abc_NtkBox(pNtk, k)), pp);
            }

            pObj = Abc_AigOr((Abc_Aig_t*)pNtk->pManFunc, pObj, pp);
        }
        Abc_ObjAddFanin(pPo, pObj);
    }

    for(unsigned i=0; i<ns; ++i) {
        //cout << "s" << i << endl;
        Abc_Obj_t *pPo = Abc_ObjFanin0(Abc_NtkBox(pNtk, i));
        unsigned n = niVec[i].size();
        assert(n == ncVec[i].size());

        Abc_Obj_t *pObj = Abc_ObjNot(Abc_AigConst1(pNtk));

        if(!n) {
            cout << "here2!!" << endl;
            Abc_ObjAddFanin(pPo, pObj);
            continue;
        }

        //ofs << head << "n" << i << "\n";
        for(unsigned j=0; j<n; ++j) {
            //ofs << niVec[i][j] << sMap[ncVec[i][j]] << " 1\n";
            Abc_Obj_t *pp = Abc_AigConst1(pNtk);
            for(uint k=0; k<niVec[i][j].size(); ++k) {
                if(niVec[i][j][k] == '0')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_NtkPi(pNtk, k)), pp);
                else if(niVec[i][j][k] == '1')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_NtkPi(pNtk, k), pp);
            }

            for(uint k=0; k<sMap[ncVec[i][j]].size(); ++k) {
                if(sMap[ncVec[i][j]][k] == '0')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_ObjFanout0(Abc_NtkBox(pNtk, k))), pp);
                else if(sMap[ncVec[i][j]][k] == '1')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjFanout0(Abc_NtkBox(pNtk, k)), pp);
            }

            pObj = Abc_AigOr((Abc_Aig_t*)pNtk->pManFunc, pObj, pp);
        }
        Abc_ObjAddFanin(pPo, pObj);
    }

    assert(Abc_NtkCheck(pNtk));
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);

    
    return 0;

}

// called during ABC startup
void init1(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "read_kiss1", KissHot_Command, 0);
}

// called during ABC termination
void destroy1(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer1 = { init1, destroy1 };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer1);
    }
} kissToBlif1_registrar;

} // end namespace kissToBlif