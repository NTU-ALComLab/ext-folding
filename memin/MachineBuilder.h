/*
 * MachineBuilder.h
 *
 *  Created on: 06.03.2015
 *      Author: Andreas Abel
 */

#ifndef MACHINEBUILDER_H_
#define MACHINEBUILDER_H_

#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "ext-folding/memin/IncSpecSeq.h"

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::pair;
using std::stringstream;

namespace MeMin
{

void buildMachine(vector<vector<int> >& newMachineNextState, vector<vector<IncSpecSeq*> >& newMachineOutput, int& newResetState, int nClasses, vector<int>& dimacsOutput, vector<pair<int, int> >& literalToStateClass,  vector<vector<int> >& origMachineNextState, vector<vector<IncSpecSeq*> >& origMachineOutput, int origResetState, int maxInput);
void writeKISSFile(vector<vector<int> >& machineNextState, vector<vector<IncSpecSeq*> >& machineOutput, int resetState, int inputLength, int outputLength, vector<IncSpecSeq>& inputIDToIncSpecSeq, string filename);

} // end namespace MeMin

#endif /* MACHINEBUILDER_H_ */
