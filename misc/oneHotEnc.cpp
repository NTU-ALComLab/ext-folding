#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace kissToBlif
{

typedef unordered_map<string, unsigned> StateMap;

void tokenize(string x, vector<string>& toks, const unsigned& size)
{
    toks.clear();
    boost::erase_all(x, "\n");  boost::erase_all(x, "\r");
    boost::split(toks, x, boost::is_any_of(" \t"));
    toks.erase(remove_if(toks.begin(), toks.end(), [](const string& s){ return s.empty(); }), toks.end());
    assert(toks.size() == size);
}

unsigned getStateIdx(const string &name, StateMap &stateMap)
{
    if(stateMap.count(name) > 0)
        return stateMap[name];
    unsigned ret = stateMap.size();
    stateMap[name] = ret;
    return ret;
}

/* string toBinStr(const unsigned& x, const unsigned& n)
{
    string ret = "";
    unsigned tmp = x;
    for(unsigned i=0; i<n; ++i) {
        ret += to_string(tmp & 1);
        tmp >>= 1;
    }
    assert(!tmp && ret.size() == n);
    return ret;
} */

// usage: ./kiss1hot <input_kiss> <output_blif>
void oneHotEnc(const char *inKiss, const char *outBlif, const bool dcVal)
{
    ifstream ifs(inKiss);
    if(!ifs.is_open()) {
        cerr << "File \"" << inKiss << "\" not found.\n";
        return;
    }
    
    string buf;
    vector<string> toks;
    
    unsigned ni, no, np, ns, rst;
    ni = no = np = ns = rst = 0;
    StateMap stateMap;

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
    
    ofstream ofs(outBlif);
    if (!ofs.is_open()) {
        cerr << "cannot open file \"" << outBlif << "\"!!" << endl;
        return;
    }
    ofs << ".model " << inKiss << ".blif\n";
    ofs << ".inputs ";
    for(unsigned i=0; i<ni; ++i)
        ofs << "i" << i << ((i==ni-1) ? '\n' : ' ');
    ofs << ".outputs ";
    for(unsigned i=0; i<no; ++i)
        ofs << "o" << i << ((i==no-1) ? '\n' : ' ');
    for(unsigned i=0; i<ns; ++i)
        ofs << ".latch n" << i << " c" << i << " " << sMap[rst][i] << "\n";
    
    string head = "";
    head += ".names ";
    for(unsigned i=0; i<ni; ++i)
        head += "i" + to_string(i) + " ";
    for(unsigned i=0; i<ns; ++i)
        head += "c" + to_string(i) + " ";
    
    for(unsigned i=0; i<no; ++i) {
        unsigned n = oiVec[i].size();
        assert(n == ocVec[i].size());
        if(!n) continue;
        ofs << head << "o" << i << "\n";
        for(unsigned j=0; j<n; ++j)
            ofs << oiVec[i][j] << sMap[ocVec[i][j]] << " 1\n";
    }
    
    for(unsigned i=0; i<ns; ++i) {
        unsigned n = niVec[i].size();
        assert(n == ncVec[i].size());
        if(!n) continue;
        ofs << head << "n" << i << "\n";
        for(unsigned j=0; j<n; ++j)
            ofs << niVec[i][j] << sMap[ncVec[i][j]] << " 1\n";
    }
    
    ofs << ".end\n";
    ofs.close();
    
}

} // end namespace kissToBlif