#include "ext-folding/2-partition/cirType.h"

#include <iostream>
#include <vector>
#include <cassert>

namespace fmPart2 {

// class Net member functions
void Net::report() const
{
    cout << name_ << " :";
    for(unsigned i=0; i<cellList_.size(); ++i)
        cout << " " << cellList_[i]->getName();
    cout << endl;
}

void Net::initGroupCnt()
{
    groupCnt_[0] = groupCnt_[1] = 0;
    for(unsigned i=0; i<cellList_.size(); ++i) {
        int g = cellList_[i]->getGroup();
        assert(g != -1);
        groupCnt_[g] += 1;
    }
}

void Net::incGainForGroupSingle(int g, int inc, CellBucket** cellBucket)
{
    for(unsigned i=0; i<cellList_.size(); ++i) {
        Cell* cell = cellList_[i];
        if(cell->isFree() && cell->getGroup() == g) {
            cellBucket[g]->remove(cell);
            cell->incGain(inc);
            cellBucket[g]->insert(cell);
            break;
        }
    }
}

void Net::incGainForAll(int inc, CellBucket** cellBucket)
{
    for(unsigned i=0; i<cellList_.size(); ++i) {
        Cell* cell = cellList_[i];
        if(cell->isFree()) {
            int g = cell->getGroup();
            cellBucket[g]->remove(cell);
            cell->incGain(inc);
            cellBucket[g]->insert(cell);
        }
    }
}

// class Cell member functions
void Cell::report() const
{
    cout << name_ << " :";
    for(unsigned i=0; i<netList_.size(); ++i)
        cout << " " << netList_[i]->getName();
    cout << endl;
}

void Cell::initGain()
{
    gain_ = 0;
    for(unsigned i = 0; i < netList_.size(); ++i) {
        if(netList_[i]->getGroupCnt(group_) == 1) gain_ += 1;
        if(netList_[i]->getGroupCnt(group_ ^ 1) == 0) gain_ -= 1;
    }
}

void Cell::insertNext(Cell* cell)
{
    Cell* next = pNext_;
    pNext_ = cell;
    cell->pPrev_ = this;
    cell->pNext_ = next;
    next->pPrev_ = cell;
}

void Cell::removeNext()
{
    Cell* next = pNext_;
    pNext_ = next->pNext_;
    pNext_->pPrev_ = this;
}

// class CellBucket member functions
CellBucket::CellBucket(int offset): offset_(offset)
{
    bucket_.clear();
    bucket_.reserve(2*offset_+1);
    for(unsigned i=0; i<=2*offset_; ++i) {
        Cell* cell = new Cell("dummy");
        //Cell* cell = new Cell(UINT_MAX, "dummy");
        cell->pNext_ = cell->pPrev_ = cell;
        bucket_.push_back(cell);
    }
    maxIdx_ = -1;
}

CellBucket::~CellBucket()
{
    for(unsigned i=0; i<=2*offset_; ++i)
        delete bucket_[i];
}

void CellBucket::report() const
{
    for(int i=2*offset_; i>=0; --i) {
        cout << "[" << i - offset_ << "]";
        for(Cell* cell=bucket_[i]->pNext_; cell!=bucket_[i]; cell = cell->pNext_)
            cout << " " << cell->getName();
        cout << endl;
    }
}

void CellBucket::init()
{
    for(unsigned i=0; i<=2*offset_; ++i) {
        Cell* cell = bucket_[i];
        cell->pNext_ = cell->pPrev_ = cell;
    }
    maxIdx_ = -1;
}

void CellBucket::insert(Cell* cell)
{
    const int idx = cell->getGain() + offset_;
    bucket_[idx]->insertNext(cell);
    if(idx > maxIdx_) maxIdx_ = idx;
}

void CellBucket::remove(Cell* cell)
{
    //const int idx = cell->getGain() + offset_;
    cell->pPrev_->removeNext();
}

Cell* CellBucket::getCellWithMaxGain(int& idx)
{
    updateMaxIdx();
    idx = maxIdx_;
    if(maxIdx_ == -1) return 0;
    assert(bucket_[idx]->pNext_ != bucket_[idx]);
    return bucket_[idx]->pNext_;
}

void CellBucket::updateMaxIdx()
{
    while(maxIdx_ >= 0) {
        if(bucket_[maxIdx_]->pNext_ != bucket_[maxIdx_]) break;
        maxIdx_ -= 1;
    }
}

} // end namespace fmPart2
