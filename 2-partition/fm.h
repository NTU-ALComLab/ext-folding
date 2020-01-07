#ifndef __FM_H__
#define __FM_H__

#include "ext-folding/utils/utils.h"
#include "ext-folding/2-partition/cirType.h"

using namespace utils;

namespace fmPart2 {

// Class CirMgr Definition
class CirMgr {
public:
    // Constructor / Destructor
    CirMgr(const double r, cuint tl, cuint fl, cuint il, const bool v): balanceFactor_(r), timeLimit_(tl), failLimit_(fl), maxIteration_(il), verbose_(v) {
        cellBucket_[0] = cellBucket_[1] = NULL;
    }

    ~CirMgr();

    // functions about I/O
    void report() const;
    void reportPartition() const;
    void partition();

    void readCircuit(Abc_Ntk_t *pNtk);
    void writeSolution(const char* prefix) const;

private:
    bool FM_partition();
    uint getMaxPin() const;
    void genInitialPartition();
    void initByCurrentPartition();
    void saveSolution();
    void randomSwap();
    Cell* getTargetCell() const;
    void moveCell(Cell* cell);
    //Abc_Ntk_t* prepNtk0() const;
    Abc_Ntk_t* prepNtk(cuint gid) const;

    double          balanceFactor_;       // partition balance factor
    uint            timeLimit_;           // time limit
    uint            failLimit_;           // fail limit
    uint            maxIteration_;        // maximum number of partition iterations
    vector<Net*>    netList_;             // list of nets
    vector<Cell*>   cellList_;            // list of cells
    CellBucket      *cellBucket_[2];      // list of cells with same gain
    int             cost_;                // current cut size
    //uint            minSize_;             // min size of a group
    uint            size_[2];             // target group size
    uint            groupSize_[2];        // current group size
    //vector<Cell*>   globalBestGroup_[2];  // global best partition
    uint            globalBestCost_;      // global best cut size
    bool            verbose_;
    Abc_Ntk_t       *pNtk_;
};


} // end namespace fmPart2

#endif /* __FM_H__ */