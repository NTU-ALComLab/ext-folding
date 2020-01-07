#ifndef __CIRTYPE_H__
#define __CIRTYPE_H__

#include <string>
#include <vector>
#include <climits>

using namespace std;

namespace fmPart2 {

// Forward declaration
class Net;
class Cell;
class CellBucket;

// Class Net Definition
class Net {
public:
    // Constructor / Destructor
    Net(const string& name): name_(name) {}
    //Net(const unsigned idx, const string& name): idx_(idx), name_(name) {}
    ~Net() {}

    // functions for getting values
    //const unsigned&         getIdx()                    const { return idx_; }
    const string&           getName()                   const { return name_; }
    const vector<Cell*>&    getCellList()               const { return cellList_; }
    unsigned                getGroupCnt(int id)         const { return groupCnt_[id]; }
    unsigned                getBestGroupCnt(int id)     const { return bestGroupCnt_[id]; }
    bool                    getFlag()                   const { return flag_; }
    void                    report()                    const;

    // functions for setting values
    void addCell(Cell* cell) { cellList_.push_back(cell); }
    void setFlag(bool f) { flag_ = f; }
    void initGroupCnt();
    void incGroupCnt(int id, int inc) { groupCnt_[id] += inc; }
    void incGainForGroupSingle(int id, int inc, CellBucket** cellBucket);
    void incGainForAll(int inc, CellBucket** cellBucket);
    void saveBest() { for(uint i=0; i<2; ++i) bestGroupCnt_[i] = groupCnt_[i]; }

private:
    //unsigned        idx_;
    string          name_;
    vector<Cell*>   cellList_;
    unsigned        groupCnt_[2], bestGroupCnt_[2];
    bool            flag_;
};

// Class Cell Definition
class Cell {
public:
    // Constructor / Destructor
    Cell(const string& name): name_(name), group_(-1), bestGroup_(-1), free_(true) {}
    //Cell(const unsigned idx, const string& name): idx_(idx), name_(name), group_(-1), bestGroup_(-1), free_(true) {}
    ~Cell() {}

    // functions for getting values
    //const unsigned&         getIdx()        const { return idx_; }
    const string&           getName()       const { return name_; }
    const vector<Net*>&     getNetList()    const { return netList_; }
    int                     getGroup()      const { return group_; }
    int                     getBestGroup()  const { return bestGroup_; }
    int                     getGain()       const { return gain_; }
    bool                    isFree()        const { return free_; }
    void                    report()        const;

    // functions for setting values
    void addNet(Net* net) { netList_.push_back(net); }
    void setGroup(int g) { group_ = g; }
    void setFree(bool f) { free_ = f; }
    void initGain();
    void incGain(int inc) { gain_ += inc; }
    void saveBest() { bestGroup_ = group_; }

    // functions about linked list
    void insertNext(Cell* cell);
    void removeNext();

    // Public variable, used for  linked list manipulation
    Cell* pNext_;
    Cell* pPrev_;

private:
    //unsigned        idx_;
    string          name_;
    vector<Net*>    netList_;
    int             group_, bestGroup_;
    int             gain_;
    bool            free_;
};

// Class CellBucket definition
class CellBucket {
public:
    // Constructor / Destructor
    CellBucket(int offset);
    ~CellBucket();

    // functions for insert / remove
    void report() const;
    void init();
    void insert(Cell* cell);
    void remove(Cell* cell);
    Cell* getCellWithMaxGain(int& idx);

private:
    void updateMaxIdx();

    int             offset_;
    vector<Cell*>   bucket_;
    int             maxIdx_;
};

} // end namespace fmPart2

#endif /* __CIRTYPE_H__ */
