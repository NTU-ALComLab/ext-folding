#include "ext-folding/2-partition/fm.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <queue>
#include <cmath>
#include <cassert>
#include <ctime>
#include <map>
#include <set>

extern "C"
{
Abc_Ntk_t * Abc_NtkDarCleanupAig( Abc_Ntk_t * pNtk, int fCleanupPis, int fCleanupPos, int fVerbose );
}

namespace fmPart2
{

class CmpNet {
public:
    bool operator () (const Net* x, const Net* y) const {
        return x->getCellList().size() < y->getCellList().size();
    }
};

CirMgr::~CirMgr()
{
    for(uint i=0; i<netList_.size(); ++i) delete netList_[i];
    netList_.clear();
    for(uint i=0; i<cellList_.size(); ++i) delete cellList_[i];
    cellList_.clear();
    if(cellBucket_[0]) { delete cellBucket_[0]; cellBucket_[0] = 0; }
    if(cellBucket_[1]) { delete cellBucket_[1]; cellBucket_[1] = 0; }
}

void CirMgr::report() const
{
    cout << "[Balance factor]" << endl;
    cout << balanceFactor_ << endl;
    cout << "[Net]" << endl;
    for(uint i = 0; i < netList_.size(); ++i) netList_[i]->report();
    cout << "[Cells]" << endl;
    for(uint i = 0; i < cellList_.size(); ++i) cellList_[i]->report();
}

void CirMgr::reportPartition() const
{
    // groups
    cout << "Group 1 :";
    for(uint i=0; i<cellList_.size(); ++i)
        if(cellList_[i]->getGroup() == 0)
            cout << " " << cellList_[i]->getName();
    cout << endl;
    cout << "Group 2 :";
    for(uint i=0; i<cellList_.size(); ++i)
        if(cellList_[i]->getGroup() == 1)
            cout << " " << cellList_[i]->getName();
    cout << endl;
    // group count
    cout << "Group count:" << endl;
    for(uint i=0; i<netList_.size(); ++i) {
        Net* net = netList_[i];
        cout << net->getName() << " " << net->getGroupCnt(0) << " " << net->getGroupCnt(1) << endl;
    }
    // cell gain
    cout << "Gain:" << endl;
    for(uint i=0; i<cellList_.size(); ++i) {
        Cell* cell = cellList_[i];
        cout << cell->getName() << " " << cell->getGain() << endl;
    }
    // partition cost
    cout << "Cost:" << endl;
    cout << cost_ << endl;
    // cell buckets
    cout << "Bucket 1:" << endl;
    cellBucket_[0]->report();
    cout << "Bucket 2:" << endl;
    cellBucket_[1]->report();
}

inline void connect(Net *n, Cell *c)
{
    n->addCell(c);
    c->addNet(n);
}

void CirMgr::readCircuit(Abc_Ntk_t *pNtk)
{
    pNtk_ = pNtk;
    Abc_Obj_t *pObj;  uint i;
    netList_.reserve(Abc_NtkPiNum(pNtk));
    Abc_NtkForEachPi(pNtk, pObj, i)
        netList_.push_back(new Net(Abc_ObjName(pObj)));
        //netList_.push_back(new Net(i, Abc_ObjName(pObj)));
    
    cellList_.reserve(Abc_NtkPoNum(pNtk));
    Abc_NtkForEachPo(pNtk, pObj, i) {
        Cell *cell = new Cell(Abc_ObjName(pObj));
        //Cell *cell = new Cell(i, Abc_ObjName(pObj));
        cellList_.push_back(cell);
        
        Vec_Int_t *vInt = Abc_NtkNodeSupportInt(pNtk, i);
        uint j, jPi;
        Vec_IntForEachEntry(vInt, jPi, j) {
            assert(jPi < netList_.size());
            connect(netList_[jPi], cell);
        }
        Vec_IntFree(vInt);
    }

    /*
    for(i=0; i< netList_.size(); ++i)
        cout << netList_[i]->getIdx() << " " << netList_[i]->getName() << endl;
    cout << "============================================\n";
    for(i=0; i< cellList_.size(); ++i)
        cout << cellList_[i]->getIdx() << " " << cellList_[i]->getName() << endl;
    */

    // set other parameters
    cuint maxPin = getMaxPin();
    cellBucket_[0] = new CellBucket(maxPin);
    cellBucket_[1] = new CellBucket(maxPin);

    size_[0] = round(cellList_.size() * balanceFactor_);
    size_[1] = cellList_.size() - size_[0];
}

/*
Abc_Ntk_t* CirMgr::prepNtk0() const
{
    char buf[100]; uint i;
    Abc_Obj_t *pObj, *pObjNew;  
    
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    sprintf(buf, "%s-p0", pNtk_->pName);
    pNtkNew->pName = Extra_UtilStrsav(buf);
    Abc_AigConst1(pNtk_)->pCopy = Abc_AigConst1(pNtkNew);

    for(i=0; i<netList_.size(); ++i) if(netList_[i]->getGroupCnt(0) > 0) {
        pObj = Abc_NtkPi(pNtk_, i);
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }

    Abc_AigForEachAnd(pNtk_, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtk_->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    for(i=0; i<cellList_.size(); ++i) if(cellList_[i]->getGroup() == 0) {
        pObj = Abc_NtkPo(pNtk_, i);
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        Abc_ObjAssignName(pObj, Abc_ObjName(pObj), NULL);
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));

    return pNtkNew;
}
*/

Abc_Ntk_t* CirMgr::prepNtk(cuint gid) const
{
    char buf[100]; uint i;
    Abc_Obj_t *pObj, *pObjNew;  
    
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    sprintf(buf, "%s-p1", pNtk_->pName);
    pNtkNew->pName = Extra_UtilStrsav(buf);
    Abc_AigConst1(pNtk_)->pCopy = Abc_AigConst1(pNtkNew);

    for(i=0; i<netList_.size(); ++i) {
        pObj = Abc_NtkPi(pNtk_, i);
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        pObj->pCopy = pObjNew;
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }

    Abc_AigForEachAnd(pNtk_, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    for(i=0; i<cellList_.size(); ++i) if(cellList_[i]->getBestGroup() == gid) {
        pObj = Abc_NtkPo(pNtk_, i);
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }

    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    assert(Abc_NtkCheck(pNtkNew));

    return pNtkNew;
}

inline vector<uint> rename(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2)
{
    vector<uint> ret;  ret.reserve(Abc_NtkPiNum(pNtk2));
    uint i, cnt = 0;
    Abc_Obj_t *pObj1, *pObj2;
    Abc_NtkForEachPi(pNtk1, pObj1, i) if(Abc_ObjFanoutNum(pObj1) > 0) {
        ret.push_back(i);
        pObj2 = Abc_NtkPi(pNtk2, cnt++);
        Nm_ManDeleteIdName(pNtk2->pManName, pObj2->Id);
        Abc_ObjAssignName(pObj2, Abc_ObjName(pObj1), NULL);
    }

    assert(Abc_NtkPiNum(pNtk2) == cnt);
    return ret;
}

void CirMgr::writeSolution(const char* prefix) const
{
    Abc_Ntk_t *pNtk0 = prepNtk(0);
    Abc_Ntk_t *_pNtk0 = Abc_NtkDarCleanupAig(pNtk0, 1, 0, 0);
    vector<uint> kids = rename(pNtk0, _pNtk0);

    Abc_Ntk_t *pNtk1 = prepNtk(1);

    /*
    uint i, cnt = 0, cnt2 = 0;
    Abc_Obj_t *pObj;
    for(i=0; i<netList_.size(); ++i) if(netList_[i]->getBestGroupCnt(0) > 0) cnt++;
    Abc_NtkForEachPi(pNtk0, pObj, i) if(Abc_ObjFanoutNum(pObj) > 0) cnt2++;
    cout << cnt << " / " << cnt2 << " / " << Abc_NtkPiNum(_pNtk0);
    */

    char buf[100];
    sprintf(buf, "%s-p0.blif", prefix);
    Io_Write(_pNtk0, buf, IO_FILE_BLIF);
    sprintf(buf, "%s-p1.blif", prefix);
    Io_Write(pNtk1, buf, IO_FILE_BLIF);

    sprintf(buf, "%s-part.info", prefix);
    ofstream fp(buf);
    for(uint j=0; j<2; ++j) {
        fp << "p" << j << ":";
        for(uint i=0; i<cellList_.size(); ++i) if(cellList_[i]->getBestGroup() == j) fp << " " << i;
        fp << "\n";
    }
    fp << "p0i:";
    for(cuint& i: kids) fp << " " << i;
    fp << "\n";

    Abc_NtkDelete(pNtk0);
    Abc_NtkDelete(_pNtk0);
    Abc_NtkDelete(pNtk1);
}


// Main partition function
void CirMgr::partition()
{
    clock_t st = clock(), ed;
    uint fail = 0;
    // initialize
    genInitialPartition();
    initByCurrentPartition();
    saveSolution();
    for(uint iteration=1; iteration<=maxIteration_; ++iteration) {
        if(!FM_partition()) {
            // do not become better, random switch and increment fail counter
            randomSwap();
            fail += 1;
        } else if(cost_ < globalBestCost_) {
            // better than global best, update global best and reset fail counter
            saveSolution();
            fail = 0;
        }
        // report current status
        cout << "\rIteration " << iteration << ", Cut size = " << cost_
             << ", Best = " << globalBestCost_ << "     " << flush;
        // exceed fail counter limit or time limit
        ed = clock();
        if(fail > failLimit_ || ed - st > timeLimit_ * CLOCKS_PER_SEC) break;
    }
    cout << endl;
}

// FM heuristic, return true if result is better than initial partition
bool CirMgr::FM_partition()
{
    vector<Cell*> trace;
    trace.reserve(cellList_.size());
    // initialize
    initByCurrentPartition();
    // move cells
    int bestCost = cost_, bestPos = -1;
    for(int p=0; ; ++p) {
        if(verbose_) reportPartition();
        // move cell
        Cell* target = getTargetCell();
        if(target == 0) break;
        assert(cost_ >= target->getGain());
        cost_ -= target->getGain();
        moveCell(target);
        trace.push_back(target);
        //if((cost_ < bestCost) && (groupSize_[0] >= minSize_) && (groupSize_[1] >= minSize_)) {
        if((cost_ < bestCost) && (groupSize_[0] == size_[0])) {
            bestCost = cost_;
            bestPos = p;
        }
    }
    // restore best solution
    for(int i=trace.size()-1; i>bestPos; --i) {
        int g = trace[i]->getGroup();
        trace[i]->setGroup(g ^ 1);
        groupSize_[g] -= 1;
        groupSize_[g ^ 1] += 1;
    }
    // better than initial partition
    const bool better = (bestCost < cost_);
    cost_ = bestCost;
    return better;
}

// Get maximum pin number
uint CirMgr::getMaxPin() const
{
    uint maxPin = 0;
    for(uint i=0; i<cellList_.size(); ++i) {
        uint nPin = cellList_[i]->getNetList().size();
        maxPin = max(maxPin, nPin);
    }
    return maxPin;
}

// Generate initial partition
void CirMgr::genInitialPartition()
{
    cuint n = cellList_.size();
    priority_queue<Net*, vector<Net*>, CmpNet> heap;
    groupSize_[0] = groupSize_[1] = 0;
    for(uint i=0; i<n; ++i)
        cellList_[i]->setGroup(-1);
    for(uint i=0; i<netList_.size(); ++i) {
        netList_[i]->setFlag(false);
        heap.push(netList_[i]);
    }
    //for(uint i=0; (i<n) && (groupSize_[0]<n/2); ++i) {
    for(uint i=0; (i<n) && (groupSize_[0]<size_[0]); ++i) {
        if(netList_[i]->getFlag()) continue;
        while(!heap.empty()) heap.pop();
        heap.push(netList_[i]);
        netList_[i]->setFlag(true);
        //while(!heap.empty() && groupSize_[0] < n/2) {
        while(!heap.empty() && (groupSize_[0]<size_[0])) {
            Net* net = heap.top();
            heap.pop();
            assert(net->getFlag());
            const vector<Cell*>& cList = net->getCellList();
            //for(uint j=0; (j<cList.size()) && (groupSize_[0]<n/2); ++j) {
            for(uint j=0; (j<cList.size()) && (groupSize_[0]<size_[0]); ++j) {
                Cell* cell = cList[j];
                if(cell->getGroup() != -1) continue;
                cell->setGroup(0);
                groupSize_[0] += 1;
                const vector<Net*>& nList = cell->getNetList();
                for(uint k=0; k<nList.size(); ++k) {
                    if(!nList[k]->getFlag()) {
                        nList[k]->setFlag(true);
                        heap.push(nList[k]);
                    }
                }
            }
        }
    }
    //assert(groupSize_[0] == n/2);
    assert(groupSize_[0] == size_[0]);
    for(uint i=0; i<cellList_.size(); ++i) {
        if(cellList_[i]->getGroup() == -1) {
            cellList_[i]->setGroup(1);
            groupSize_[1] += 1;
        }
    }
}

// save current partition
void CirMgr::saveSolution()
{
    globalBestCost_ = cost_;
    for(uint i=0; i<cellList_.size(); ++i)
        cellList_[i]->saveBest();
    for(uint i=0; i<netList_.size(); ++i)
        netList_[i]->saveBest();
    /*
    globalBestGroup_[0].clear();
    globalBestGroup_[1].clear();
    for(uint i=0; i<cellList_.size(); ++i) {
        Cell* cell = cellList_[i];
        globalBestGroup_[cell->getGroup()].push_back(cell);
    }
    */
}

// random swap groups
void CirMgr::randomSwap()
{
    cuint n = cellList_.size();
    cuint nSwap = n / 10;
    for(uint i=0; i<nSwap; ++i) {
        uint id0 = rand() % n, id1 = rand() % n;
        int group0 = cellList_[id0]->getGroup();
        int group1 = cellList_[id1]->getGroup();
        assert(group0 != -1 && group1 != -1);
        if(group0 != group1) {
        cellList_[id0]->setGroup(group1);
        cellList_[id1]->setGroup(group0);
        }
    }
}

// Set parameters using curent partition
void CirMgr::initByCurrentPartition()
{
    // initialize group count for each net
    for(uint i=0; i<netList_.size(); ++i) {
        netList_[i]->initGroupCnt();
    }
    // initialize gain for each cell
    for(uint i=0; i<cellList_.size(); ++i) {
        cellList_[i]->setFree(true);
        cellList_[i]->initGain();
    }
    // initilize cell bucket
    cellBucket_[0]->init();
    cellBucket_[1]->init();
    for(uint i=0; i<cellList_.size(); ++i) {
        int g = cellList_[i]->getGroup();
        cellBucket_[g]->insert(cellList_[i]);
    }
    // get initial cost
    cost_ = 0;
    for(uint i=0; i<netList_.size(); ++i)
        if((netList_[i]->getGroupCnt(0) != 0) && (netList_[i]->getGroupCnt(1) != 0))
            cost_ += 1;
}

Cell* CirMgr::getTargetCell() const
{
    int gain0, gain1;
    Cell* cell0 = cellBucket_[0]->getCellWithMaxGain(gain0);
    Cell* cell1 = cellBucket_[1]->getCellWithMaxGain(gain1);
    if((cell0 == 0) && (cell1 == 0)) return 0;
    else if(cell0 == 0) return cell1;
    else if(cell1 == 0) return cell0;
    //else if(groupSize_[0] <= minSize_) return cell1;
    //else if(groupSize_[1] <= minSize_) return cell0;
    else if(groupSize_[0] < size_[0]) return cell1;
    else if(groupSize_[1] < size_[1]) return cell0;
    else return (gain0 >= gain1) ? cell0 : cell1;
}

void CirMgr::moveCell(Cell* cell)
{
    assert(cell->isFree());
    const int F = cell->getGroup();
    const int T = F ^ 1;
    cell->setGroup(T);
    cell->setFree(false);
    cellBucket_[F]->remove(cell);
    groupSize_[F] -= 1;
    groupSize_[T] += 1;
    const vector<Net*>& nList = cell->getNetList();
    for(uint i=0; i<nList.size(); ++i) {
        Net* net = nList[i];
        if(net->getGroupCnt(T) == 0) {
            net->incGainForAll(1, cellBucket_);
        } else if(net->getGroupCnt(T) == 1) {
            net->incGainForGroupSingle(T, -1, cellBucket_);
        }
        net->incGroupCnt(F, -1);
        net->incGroupCnt(T,  1);
        if(net->getGroupCnt(F) == 0) {
            net->incGainForAll(-1, cellBucket_);
        } else if(net->getGroupCnt(F) == 1) {
            net->incGainForGroupSingle(F, 1, cellBucket_);
        }
    }
}

    
} // end namespace fmPart2