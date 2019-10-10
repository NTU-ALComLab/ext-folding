/*
 * IncSpecSeq.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: Andreas Abel
 */

#include "ext-folding/memin/IncSpecSeq.h"

namespace MeMin
{

std::ostream& operator<< (std::ostream &out, IncSpecSeq& output) {
	for (unsigned int i=0; i<output.seqLength; i++) {
		int curPos = i%(sizeof(int)*8);
		if (((output.specifiedBits[i/(sizeof(int)*8)]>>curPos)&1)==0) {
			out<<"-";
		} else {
			out<<((output.seq[i/(sizeof(int)*8)]>>curPos)&1);
		}
	}
	return out;
}

} // end namespace MeMin
