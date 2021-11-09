
#ifndef FM_HPP
#define FM_HPP
#include <list>
#include <utility>
#include <vector>
#include <set>
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
    bool group1;
    std::list<Net*>nets;
    int netNum = 0;
    virtual void addNet(Net*net){nets.push_front(net);netNum++;}


    // bucket relative
    int gain = 0;
    bool freeze = true;
    std::list<int>::iterator it;//to access gain bucket.


    virtual std::list<Net*>getNetlist(){return nets;}
    virtual bool isValid(){return true;}
    virtual int getSize()const{return 1;}  //bug 1 ........................
};//using std::vector<Cell> to save all Cells.   




//a specialed cell , can be used in coarsening and uncoarsening
class Cluster : public Cell{
public:
    Cluster(int realid){
        id = realid;
        // cluster relative
        cells.push_front(this);
        valid = true;
        iscluster = false;
        clusterId = sortId = -1;
        cellsNum = 1;
    }
    void setSortId(int sortedId){
        clusterId = sortId = sortedId;
    }
    bool is_master(){
        return sortId==clusterId;
    }
    // cluster relative member
    int clusterId;
    bool valid;//cluster or non-cluster single valid cell. 
    bool iscluster;//decluster後set false,cluster後 set true.
    std::list<Cluster*>cells;//紀錄存了哪些cell,decluster需要
    int originNetNum = 0;//decluster時會用到

    int cellsNum;//maintain for quickly get size.

    bool is_cluster(){return iscluster;}//only one cell.
    // update cluster member
    int clustering(Cluster*v);//return id
    void decluster();



    // after all clustering is done
    // put nets in clustersNetSet into clustersNets
    void BuildClustersNets();


    // inherint functions
    int getSize()const{return cellsNum;}

    bool isValid(){
        if(is_master()&&!valid){std::cerr<<"isValid warning, a cluster ,master is always valid,you may need to check if you have already call BuildClustersNets\n";}
        return valid;
    }
    void addNet(Net*net){
        if(iscluster){std::cerr<<"warrning,this cell is already a cluster, use addNet may causing some unexpected result.\n";}
        nets.push_front(net);
        clustersNetSet.insert(net);
        netNum++;
    }
    std::list<Net*>getNetlist(){
        if(is_master())
            return (iscluster) ? clustersNets : nets;
        else{
            std::cerr<<"netNelist warning, "<<sortId<<" is not a cluster master\n";
            return nets;
        }
    }
private:
    std::set<Net*>clustersNetSet;
    std::list<Net*>clustersNets;//FM需要整個Nets
};

struct clusterCmp
{
    bool operator()(const Cluster*c1,const Cluster*c2)const{
        return c1->getSize() < c2->getSize();
    }
};
using dClusterQ = std::priority_queue<Cluster*,std::vector<Cluster*>,clusterCmp>;
dClusterQ getdClusterQ(std::vector<Cluster*>&cellVec);
std::vector<int>get_dclusterID(dClusterQ &q,int size );

void InitNets(std::vector<Cluster*>&cellVec,std::list<Net*>&nets);

// <from part,to part>
inline std::pair<int,int> GroupNum(const Net& net,const Cell* cell){
    return cell->group1? std::pair<int,int>{net.group1,net.group2} : std::pair<int,int>{net.group2,net.group1};
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

void FM(std::vector<Cluster*>&cellVec,std::list<Net*>netlist,float ratio1,float ratio2,ties*tie = nullptr);

int CutSize(std::list<Net*>&net);

#endif
