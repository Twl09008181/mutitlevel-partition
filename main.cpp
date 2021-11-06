#include "include/FM.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <map>

std::pair<std::vector<Cell>,std::list<Net*>> parser(const std::string &filename);
void InitialPartition(std::vector<Cell>&cellVec,std::vector<bool>&partition);
void InitNets(std::vector<Cell>&cellVec);
//group cells



std::map<int,char>dict{
    {1,'a'},{2,'b'},{3,'c'},{4,'d'},{5,'e'},{6,'f'},{7,'g'},{8,'h'} 
};


int main(int argc,char*argv[]){

    if(argc!=2)
    {
        std::cerr<<"please enter ./main <INPUT> \n";
        exit(1);
    }

    
    auto info = parser(argv[1]);//get unsorted cellVec

    auto cellVec = info.first;
    auto netList = info.second;

    std::sort(cellVec.begin(),cellVec.end(),[](Cell&c1,Cell&c2){return c1.id < c2.id;});//sorted by cellId
    for(int sortId = 0;sortId < cellVec.size(); ++sortId){cellVec.at(sortId).sortId = sortId;}//store sortId 

    //init net

    //using slide p31
    //[a b c d e f g h 
    //[1,0,1,1,0,0,1,0]
    std::vector<bool>partition{1,0,1,1,0,0,1,0};
    InitialPartition(cellVec,partition);         //這邊要想辦法做一個初始的partition
     


    #ifdef DEBUG
        for(auto &cell:cellVec)
        {
            std::string gp = cell.group1 ? "group1" : "group2";
            std::cout<<dict[cell.id]<<" is " << gp <<"\n";
        }
    #endif

    InitNets(cellVec,netList);
    
    #ifdef DEBUG
        for(auto net:netList){
            std::cout<<"Net:"<<net->NetId<<"\n";
            std::cout<<"gp1 num:"<<net->group1<<" gp2 num:"<<net->group2<<"\n";
            for(auto cell:net->cells)
            {
                std::cout<<dict[cellVec.at(cell).id]<<" ";
            }
            std::cout<<"\n";
        }
    #endif


    #ifdef DEBUG
    FM(cellVec,0.375,0.625);
    #endif

    #ifndef DEBUG
    FM(cellVec,0.45,0.55);
    #endif

    for(auto net:netList)
        delete net;
    return 0;
}

std::pair<std::vector<Cell>,std::list<Net*>> parser(const std::string &fileName){
   std::ifstream input{fileName};

   if(!input){std::cerr<<"can't open"<<fileName<<"\n";} 
    
    int NetsNum,CellNum;
    input >> NetsNum >> CellNum;

    std::vector<Cell>cells;cells.resize(CellNum);
    std::unordered_map<int,int>cellRecord;//<id,vec_pos>

    
    std::string line;         
    std::getline(input,line);//清掉空白

    std::list<Net*>nets;
    
    //read nets
    for(int i = 1;i <= NetsNum;i++)
    {
        Net * net = new Net{i};
        nets.push_back(net);
        // line process
        std::string line;
        std::getline(input,line);
        std::stringstream ss{line};

        // read cells
        int id;
        while(ss >> id){
            auto cptr = cellRecord.find(id);
            if(cptr == cellRecord.end())
            {
                int pos = cellRecord.size();
                cellRecord.insert({id,pos});
                cells.at(pos).id = id;
                cells.at(pos).addNet(net);
            }
            else{
                int pos = cptr->second;
                cells.at(pos).addNet(net);
            }
        }
    }
    input.close();   
    return {cells,nets}; 
}
void InitialPartition(std::vector<Cell>&cellVec,std::vector<bool>&partition)
{
    if(cellVec.size()!=partition.size())
    {
        std::cerr<<"Initial partition size is uncompatible\n";
        return ;
    }
    for(int i = 0;i<cellVec.size();i++){
        cellVec.at(i).group1 = partition.at(i);
    }

}

