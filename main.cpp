#include "include/FM.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <map>

// std::pair<std::vector<Cell>,std::list<Net*>> parser(const std::string &filename);
// void InitialPartition(std::vector<Cell>&cellVec,std::vector<bool>&partition);
// void InitialPartition_avg(std::vector<Cell*>&cellVec);
// void InitialPartition_all1(std::vector<Cell>&cellVec);
// void Output(std::vector<Cell>&cellVec);
//group cells



std::pair<std::vector<Cluster*>,std::list<Net*>> parser(const std::string &filename);
// void InitialPartition(std::vector<Cluster>&cellVec,std::vector<bool>&partition);
// void InitialPartition_avg(std::vector<Cluster*>&cellVec);
void InitialPartition_all1(std::vector<Cluster*>&cellVec);
void Output(std::vector<Cluster*>&cellVec);

void showNet(Net*net,std::vector<Cluster*>&cellVec);

int main(int argc,char*argv[]){

    if(argc!=2)
    {
        std::cerr<<"please enter ./main <INPUT> \n";
        exit(1);
    }

   

    
    auto info = parser(argv[1]);//get unsorted cellVec

    auto cellVec = info.first;
    auto netList = info.second;

    std::sort(cellVec.begin(),cellVec.end(),[](Cluster*c1,Cluster*c2){return c1->id < c2->id;});//sorted by cellId
    for(int sortId = 0;sortId < cellVec.size(); ++sortId){cellVec.at(sortId)->sortId = sortId;}//store sortId 

    //init net

    InitialPartition_all1(cellVec);
    InitNets(cellVec,netList);

    // for(auto net:netList){showNet(net,cellVec);}

    // std::cout<<"done\n";

    // std::cout<<cellVec.at(1)->sortId<<"\n";
    // std::cout<<&cellVec.at(1)<<"\n";

    // std::cout<<cellVec.at(1)->cells.front()<<"\n";



    cellVec.at(0)->clustering(cellVec.at(1));
    cellVec.at(0)->BuildClustersNets();
    InitNets(cellVec,netList);
    for(auto net:netList){showNet(net,cellVec);}
    

    cellVec.at(0)->decluster();
    InitNets(cellVec,netList);
    for(auto net:netList){showNet(net,cellVec);}

    // FM(cellVec,netList,0.45,0.55);


    // std::cout<<"final cutsize:"<<CutSize(netList)<<"\n";
    // Output(cellVec);
    // for(auto net:netList)
    //     delete net;


    return 0;
}

std::pair<std::vector<Cluster*>,std::list<Net*>> parser(const std::string &fileName){
   std::ifstream input{fileName};

   if(!input){std::cerr<<"can't open"<<fileName<<"\n";} 
    
    int NetsNum,CellNum;
    input >> NetsNum >> CellNum;

    std::vector<Cluster*>cells;cells.reserve(CellNum);
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
            if(cptr == cellRecord.end()){
                int pos = cells.size();
                cellRecord.insert({id,pos});
            
                //build cell
                cells.push_back(new Cluster{id,pos});
                cells.at(pos)->addNet(net);
            }else{
                int pos = cptr->second;
                cells.at(pos)->addNet(net);
            }
        }
    }
    input.close();   
    return {cells,nets}; 
}
// void InitialPartition(std::vector<Cell>&cellVec,std::vector<bool>&partition)
// {
//     if(cellVec.size()!=partition.size())
//     {
//         std::cerr<<"Initial partition size is uncompatible\n";
//         return ;
//     }
//     for(int i = 0;i<cellVec.size();i++){
//         cellVec.at(i).group1 = partition.at(i);
//     }

// }

// void InitialPartition_avg(std::vector<Cell>&cellVec){
//     bool group1 = true;
//     for(int i = 0;i<cellVec.size();i++){
//         cellVec.at(i).group1 = group1;
//         group1 = !group1;
//     }
// }

void InitialPartition_all1(std::vector<Cluster*>&cellVec){
    for(int i = 0;i<cellVec.size();i++)
        cellVec.at(i)->group1 = true;
}
void Output(std::vector<Cluster*>&cellVec){
    std::ofstream out{"output.txt"};
    for(auto c:cellVec)
        out << c->group1 <<"\n";
    out.close();
}
void showNet(Net*net,std::vector<Cluster*>&cellVec)
{
    std::cout<<"Net"<<net->NetId<<"\n";
    std::cout<<"total cells:"<<net->cells.size()<<"\n";
    std::cout<<"group1:"<<net->group1<<" group2 "<<net->group2<<"\n";
    for(auto c:net->cells){

        auto cell = cellVec.at(c); 
        if(!cell->isValid())continue;

        if(cell->is_cluster()){
            std::cout<<c<<" is a cluster,cells : ";
            for(auto cells:cell->cells)
                std::cout<<cells->sortId<<" ";
        }
        else{
            std::cout<<c<<" ";
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";
}
