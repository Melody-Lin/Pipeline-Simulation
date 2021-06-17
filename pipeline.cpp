//
//  main.cpp
//
//  Created by hclin on 2020/5/30.
//  Copyright © 2020 yihsuanlin. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <cmath>
using namespace std;

ifstream inFile;
ofstream outFile;

int reg[10]={0,9,5,7,1,2,3,4,5,6};
int mem[5]={5,9,4,8,7};
string instruction[20]; //store instructions
int tmp=0;
int pc;
//binary to decimal
int toDecimal(string str){
    int result=0;
    bool neg=false;
    //if it's a negative number
    if(str[0]=='1'){
        neg=true;
        //first, invert the bits
        for(int i=0;i<str.length();i++){
            if(str[i]=='1'){
                str[i]='0';
            }
            else{
                str[i]='1';
            }
        }
    }
    for(int i=str.length()-1,k=0;i>0;k++,i--){
        result+=(str[i]-'0')*pow(2,k);
    }
    //then, plus one and make it negative
    if(neg){
        result=-(result+1);
    }
    return result;
}
//instruction fetch
string ins="00000000000000000000000000000000";
//
void fetch(){
    pc+=4; //pc=pc+4
    if(instruction[pc/4]==""){
        ins="00000000000000000000000000000000";
    }
    else{
        ins=instruction[pc/4];
    }
}
//instruction decode and register file read
string op;
string funct;
string control="000000000"; //control signal: EX+MEM+WB
int readData1,readData2;
int sign_ext; //sign extend
int rs,rt,rd;
int rd_ex;  //rd of EX
string ctrl_ex="00000";
//
void decode(){
    if(ins=="00000000000000000000000000000000"){
        control="000000000";
        readData1=0,readData2=0;
        sign_ext=0;
        rs=0,rt=0,rd=0;
        return;
    }
    op=ins.substr(0,6); //get opcode
    funct=ins.substr(26,6); //get funct
    rs=toDecimal(ins.substr(6,5)); //get rs
    rt=toDecimal(ins.substr(11,5)); //get rt
    readData1=reg[rs]; //get data1
    readData2=reg[rt]; //get data2
    sign_ext=toDecimal(ins.substr(16,16)); //get sign extend
    //R-type:add,sub,and,or,slt
    if(op=="000000"){
        rd=toDecimal(ins.substr(16,5)); //get rd
        //Load Hazard
        //if the former instruction is lw and a load hazard happens, then stall yourself
        if(ctrl_ex=="01011" && (rd_ex==rt || rd_ex==rs)){
                pc-=4;
                control="000000000";
        }
        else{
                control="110000010";
        }
    }
    //I-type:lw,sw,addi,andi,beq
    else{
        rd=0;
        //lw
        if(op=="100011"){
                control="000101011";
        }
        //sw
        else if(op=="101011"){
            control="000100100";
        }
        //addi
        else if(op=="001000"){
            control="000100010";
        }
        //andi
        else if(op=="001100"){
            control="011100010";
        }
        //beq
        else if(op=="000100"){
            control="001010000";
        }
    }
}
//execution or address calculation
int ALUout_ex;  //ALUout of EX
int writeData;
bool branch;  //check if branch happened
string ALUop;
//
void execute(){
    if(control=="000000000"){
        ctrl_ex="00000";
        rd_ex=0;
        writeData=0;
        ALUout_ex=0;
        return;
    }
    writeData=readData2;
    ALUop=control.substr(1,2);  //get ALUop from control signal
    ctrl_ex=control.substr(4,5);  //control signal: MEM & WB
    branch=false;
    //R-type
    if(control.substr(0,1)=="1"){
        rd_ex=rd;  //from rd
        tmp=1; //asserted
    }
    //I-type
    else{
        rd_ex=rt; //from rt
        tmp=0; //non-asserted
    }
    int tmp1=readData1;
    int tmp2;
    //control[3]: ALUSrc
    if(control[3]=='0'){
        tmp2=readData2; //ALU的讀取資料來自Read data2
    }
    else{
        tmp2=sign_ext; //ALU的讀取資料來自sign extend的立即值
    }
    //R-type:add,sub,and,or,slt
    if(ALUop=="10"){
        //add
        if(funct=="100000"){
            ALUout_ex=tmp1+tmp2;
        }
        //sub
        else if(funct=="100010"){
            ALUout_ex=tmp1-tmp2;
        }
        //and
        else if(funct=="100100"){
            ALUout_ex=tmp1&tmp2;
        }
        //or
        else if(funct=="100101"){
            ALUout_ex=tmp1|tmp2;
        }
        //slt
        else if(funct=="101010"){
            ALUout_ex=(tmp1<tmp2)?1:0;
        }
        //nop
        else{
            ALUout_ex=0;
        }
    }
    //I-type:lw,sw,addi,andi,beq
    //lw,sw,addi
    else if(ALUop=="00"){
        ALUout_ex=tmp1+tmp2;
    }
    //beq
    else if(ALUop=="01"){
        ALUout_ex=tmp1-tmp2;
        if(ALUout_ex==0){
            pc=pc+(sign_ext*4)-4;
            ins="00000000000000000000000000000000";
        }
    }
    //andi
    else if(ALUop=="11"){
        ALUout_ex=tmp1&tmp2;
    }
}
//data memory access
int readData;
int rd_mem;  //rd of MEM
int ALUout_mem;  //ALUout of MEM
string ctrl_mem="00";  //control signal: WB
void memoryAccess(){
    if(ctrl_ex=="00000"){
        readData=0;
        rd_mem=0;
        ALUout_mem=0;
        ctrl_mem="00";
        return;
    }
    rd_mem=rd_ex;
    ALUout_mem=ALUout_ex;
    ctrl_mem=ctrl_ex.substr(3,2);
    //lw
    if(ctrl_mem=="11"){
        readData=mem[ALUout_mem/4];
    }
    //sw
    else if(ctrl_ex[2]=='1'){
        mem[ALUout_mem/4]=rd_mem;
        readData=0;
    }
    else{
        readData=0;
    }
}
//write back
void writeBack(){
    if(rd_mem==0){
        return;
    }
    if(ctrl_mem[1]=='1'){
        reg[rd_mem]=readData;
    }
    else if(ctrl_mem[1]=='0' && ctrl_mem[0]=='1'){
        reg[rd_mem]=ALUout_mem;
    }
}
//Data Hazard:EX
void exHazard(){
    //Forward A=10
    if(ctrl_ex[3]=='1' && rd_ex!=0 && rd_ex==rs && ctrl_ex!="01011"){  //without load hazard
        readData1=ALUout_ex;
    }
    //Forward B=10
    if(ctrl_ex[3]=='1' && rd_ex!=0 && rd_ex==rt && ctrl_ex!="01011"){  //without load hazard
        readData2=ALUout_ex;
    }
}
//Data Hazard:MEM
void memHazard(){
    //Forward A=01
    if(ctrl_mem[0]=='1' && rd_mem!=0 && rd_ex!=rs && rd_mem==rs){
        readData1=(ctrl_mem[1]=='1')?readData:ALUout_mem;
    }
    //Forward B=01
    if(ctrl_mem[0]=='1' && rd_mem!=0 && rd_ex!=rt && rd_mem==rt){
        readData2=(ctrl_mem[1]=='1')?readData:ALUout_mem;
    }
}
//Branch Hazard
void branchHazard(){
    if (branch){
        ins="00000000000000000000000000000000";
    }
}
int cc;  //clock cycle
//create pipeline
void pipeline(){
    cc=0;
}
//print result
void print(){
    outFile<<"CC "<<cc<<":"<<endl;
    outFile<<endl;
    outFile<<"Registers:"<<endl;
    outFile<<"$0: "<<reg[0]<<endl;
    outFile<<"$1: "<<reg[1]<<endl;
    outFile<<"$2: "<<reg[2]<<endl;
    outFile<<"$3: "<<reg[3]<<endl;
    outFile<<"$4: "<<reg[4]<<endl;
    outFile<<"$5: "<<reg[5]<<endl;
    outFile<<"$6: "<<reg[6]<<endl;
    outFile<<"$7: "<<reg[7]<<endl;
    outFile<<"$8: "<<reg[8]<<endl;
    outFile<<"$9: "<<reg[9]<<endl;
    outFile<<endl;
    outFile<<"Data memory:"<<endl;
    outFile<<"0x00: "<<mem[0]<<endl;
    outFile<<"0x04: "<<mem[1]<<endl;
    outFile<<"0x08: "<<mem[2]<<endl;
    outFile<<"0x0C: "<<mem[3]<<endl;
    outFile<<"0x10: "<<mem[4]<<endl;
    outFile<<endl;
    outFile<<"IF/ID :"<<endl;
    outFile<<"PC              "<<pc<<endl;
    outFile<<"Instruction     "<<ins<<endl;
    outFile<<endl;
    outFile<<"ID/EX :"<<endl;
    outFile<<"ReadData1       "<<readData1<<endl;
    outFile<<"ReadData2       "<<readData2<<endl;
    outFile<<"sign_ext        "<<sign_ext<<endl;
    outFile<<"Rs              "<<rs<<endl;
    outFile<<"Rt              "<<rt<<endl;
    outFile<<"Rd              "<<rd<<endl;
    outFile<<"Control signals "<<control<<endl;
    outFile<<endl;
    outFile<<"EX/MEM :"<<endl;
    outFile<<"ALUout          "<<ALUout_ex<<endl;
    outFile<<"WriteData       "<<writeData<<endl;
    outFile<<"Rt/Rd           "<<rd_ex<<endl;
    outFile<<"Control signals "<<ctrl_ex<<endl;
    outFile<<endl;
    outFile<<"MEM/WB :"<<endl;
    outFile<<"Read Data       "<<readData<<endl;
    outFile<<"ALUout          "<<ALUout_mem<<endl;
    outFile<<"Rt/Rd           "<<rd_mem<<endl;
    outFile<<"Control signals "<<ctrl_mem<<endl;
    outFile<<"================================================================="<<endl;
}
//to next clock cycle
bool nextcc(){
    memHazard();
    exHazard();
    writeBack();
    memoryAccess();
    execute();
    //check hazard
    decode();
    branchHazard();
    fetch();
    cc++;
    print();
    //if finish executing all instructions
    if(cc!=1 && control=="000000000" && ctrl_ex=="00000" && readData==0 && ctrl_mem=="00" && ALUout_mem==0){
        return false;
    }
    else{
        return true;
    }
}
//initializer: initialize register, memory, variables
void initialize(){
    //initialize register
    reg[0]=0;
    reg[1]=9;
    reg[2]=5;
    reg[3]=7;
    reg[4]=1;
    reg[5]=2;
    reg[6]=3;
    reg[7]=4;
    reg[8]=5;
    reg[9]=6;
    //initialize memory
    mem[0]=5;
    mem[1]=9;
    mem[2]=4;
    mem[3]=8;
    mem[4]=7;
    instruction[20];
    tmp=0;
    pc=0;
    op="";
    funct="";
    readData1=0,readData2=0;
    sign_ext=0;
    rs=0,rt=0,rd=0;
    ALUout_ex=0;
    writeData=0;
    branch=false;
    ALUop="";
    rd_ex=0;
    readData=0;
    rd_mem=0;
    ALUout_mem=0;
}
void gerneral(){
    initialize();
    inFile.open("General.txt",ios::in);
    outFile.open("genResult.txt",ios::out);
    int n=1;
    while(inFile>>instruction[n]){
        n++;
    }
    pipeline();
    while(nextcc());
    inFile.close();
    outFile.close();
}
void datahazard(){
    initialize();
    inFile.open("Datahazard.txt",ios::in);
    outFile.open("dataResult.txt",ios::out);
    int n=1;
    while(inFile>>instruction[n]){
        n++;
    }
    pipeline();
    while(nextcc());
    inFile.close();
    outFile.close();
}
void loadhazard(){
    initialize();
    inFile.open("Lwhazard.txt",ios::in);
    outFile.open("loadResult.txt",ios::out);
    int n=1;
    while(inFile>>instruction[n]){
        n++;
    }
    pipeline();
    while(nextcc());
    inFile.close();
    outFile.close();
}
void branchhazard(){
    initialize();
    inFile.open("Branchhazard.txt",ios::in);
    outFile.open("branchResult.txt",ios::out);
    int n=1;
    while(inFile>>instruction[n]){
        n++;
    }
    pipeline();
    while(nextcc());
    inFile.close();
    outFile.close();
}
int main(int argc, const char * argv[]) {
    gerneral();
    datahazard();
    loadhazard();
    branchhazard();
    return 0;
}
