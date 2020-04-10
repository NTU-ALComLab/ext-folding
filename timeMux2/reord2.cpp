#include "ext-folding/timeMux2/timeMux2.h"

namespace timeMux2
{

static inline void initIPerm(DdManager *dd, int *iPerm, cuint n)
{
    for(uint i=0; i<n; ++i) iPerm[i] = i;
    assert(Cudd_ShuffleHeap(dd, iPerm));
}

static inline void reord567(DdManager *dd, int *iPerm, int *oPerm, uint &nPo)
{
    nPo = 3;  // sum, carry and parity
    initIPerm(dd, iPerm, 68);

    for(uint i=0; i<34; ++i) oPerm[i] = nPo * i;
    oPerm[34] = nPo * 8 + 2;
    oPerm[35] = nPo * 17 + 2;
    oPerm[36] = nPo * 26 + 2;
    oPerm[37] = nPo * 33 + 2;
    oPerm[38] = nPo * 33 + 1;
}

static inline void reord8(DdManager *dd, int *iPerm, int *oPerm, uint &nPo)
{
    nPo = 1;  // carry
    initIPerm(dd, iPerm, 68);
    oPerm[0] = 17;
    oPerm[1] = 33;
}

static inline void reord9(DdManager *dd, int *iPerm, int *oPerm, uint &nPo)
{
    nPo = 1;
    initIPerm(dd, iPerm, 42);
    oPerm[0] = 1;
}

void manualReord(DdManager *dd, int *iPerm, int *oPerm, uint &nPo, cuint expConfig)
{
    if(expConfig == 4) reord567(dd, iPerm, oPerm, nPo);
    else if(expConfig == 5) reord8(dd, iPerm, oPerm, nPo);
    else if(expConfig == 6) reord9(dd, iPerm, oPerm, nPo);
    else assert(false);
}

} // end namespace timeMux2
