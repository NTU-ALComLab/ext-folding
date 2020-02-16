#include "ext-folding/n-split/split.h"
#include "ext-folding/timeMux3/timeMux3.h"

using namespace nSplit;

namespace nSplit
{

static inline vector<Abc_Ntk_t*> readNtks(const InfoPairs &infos)
{
    vector<Abc_Ntk_t*> ret;  ret.reserve(infos.size() + 1);
    for(uint i=0; i<infos.size(); ++i)
        ret.push_back(aigUtils::aigReadFromFile(infos[i].first));
    return ret;
}

static inline vector<uint> parsePerm(const string &fileName, const string &head)
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

static inline vector<vector<uint> > readPerms(const InfoPairs &infos)
{
    vector<vector<uint> > ret;  ret.reserve(infos.size());
    for(uint i=0; i<infos.size(); ++i)
        ret.push_back(parsePerm(infos[i].second, "#oPerm:"));
    return ret;
}

// append pNtk2 to pNtk1
static inline vector<Abc_Obj_t*> aigAppend(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2)
{
    vector<Abc_Obj_t*> ret;  ret.reserve(Abc_NtkPoNum(pNtk2));
    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtk2);

    Abc_Obj_t *pObj, *pObjNew;  uint i;
    Abc_NtkForEachPi(pNtk1, pObj, i)
        pObj->pCopy = Abc_NtkCreatePi(pNtk2);

    Abc_NtkForEachBox(pNtk1, pObj, i) {
        pObjNew = aigUtils::aigNewLatch(pNtk2, Abc_LatchInit(pObj));
        Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew)->pCopy;
    }

    Abc_AigForEachAnd(pNtk1, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtk2->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachPo(pNtk1, pObj, i)
        ret.push_back(Abc_ObjChild0Copy(pObj));

    Abc_NtkForEachBox(pNtk1, pObj, i) {
        pObjNew = Abc_NtkBox(pNtk2, i);
        Abc_ObjAddFanin(Abc_ObjFanin0(pObjNew), Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));
    }

    return ret;
}

Abc_Ntk_t* aigLink(cuint nTimeFrame, const InfoPairs &infos, const string &topNtk)
{
    Abc_Ntk_t *pNtkTop = aigUtils::aigReadFromFile(topNtk);
    Abc_Ntk_t *pNtkRes = aigUtils::aigInitNtk(0, 0, nTimeFrame, "linkNet");
    timeMux3::buildLatchTransWithTime(pNtkRes, nTimeFrame, NULL);

    vector<Abc_Ntk_t*> ntks = readNtks(infos);
    vector<vector<uint> > oPerms = readPerms(infos);

    for(uint i=0; i<ntks.size(); ++i) {
        Abc_Ntk_t *pNtk = ntks[i];
        vector<uint> &oPerm = oPerms[i];
        vector<Abc_Obj_t*> vObjs = aigAppend(pNtkRes, pNtk);

        for(uint j=0; j<oPerm.size(); ++j) {
            cuint nt = oPerm[j] / Abc_NtkPoNum(pNtk);
            cuint id = oPerm[j] % Abc_NtkPoNum(pNtk);

            if(nt < nTimeFrame - 1) {
                ;
            } else {
                ;
            }
        }
        
    }

    return pNtkRes;
}

} // end namespace nSplit