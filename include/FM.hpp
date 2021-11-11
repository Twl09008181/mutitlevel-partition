
#ifndef FM_HPP
#define FM_HPP
#include <list>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <queue>

struct Cell;
struct Net{
    Net(int id):NetId{id}{}
    int NetId;
    using CellId = int;//pesudo id (sort id)
    std::list<CellId> cells;

    //need maintain for quickly cacluating gains.
    int group1 = 0;
    int group2 = 0;
    void addCells(Cell*cell);
};

struct Cell{
    int id;//編號不一定會是從1~node的總數
    int sortId;
    bool group1 = false;
    int netNum = 0;
    std::map<int,int>NetSet;//net id , second is for cluster
    virtual void addNet(Net*net){NetSet.insert({net->NetId,1});netNum++;}
    // bucket relative
    int gain = 0;
    bool freeze = true;
    std::list<int>::iterator it;//to access gain bucket.
    std::map<int,int>& getNets(){return NetSet;}
    virtual bool isValid(){return true;}
    virtual int getSize()const{return 1;}  
};//using std::vector<Cell> to save all Cells.   




//a specialed cell , can be used in coarsening and uncoarsening
class Cluster : public Cell{
public:
    Cluster(int realid){
        id = realid;
        // cluster relative flag.
        cells.push_front(this);
        valid = true;
        iscluster = false;
        clusterId = sortId = -1;
        cellsNum = 1;
    }

    void setSortId(int sortedId){clusterId = sortId = sortedId;}
    bool is_master(){return sortId==clusterId;}
    bool is_cluster(){return cellsNum > 1;}//only one cell is not a cluster.

    // cluster relative member
    int clusterId;
    bool valid;//cluster or non-cluster single valid cell. 
    bool iscluster;
    std::list<Cluster*>cells;//紀錄存了哪些cluster 
    int originNetNum = 0;//當變為non-cluster時會用到
    int cellsNum;//maintain for quickly get size.
    int clusterPhase = -1;//used to decluster
    

    // update cluster member
    int clustering(Cluster*v,int phase);//return id
    std::vector<int> decluster(int phase);


    // inherint functions
    int getSize()const{return cellsNum;}
    bool isValid(){
        if(is_master()&&!valid){std::cerr<<"isValid warning, a cluster ,master is always valid,you may need to check if you have already call BuildClustersNets\n";}
        return valid;
    }
    void addNet(Net*net){
        if(is_cluster()){std::cerr<<"warrning,this cell is already a cluster, use addNet may causing some unexpected result.\n";}
        NetSet.insert({net->NetId,1});netNum++;
    }
};


void InitNets(std::vector<Cluster*>&cellVec,std::vector<Net*>&nets);

// <from part,to part>
inline std::pair<int,int> GroupNum(const Net& net,const Cell* cell){
    // return cell->group1? std::pair<int,int>{net.group1,net.group2} : std::pair<int,int>{net.group2,net.group1};

    int gp1 = 0;
    int gp2 = 0;

    for(auto c:net.cells)
    {
        
    }

}

inline int alphabetical_order(int gain1,int gain2,int id1,int id2){
    if(gain1 > gain2)return id1;
    if(gain1 < gain2)return id2;
    return (id1 < id2) ? id1:id2;
}

using ties = decltype(alphabetical_order);

struct Bucket{
    using CellId = int;//pesudo id.
    std::vector<std::list<CellId>>gainVec;
    int maxGain;   
    int maxPin;
    std::pair<int,int> front(ties* tie = nullptr);//return <max-gain cellID,gain>
    void erase(Cell*cell);
    void push_front(Cell*cell);
};

std::pair<int,int> onePass(std::vector<Cluster*>&cellVec,std::pair<Bucket*,Bucket*>buckets,\
    std::pair<float,float>ratios,std::pair<int*,int*>groups,ties *tie = nullptr);

void FM(std::vector<Cluster*>&cellVec,std::vector<Net*>netlist,float ratio1,float ratio2,int phase,ties*tie = nullptr);

int CutSize(std::vector<Net*>&net);


bool Decluster(std::queue<int>&declusterQ,std::vector<Cluster*>&cellVec,std::vector<Net*>&netlist,int phase);

#endif
