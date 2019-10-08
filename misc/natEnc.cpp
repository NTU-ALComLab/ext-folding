// written by Yu-Zhou, Lin and Yu-Rong, Zhu

#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

namespace kissToBlif
{

static string toBinary(int n, int bnum)
{
    string r;
    while(n!=0) {r=(n%2==0 ?"0":"1")+r; n/=2;}
    int s=r.size();
    for(;s<bnum;s++){
        r=("0")+r;
    }
    return r;
}


void natEnc(const char *inKiss, const char *outBlif, const bool dcVal)
{
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
        encode_list.push_back(toBinary(i,bnum));
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
    fout.open(outBlif, ios::out);
    fout << ".model blif"<<endl;
    fout << ".inputs ";
    for(int i=0;i<input_num;i++){
        fout<<"v"<<i;
        if(i!=input_num-1) fout<<" ";
    }
    fout <<endl<< ".outputs ";
    int io_sum=input_num+output_num;
    for(int i=input_num;i<io_sum;i++){
        fout<<"v"<<i;
        if(i!=io_sum-1) fout<<" ";
    }

	//second part : latch
    int iob_sum=io_sum+bnum;
    for(int i=io_sum;i<iob_sum;i++){
        fout<<endl<< ".latch ";
        fout<<"["<<i<<"] v"<<i<<" "<<r[i-io_sum];
    }
    fout <<endl<<endl;

	//third part : bits of next_st
	int logic_count=0;
	int ib_sum=input_num+bnum;
    for(int i=io_sum;i<iob_sum;i++){
        logic_count=0;
        fout<< ".names ";
        for(int j=0;j<input_num;j++) fout<<"v"<<j<<" ";
        for(int j=io_sum;j<iob_sum;j++) fout<<"v"<<j<<" ";
        fout<<"["<<i<<"]"<<endl;
        for(int j=0;j<term_num;j++){
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
            fout<<table[j][0];//input
            for(int k=0;k<state_list.size();k++){//cur_st
                if(state_list[k]==table[j][1]){
                    fout<<encode_list[k]<<" ";
                    break;
                }
            }
            for(int k=0;k<state_list.size();k++){//bit of next_st
                if(state_list[k]==table[j][2]){
                    fout<<(encode_list[k])[i-io_sum];
                    break;
                }
            }
            fout<<endl;
        }
        if(logic_count==0){
            for(int j=0;j<ib_sum;j++)
                fout<<"-";
            fout<<" 0\n";
        }
    }

	//fourth part : bits of output
	
    for(int i=input_num;i<io_sum;i++){
        logic_count=0;
        fout<< ".names ";
        for(int j=0;j<input_num;j++) fout<<"v"<<j<<" ";
        for(int j=io_sum;j<iob_sum;j++) fout<<"v"<<j<<" ";
        fout<<"v"<<i<<endl;

        for(int j=0;j<term_num;j++){
			//if bit=0, pass//
            if(((table[j][3])[i-input_num])=='0') {continue;}
			//---------------//
            logic_count++;
			fout<<table[j][0];//input
            for(int k=0;k<state_list.size();k++){//cur_st
                if(state_list[k]==table[j][1]){
                    fout<<encode_list[k]<<" ";
                    break;
                }
            }
            fout<<(table[j][3])[i-input_num]<<endl;//output

        }
        if(logic_count==0){
            for(int j=0;j<ib_sum;j++)
                fout<<"-";
            fout<<" 0\n";
        }
    }
	fout<<".end";
    fout.close();
}

} // end namespace kissToBlif