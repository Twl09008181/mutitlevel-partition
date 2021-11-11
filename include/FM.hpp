
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
    
    //Core attributes. 

    // 1. Size : need maintain for quickly check ratios
    int group1 = 0;
    int group2 = 0;

    // 2. Number  : need maintain for quickly cacluating gains.
    int group1Num = 0;
    int group2Num = 0;

    //This two attributes are core of cluster&FM
    //Be maintained in FM procedure using move()/recoverStage , and update in Cluster&deCluster using initNets().
   
    //for exampel  if clusterId = 0, have cells : 0,1,2
    //assume this net only contain cell 0 and cell 0 now is in group1,
    //then group1 = 3   , group2 = 0   for view of size. 
    //then group1Num = "1" , group2Num = 0  for view of number.

    void updateGroup(int size,bool From1To2){
        if(From1To2){
            group1 -= size;
            group2 += size;
            group1Num --;
            group2Num ++;
        }
        else{
            group1 += size;
            group2 -= size;
            group1Num ++;
            group2Num --;
        }
    }
    void addCells(Cell*cell);
};

struct Cell{
    int id;//real inputId
    int sortId;//sorted in CellVec id
    bool group1 = false;
    int netNum = 0;
    std::map<int,int>NetSet;//net id , second is for cluster
    virtual void addNet(Net*net){NetSet.insert({net->NetId,1});netNum++;}
    // bucket relative
    int gain = 0;    //used to get position in gainBucket
    bool freeze = true; //if freeze , it can't be in gainBucket
    std::list<int>::iterator it;//to quickly access gain bucket list.
    std::map<int,int>& getNets(){return NetSet;}
    virtual bool isValid(){return true;}
    virtual int getSize()const{return 1;}  
};//Fm need std::vector<Cell> to save all Cells.   




//a specialed cell , can be used in coarsening and uncoarsening
class Cluster : public Cell{
public:
    Cluster(int realid){
        id = realid;
        // cluster relative attributes.
        cells.push_front(this);
        valid = true;
        iscluster = false;
        clusterId = sortId = -1;
        cellsNum = 1;
    }


    //Must initial by setSortID.
    void setSortId(int sortedId){clusterId = sortId = sortedId;}//need sorted to get sortId,clusterId

    //Flag
    bool is_master(){return sortId==clusterId;}
    bool is_cluster(){return cellsNum > 1;}//only one cell is not a cluster.

    // cluster relative member
    int clusterId;
    bool valid;//cluster or non-cluster single valid cell. 
    bool iscluster;
    std::list<Cluster*>cells;// recored for decluster
    int originNetNum = 0;// recored for decluster
    int cellsNum;// maintain for quickly get size.
    int clusterPhase = -1;// if clusterPhase!=-1, this cluster cell is clustered by another cell in phase = clusterPhase.
    

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

// break tie example using  alphabetical_order
inline int alphabetical_order(int gain1,int gain2,int id1,int id2){
    if(gain1 > gain2)return id1;
    if(gain1 < gain2)return id2;
    return (id1 < id2) ? id1:id2;
}

using ties = decltype(alphabetical_order);

struct Bucket{
    using CellId = int;//pesudo id (sortId)
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


// return remain vertex num , total phase
// Now support Edge coarseningt
std::pair<int,int> Coarsen(std::vector<Cluster*>&cellVec,std::vector<Net*>&netlist);

bool Decluster(std::queue<int>&declusterQ,std::vector<Cluster*>&cellVec,std::vector<Net*>&netlist,int phase);

#endif
