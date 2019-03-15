#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>

using namespace std;

// bddUtils.cpp
void printBddManInfo(DdManager *dd);
void printBddNodeInfo(DdManager *dd, DdNode *pNode);
void showBdd(DdManager *dd, DdNode **pNodeVec, int nNode, string fileName);

DdNode** computeSign(DdManager *dd, const size_t &range);
DdNode* bddDot(DdManager *dd, DdNode **v1, DdNode **v2, const size_t& len);
void bddNotVec(DdNode **vec, const size_t& len);
//st__table* bddCut(DdManager *dd, DdNode *f, int lev);
void bddDerefVec(DdManager *dd, DdNode **v, const size_t& len);
void bddFreeVec(DdManager *dd, DdNode **v, const size_t& len);
st__table* createDummyState(DdManager *dd);
void bddFreeTable(DdManager *dd, st__table *tb);
