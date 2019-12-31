#ifndef __SUPVEC_H__
#define __SUPVEC_H__

#undef b1
#include <algorithm>
#include <boost/dynamic_bitset.hpp>

namespace SupVech {

typedef boost::dynamic_bitset<> SupVec;
typedef vector<SupVec> SupVecs;

// comparison function for SupVec
class SupVecCompFunc
{
  public:
    SupVecCompFunc(const SupVecs& svs): sVecs(svs) {}
    bool operator() (cuint x, cuint y) {
        return (sVecs[x].count() < sVecs[y].count());
    }
  private:
    const SupVecs &sVecs;
};

} // end namespace SupVech

#define b1 (dd)->one

#endif /* __SUPVEC_H__ */