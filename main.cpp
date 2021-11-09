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

std::pair<std::vector<Cluster*>,std::list<Net*>> parser(const std::string &filename);

void SortById(std::vector<Cluster*>&cellVec);
void InitialPartition_all1(std::vector<Cluster*>&cellVec);
void Output(std::vector<Cluster*>&cellVec);

void showNet(Net*net,std::vector<Cluster*>&cellVec);


void showCell(std::vector<Cluster*>&cellVec);

void showCell(Cluster*c);


// using ClusterQ = std::priority_queue<>
// void Cluster

//linear intersections
std::list<Net*> intersections(std::vector<Net*>&v1,std::vector<Net*>&v2)
{   
    auto nidcmp = [](Net*n1,Net*n2){return n1->NetId < n2->NetId;};
    std::sort(v1.begin(),v1.end(),nidcmp);
    std::sort(v2.begin(),v2.end(),nidcmp);
    int i = 0,j = 0;
    std::list<Net*>inters;
    while(i < v1.size() && j < v2.size()){
        int nid1 = v1.at(i)->NetId;
        int nid2 = v2.at(j)->NetId;
        if(nid1==nid2){
            inters.push_front(v1.at(i));
            i++;
            j++;
        }else if (nid1 < nid2){
            i++;
        }else{
            j++;
        }
    }
    return inters;
}


std::pair<int,float> getCloset(std::vector<Cluster*>&cellVec,Cluster*cell)
{
    int closet = -1;//if return is -1 , then cell has no neighbor.
    float score = -FLT_MAX;
    std::vector<Net*>NetsRecord;NetsRecord.reserve(cell->netNum);
    std::unordered_map<int,int>neighbors;//quickly find neighbors.
    for(auto net:cell->getNetlist()){
        NetsRecord.push_back(net);
        for(auto neighbor:net->cells){
            if(neighbor!=cell->sortId)
                neighbors.insert({neighbor,0});
        }
    }

  
    for(auto nid:neighbors){
        Cluster* n = cellVec.at(nid.first);

        // get Net 
        std::vector<Net*>n_NetsRecord;n_NetsRecord.reserve(n->netNum);
        for(auto net:n->getNetlist())n_NetsRecord.push_back(net);
 
        // get intersections
        auto inters = intersections(NetsRecord,n_NetsRecord);

        // find best
        float sc = 0;
        for(auto net:inters){
            float w = 1/float(net->group1 + net->group2);
            sc += w/(cell->getSize() + n->getSize());
        }
        if(sc > score){
            closet = nid.first;
            score = sc;
        }
    }
    return {closet,score};
}

void ClusterQinit(std::vector<Cluster*>&cellVec,std::list<Net*>&netlist){

    // std::unordered_map<int,int>closetPairs;//any vertex v only has one closed neighbor u.

    InitNets(cellVec,netlist);//need init first.

    //  closetPairs.at(idx).first : the closet vertex of idx,  
    //  closetPairs.at(idx).second : the coresponding cost
    std::vector<std::pair<int,float>>closetPairs;closetPairs.resize(cellVec.size());




    //可能可以平行化
    #pragma omp parllel 
    {
        std::cout<<omp_get_num_threads()<<"\n";
        #pragma omp for
        for(int i = 0;i < cellVec.size();i++){
            Cluster& cell = *cellVec.at(i);
            closetPairs.at(i) = getCloset(cellVec,&cell);
        }
    }


}



int main(int argc,char*argv[]){

    if(argc!=2)
    {
        std::cerr<<"please enter ./main <INPUT> \n";
        exit(1);
    }

 
    
    auto info = parser(argv[1]);//get unsorted cellVec


    // Get netlist and cell info.
    auto cellVec = info.first;
    auto netList = info.second;
    SortById(cellVec); // sort 



 

    

    ClusterQinit(cellVec,netList);



   // give a inital partition or useing clustering
    //clustering test
    // int id = cellVec.at(0)->clustering(cellVec.at(1));
    // id = cellVec.at(id)->clustering(cellVec.at(2));
    // std::cout<<"id : "<<id<<"\n";

    // std::cout<<cellVec.at(id)->is_cluster()<<" "<<cellVec.at(id)->is_master()<<"\n";
    // cellVec.at(id)->BuildClustersNets();



    // //after clustering, doing initial
    // InitialPartition_all1(cellVec);

    // //InitNets
    // InitNets(cellVec,netList);
    // // for(auto net:netList)showNet(net,cellVec);

    // // showCell(cellVec);



    // // std::cout<<"start Fm\n";
    // FM(cellVec,netList,0.45,0.55);
    // // std::cout<<"end Fm\n";

    // // InitNets(cellVec,netList);
    // std::cout<<"final cutsize:"<<CutSize(netList)<<"\n";
    // // Output(cellVec);
    // // for(auto net:netList)
    // //     delete net;


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
}

void showCell(Cluster*c)
{

        std::cout<<"real id :"<<c->id<<"\n";
        std::cout<<"sort id :"<<c->sortId<<"\n";
        std::cout<<"cluster id:"<<c->clusterId<<"\n";
        std::string gp = c->group1 ? "gp1":"gp2";
        std::cout<<"in "<<gp<<"\n";
        std::cout<<"nets:";
        
        for(auto n:c->getNetlist()){
            std::cout<<n->NetId<<" ";
        }
        std::cout<<"\n\n";
}
void showCell(std::vector<Cluster*>&cellVec)
{
    for(auto c:cellVec)
    {
        std::cout<<"real id :"<<c->id<<"\n";
        std::cout<<"sort id :"<<c->sortId<<"\n";
        std::cout<<"cluster id:"<<c->clusterId<<"\n";
        std::string gp = c->group1 ? "gp1":"gp2";
        std::cout<<"in "<<gp<<"\n";
        std::cout<<"nets:";
        
        for(auto n:c->getNetlist()){
            std::cout<<n->NetId<<" ";
        }
        std::cout<<"\n\n";

    }



}
void SortById(std::vector<Cluster*>&cellVec){
    std::sort(cellVec.begin(),cellVec.end(),[](Cluster*c1,Cluster*c2){return c1->id < c2->id;});//sorted by cellId
    for(int sortId = 0;sortId < cellVec.size(); ++sortId){
        cellVec.at(sortId)->setSortId(sortId);
    }//store sortId 
}
