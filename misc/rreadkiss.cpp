#include "ext-folding/utils/utils.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cassert>
#include <unordered_map>


using namespace std;

namespace kissToBlif
{
static string totoBin(int n, int bnum)
{
    string r;
    while(n!=0) {r=(n%2==0 ?"0":"1")+r; n/=2;}
    int s=r.size();
    for(;s<bnum;s++){
        r=("0")+r;
    }
    return r;
}

int KissNat_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    int c;
    bool enc = true, replace = false, dcVal = false;
    char *inKiss, *outBlif;

    inKiss = argv[globalUtilOptind++];
    //outBlif = argv[globalUtilOptind++];

    string dump,input,state,output,reset;
    vector<string> state_list;
    vector<string> encode_list;
    vector< vector<string> > table;  //save transitions
    int input_num, output_num, state_num, term_num;
    int time;
    fstream fin, fout;
    fin.open(inKiss, ios::in);

    for(int i=0;i<5;i++){
        fin>>dump;
    	if(dump==".i")
			fin>>input_num;
		else if(dump==".o")
			fin>>output_num;
		else if(dump==".s")
			fin>>state_num;
		else if(dump==".p")
			fin>>term_num;
		else if(dump==".r")
			fin>>reset;
        else i--;
    }


    for(int i=0;i<term_num;i++){
        vector<string> row;//save one transition
        fin>>input;
        row.push_back(input);
        time=2;
        while(time!=0){
            fin>>state;
            row.push_back(state);
            time--;
            int sls=state_list.size();
            if(sls==0) state_list.push_back(state);
            else{
                for(int j=0;j<sls;j++){
                    if(state_list[j]==state){
                        break;
                    }
                    else if(j==sls-1){
                        state_list.push_back(state);
                    }
                }
            }

        }

        fin>>output;
        for(char& c: output) if(c == '-') {
            if(dcVal) c = '1';
            else c = '0';
        }
        row.push_back(output);
        table.push_back(row);
    }
    fin.close();

	//calculate the number of bit we need
    int s=state_list.size()-1;
    int bnum=0;
    while(1){
        s=s/2;
        if(s>=0){
            bnum++;
            if(s==0) break;
        }
    }
	
	
	for(int i=0;i<state_list.size();i++){
        encode_list.push_back(totoBin(i,bnum));
    }

	//find r
    string r;
    for(int i=0;i<state_list.size();i++){
        if(state_list[i]==reset){
            r=encode_list[i];
            break;
        }
    }

	/*****start to write file*****/

	//first part : num of input and output
    Abc_Ntk_t *pNtk = utils::aigUtils::aigInitNtk(input_num, output_num, bnum, inKiss);
    
    //fout.open(outBlif, ios::out);
    //fout << ".model blif"<<endl;
    //fout << ".inputs ";
    //for(int i=0;i<input_num;i++){
    //    fout<<"v"<<i;
    //    if(i!=input_num-1) fout<<" ";
   // }
    //fout <<endl<< ".outputs ";
    int io_sum=input_num+output_num;
    //for(int i=input_num;i<io_sum;i++){
    //    fout<<"v"<<i;
    //    if(i!=io_sum-1) fout<<" ";
    //}

	//second part : latch
    int iob_sum=io_sum+bnum;
    //for(int i=io_sum;i<iob_sum;i++){
    //    fout<<endl<< ".latch ";
    //    fout<<"["<<i<<"] v"<<i<<" "<<r[i-io_sum];
    //}
    //fout <<endl<<endl;
    

	//third part : bits of next_st
	int logic_count=0;
	int ib_sum=input_num+bnum;
    for(int i=io_sum;i<iob_sum;i++){
        logic_count=0;
        //fout<< ".names ";
        //for(int j=0;j<input_num;j++) fout<<"v"<<j<<" ";
        //for(int j=io_sum;j<iob_sum;j++) fout<<"v"<<j<<" ";
        //fout<<"["<<i<<"]"<<endl;

        Abc_Obj_t *pPo = Abc_ObjFanin0(Abc_NtkBox(pNtk, i-io_sum));
        Abc_Obj_t *pObj = Abc_ObjNot(Abc_AigConst1(pNtk));

        for(int j=0;j<term_num;j++){
            Abc_Obj_t *pp = Abc_AigConst1(pNtk);

            //if bit=0, pass//
			int ff=0;
            for(int k=0;k<state_list.size();k++){
                if(state_list[k]==table[j][2]){
                    if((encode_list[k])[i-io_sum]=='0') {ff=1;break;}
                }
            }
            if(ff==1) continue;
			//---------------//

            logic_count++;
            for(int k=0; k<table[j][0].size(); ++k) {
                if(table[j][0][k] == '0')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_NtkPi(pNtk, k)), pp);
                else if(table[j][0][k] == '1')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_NtkPi(pNtk, k), pp);
            }

            //fout<<table[j][0];//input

            for(int k=0;k<state_list.size();k++){//cur_st
                if(state_list[k]==table[j][1]){
                    //fout<<encode_list[k]<<" ";

                    for(int m=0; m<encode_list[k].size(); ++m) {
                        if(encode_list[k][m] == '0')
                            pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_ObjFanout0(Abc_NtkBox(pNtk, m))), pp);
                        else if(encode_list[k][m] == '1')
                            pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjFanout0(Abc_NtkBox(pNtk, m)), pp);
                    }

                    break;
                }
            }
            for(int k=0;k<state_list.size();k++){//bit of next_st
                if(state_list[k]==table[j][2]){
                    //fout<<(encode_list[k])[i-io_sum];
                    assert((encode_list[k])[i-io_sum] == '1');
                    break;
                }
            }
            pObj = Abc_AigOr((Abc_Aig_t*)pNtk->pManFunc, pObj, pp);
            //fout<<endl;
        }

        //if(logic_count==0){
        //    for(int j=0;j<ib_sum;j++)
        //        fout<<"-";
        //    fout<<" 0\n";
        //}

        Abc_ObjAddFanin(pPo, pObj);
    }

	//fourth part : bits of output
	
    for(int i=input_num;i<io_sum;i++){
        logic_count=0;
        //fout<< ".names ";
        //for(int j=0;j<input_num;j++) fout<<"v"<<j<<" ";
        //for(int j=io_sum;j<iob_sum;j++) fout<<"v"<<j<<" ";
        //fout<<"v"<<i<<endl;

        Abc_Obj_t *pPo = Abc_NtkPo(pNtk, i-input_num);
        Abc_Obj_t *pObj = Abc_ObjNot(Abc_AigConst1(pNtk));

        for(int j=0;j<term_num;j++){
            Abc_Obj_t *pp = Abc_AigConst1(pNtk);
			//if bit=0, pass//
            if(((table[j][3])[i-input_num])=='0') {continue;}
			//---------------//
            logic_count++;

			//fout<<table[j][0];//input
            for(int k=0; k<table[j][0].size(); ++k) {
                if(table[j][0][k] == '0')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_NtkPi(pNtk, k)), pp);
                else if(table[j][0][k] == '1')
                    pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_NtkPi(pNtk, k), pp);
            }
            
            for(int k=0;k<state_list.size();k++){//cur_st
                if(state_list[k]==table[j][1]){
                    //fout<<encode_list[k]<<" ";

                    for(int m=0; m<encode_list[k].size(); ++m) {
                        if(encode_list[k][m] == '0')
                            pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjNot(Abc_ObjFanout0(Abc_NtkBox(pNtk, m))), pp);
                        else if(encode_list[k][m] == '1')
                            pp = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjFanout0(Abc_NtkBox(pNtk, m)), pp);
                    }

                    break;
                }
            }
            //fout<<(table[j][3])[i-input_num]<<endl;//output
            assert((table[j][3])[i-input_num] == '1');
            pObj = Abc_AigOr((Abc_Aig_t*)pNtk->pManFunc, pObj, pp);
        }
        //if(logic_count==0){
        //    for(int j=0;j<ib_sum;j++)
        //        fout<<"-";
        //    fout<<" 0\n";
        //}

        Abc_ObjAddFanin(pPo, pObj);
    }
	//fout<<".end";
    //fout.close();
    assert(Abc_NtkCheck(pNtk));
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtk);
    
    return 0;

}

// called during ABC startup
void init0(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Misc", "read_kiss0", KissNat_Command, 0);
}

// called during ABC termination
void destroy0(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer0 = { init0, destroy0 };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer0);
    }
} kissToBlif0_registrar;

} // end namespace kissToBlif