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


std::pair<int,float> getCloset(std::vector<Cluster*>&cellVec,std::vector<Net*>&netVec,Cluster*cell,std::vector<bool>&mark)
{
    int closet = -1;//if return is -1 , then cell has no neighbor.
    float score = -FLT_MAX;
    std::vector<Net*>NetsRecord;NetsRecord.reserve(cell->netNum);
    std::unordered_map<int,int>neighbors;//quickly find neighbors.

    std::list<int>Neighbors;
    for(auto netinfo:cell->getNets()){
        Net* net = netVec.at(netinfo.first);
        NetsRecord.push_back(net);
        for(auto neighbor:net->cells)
        {
            if(neighbor!=cell->sortId && cellVec.at(neighbor)->isValid()&&!mark.at(neighbor))
                neighbors.insert({neighbor,0});
        }
    }

    for(auto nid:neighbors){
        Cluster* n = cellVec.at(nid.first);

        // get Net 
        std::vector<Net*>n_NetsRecord;n_NetsRecord.reserve(n->netNum);
        for(auto netinfo:n->getNets())n_NetsRecord.push_back(netVec.at(netinfo.first));
 
        // get intersections
        auto inters = intersections(NetsRecord,n_NetsRecord);

        // score = w/(size1+size2)
        float sc = 0;
        for(auto net:inters){
            float w = 1/float(net->group1 + net->group2);
            sc += w/((cell->getSize() + n->getSize())*(cell->getSize() + n->getSize()));
        }
        if(sc > score){
            closet = nid.first;
            score = sc;
        }
    }
    return {closet,score};
}

//return cellNum after coarsen
int EdgeCoarsen(std::vector<Cluster*>&cellVec,std::vector<Net*>&netlist,int phase){

   

    InitNets(cellVec,netlist);//need init first.

    std::vector<bool>mark;mark.resize(cellVec.size(),false);
    std::vector<int>closetId;closetId.resize(cellVec.size(),-1);

    // #pragma omp parallel
    {
        // #pragma omp for
        for(int i = 0;i<cellVec.size();i++)            
        {
            Cluster& cell = *cellVec.at(i);
            if(cell.isValid() && !mark.at(i)){
                auto closet = getCloset(cellVec,netlist,&cell,mark);
                if(closet.first==-1)continue;
                mark.at(i) = true;
                mark.at(closet.first) = true;
                closetId.at(i) = closet.first;
            }
        }
    }
    
    
    std::list<int>clusterId;
    for(int i = 0;i<closetId.size();i++){
        if(closetId.at(i)!=-1){
            int cluserId1 = cellVec.at(i)->clusterId;
            int cluserId2 = cellVec.at(closetId.at(i))->clusterId;
            Cluster* c1 = cellVec.at(cluserId1);
            Cluster* c2 = cellVec.at(cluserId2);
            clusterId.push_front(c1->clustering(c2,phase));
        }
    }
    int Num = 0;
    for(auto cell:cellVec){
        if(cell->is_master()){
            Num++;
        }
    }
    return Num;
}


//remain vertex num , total phase
std::pair<int,int> Coarsen(std::vector<Cluster*>&cellVec,std::vector<Net*>&netlist){

    int targetNum = 2;
    int num;
    int phase = 0;
    while((num = EdgeCoarsen(cellVec,netlist,++phase))>targetNum)
    {
        std::cout<<"num:"<<num<<"\n";
    }
    std::cout<<"num:"<<num<<"\n";
    InitNets(cellVec,netlist);
    return {num,phase};
}


void debugInfo(std::vector<Cluster*>&cell);

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
    SortById(netList);

    int origin = 0;
    for(auto c:cellVec)
    {
        if(c->is_master())
        {
            origin++;
        }
    }
    std::cout<<"origin:"<<origin<<"\n";

    // showCell(cellVec,"d1.txt");

    InitialPartition_all1(cellVec);

    // std::cout<<"start coarsen\n";
    auto coarsenResult = Coarsen(cellVec,netList);
    std::cout<<"Coarsen to only "<<coarsenResult.first<<" cells\n";
    std::cout<<"total coasen stage:"<<coarsenResult.second<<"\n";
    std::cout<<"start Fm\n";
    FM(cellVec,netList,0.45,0.55,coarsenResult.second);
    std::cout<<"end Fm\n";
    // showCell(cellVec,"d2.txt");
    // showNet(netList,cellVec);
    // InitNets(cellVec,netList);
    
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
