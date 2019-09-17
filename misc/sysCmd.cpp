#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <string>
#include <iostream>

using namespace std;

namespace sysCmd
{

int Sys_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    string cmd;
    bool verbosity = true;

    Extra_UtilGetoptReset();
    while((c=Extra_UtilGetopt(argc, argv, "vh")) != EOF) {
        switch(c) {
        case 'v':
            verbosity = !verbosity;
            break;
        case 'h': default:
            goto usage;
        }
    }
    
    if(globalUtilOptind >= argc) {
        cerr << "Please type in the command." << endl;
        goto usage;
    }

    for(size_t i=globalUtilOptind; i<argc; ++i) {
        cmd += argv[globalUtilOptind];
        cmd += " ";
    }
    if(!verbosity)
        cmd += " > /dev/null";

    system(cmd.c_str());

    return 0;

usage:
    Abc_Print(-2, "usage: sys [-v] <cmd>\n");
    Abc_Print(-2, "\t        execute system command\n");
    Abc_Print(-2, "\t-v    : toggles verbosity [default = %s]\n", verbosity ? "on" : "off");
    Abc_Print(-2, "\tcmd    : command to be executed\n");
    Abc_Print(-2, "\t-h    : print the command usage\n");
    return 1;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "System Command", "sys", Sys_Command, 0);
}

// called during ABC termination
void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer);
    }
} sys_registrar;

} // end namespace sysCmd