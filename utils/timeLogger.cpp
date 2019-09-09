#include "ext-folding/utils/utils.h"

namespace utils
{

TimeLogger::TimeLogger(const string& fileName, bool v, bool f): verbose(v), instantFlush(f)
{
    fp.open(fileName.c_str());
    fp << "checkpoint,interval,elapsed time\n";
    if(instantFlush) fp.flush();

    start = prev = clock();
    if(verbose) cout << "TimeLogger initilized, info will be recorded in \"" << fileName << "\"." << endl;
}

TimeLogger::~TimeLogger()
{
    fp.close();
    if(verbose) cout << "TimeLogger destroyed." << endl;
}

void TimeLogger::log(const string& str)
{
    clock_t now = clock();
    double i = double(now - prev) / CLOCKS_PER_SEC;
    double e = double(now - start) / CLOCKS_PER_SEC;
    prev = now;

    fp << str << "," << i << "," << e << "\n";
    if(instantFlush) fp.flush();
    if(verbose) cout << str << " " << i << " " << e << endl;
}

} // end namespace utils
