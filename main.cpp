#include "include/FM.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <queue>
#include <float.h>
#include <omp.h>

std::pair<std::vector<Cluster*>,std::vector<Net*>> parser(const std::string &filename);
void InitialPartition(std::vector<Cluster*>&cellVec,std::vector<bool>&partition);
void SortById(std::vector<Cluster*>&cellVec);
void SortById(std::vector<Net*>&netVec);

void InitialPartition_all1(std::vector<Cluster*>&cellVec);
void Output(std::vector<Cluster*>&cellVec);

void showNet(Net*net,std::vector<Cluster*>&cellVec,std::ofstream&out);
void showNet(std::vector<Net*>net,std::vector<Cluster*>&cellVec);
void showCell(std::vector<Cluster*>&cellVec,std::string name);

void showCell(Cluster*c);

void InitialPartition_avg(std::vector<Cluster*>&cellVec);




void debugInfo(std::vector<Cluster*>&cell);

int main(int argc,char*argv[]){

    if(argc!=2){
        std::cerr<<"please enter ./main <INPUT> \n";
        exit(1);
    }

    auto info = parser(argv[1]);//get unsorted cellVec
    // Get netlist and cell info.
    auto cellVec = info.first;
    auto netList = info.second;
    SortById(cellVec); // sort 
    SortById(netList);

    InitialPartition_all1(cellVec);

    int stage = 6;
    // if(cellVec.size()>80000)
    //     stage = 2;
    // else if(cellVec.size()>40000)
    //     stage = 3;
    // else if(cellVec.size()>10000)
    //     stage = 4;
    // else{
    //     stage = 10;
    // }
    
    auto coarsenResult = Coarsen(cellVec,netList,stage);
    std::cout<<"Coarsen to only "<<coarsenResult.first<<" cells\n";
    std::cout<<"total coarsen stage:"<<coarsenResult.second<<"\n";
    InitNets(cellVec,netList);
    FM(cellVec,netList,0.45,0.55,coarsenResult.second);

    std::cout<<"final cutsize:"<<CutSize(netList)<<"\n";
    Output(cellVec);
    for(auto net:netList)
        delete net;

    return 0;
}

std::pair<std::vector<Cluster*>,std::vector<Net*>> parser(const std::string &fileName){
   std::ifstream input{fileName};

   if(!input){std::cerr<<"can't open"<<fileName<<"\n";} 
    
    int NetsNum,CellNum;
    input >> NetsNum >> CellNum;

    std::vector<Cluster*>cells;cells.reserve(CellNum);
    std::unordered_map<int,int>cellRecord;//<id,vec_pos>

    
    std::string line;         
    std::getline(input,line);//清掉空白

    std::vector<Net*>nets;nets.reserve(NetsNum);
    
    //read nets
    for(int i = 1;i <= NetsNum;i++)
    {
        Net * net = new Net{i-1};//sorted id 
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
                cells.push_back(new Cluster{id});
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

void InitialPartition(std::vector<Cluster*>&cellVec,std::vector<bool>&partition)
{
    if(cellVec.size()!=partition.size())
    {
        std::cerr<<"Initial partition size is uncompatible\n";
        return ;
    }
    for(int i = 0;i<cellVec.size();i++){
        cellVec.at(i)->group1 = partition.at(i);
    }

}

void InitialPartition_avg(std::vector<Cluster*>&cellVec){
    bool group1 = true;
    for(int i = 0;i<cellVec.size();i++){
        if(cellVec.at(i)->isValid()){
            cellVec.at(i)->group1 = group1;
            group1 = !group1;
        }
    }
}

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

void showNet(std::vector<Net*>net,std::vector<Cluster*>&cellVec)
{
    std::ofstream out{"d1.txt"};
    InitNets(cellVec,net);
    for(auto n:net)
        showNet(n,cellVec,out);
    out.close();
}
void showNet(Net*net,std::vector<Cluster*>&cellVec,std::ofstream&out)
{

    
    std::cout<<"Net"<<net->NetId<<"\n";
    std::cout<<"total cells:"<<net->cells.size()<<"\n";
    std::cout<<"group1:"<<net->group1<<" group2 "<<net->group2<<"\n";
    for(auto c:net->cells){

        auto cell = cellVec.at(c); 
        if(!cell->isValid())continue;

        if(cell->is_master()){
            std::cout<<c<<" is a cluster master,cells : ";
            for(auto cells:cell->cells)
                std::cout<<cells->sortId<<" ";
        }
        else{
            std::cout<<c<<" ";
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";


    

    // out<<"Net"<<net->NetId<<"\n";
    // out<<"total cells:"<<net->cells.size()<<"\n";
    // out<<"group1:"<<net->group1<<" group2 "<<net->group2<<"\n";
    // for(auto c:net->cells){

    //     auto cell = cellVec.at(c); 
    //     if(!cell->isValid())continue;

    //     if(cell->is_master()){
    //         out<<c<<" is a cluster master,cells : ";
    //         for(auto cells:cell->cells)
    //             out<<cells->sortId<<" ";
    //     }
    //     else{
    //         out<<c<<" ";
    //     }
    //     out<<"\n";
    // }
    // out<<"\n";
    
}

void showCell(Cluster*c)
{

        std::cout<<"real id :"<<c->id<<"\n";
        std::cout<<"sort id :"<<c->sortId<<"\n";
        std::cout<<"cluster id:"<<c->clusterId<<"\n";
        std::string gp = c->group1 ? "gp1":"gp2";
        std::cout<<"in "<<gp<<"\n";
        std::cout<<"cell num:"<<c->cellsNum<<"\n";
        std::cout<<"nets:";
        
        for(auto n:c->getNets()){
            std::cout<<n.first<<" ";
        }
        std::cout<<"\n\n";
}
void showCell(std::vector<Cluster*>&cellVec,std::string name)
{

    std::ofstream out{name};

    for(auto c:cellVec)
    {
        out<<"real id :"<<c->id<<"\n";
        out<<"sort id :"<<c->sortId<<"\n";
        out<<"cluster id:"<<c->clusterId<<"\n";
        // std::string gp = c->group1 ? "gp1":"gp2";
        // out<<"in "<<gp<<"\n";
        
        
        if(c->is_master())
        out<<"is master\n";
        else 
        out<<"is not master\n";
        
        out<<"nets:";
        if(c->is_master()){
            std::vector<int>Netsid;Netsid.reserve(c->netNum);
            for(auto n:c->getNets()){
                Netsid.push_back(n.first);
            }
            // std::sort(Netsid.begin(),Netsid.end());

            for(auto n:Netsid)
                out<<n<<" ";
        }
        else{
            std::vector<int>Netsid;Netsid.reserve(c->netNum);
           for(auto n:c->getNets()){
                Netsid.push_back(n.first);
            }
            // std::sort(Netsid.begin(),Netsid.end());

            for(auto n:Netsid)
                out<<n<<" ";
        }
        out<<"\n\n";

    }
    out.close();


}
void SortById(std::vector<Cluster*>&cellVec){
    std::sort(cellVec.begin(),cellVec.end(),[](Cluster*c1,Cluster*c2){return c1->id < c2->id;});//sorted by cellId
    for(int sortId = 0;sortId < cellVec.size(); ++sortId){
        cellVec.at(sortId)->setSortId(sortId);
    }//store sortId 
}
void SortById(std::vector<Net*>&netVec){
    std::sort(netVec.begin(),netVec.end(),[](Net*c1,Net*c2){return c1->NetId < c2->NetId;});//sorted by cellId
    for(int i = 0;i<netVec.size();i++)
        netVec.at(i)->NetId = i;
}


void debugInfo(std::vector<Cluster*>&cell){

    std::ofstream out{"debugOut.txt"};

    for(auto c:cell){
        
        if(c->is_master())
            out<<c->sortId<<"\n";
    }





    out.close();



}
