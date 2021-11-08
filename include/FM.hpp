
#ifndef FM_HPP
#define FM_HPP
#include <list>
#include <utility>
#include <vector>
#include <set>
#include <iostream>
struct Net{

    Net(int id)
        :NetId{id}{}
    int NetId;
    using CellId = int;//pesudo id (sort id)
    std::list<CellId> cells;

    //need maintain
    int group1 = 0;
    int group2 = 0;
};
struct Cell{
    int id;//編號不一定會是從1~node的總數
    int sortId;
    bool group1;
    std::list<Net*>nets;
    int netNum = 0;
    virtual void addNet(Net*net){nets.push_front(net);netNum++;}

    int gain = 0;
    bool freeze = true;
    std::list<int>::iterator it;//to access gain bucket.

    virtual int getSize(){return 1;}
};//using std::vector<Cell> to save all Cells.   

//First step is to build CellArray (sorted by id),and save sortId.
//need set group1 flag by any way.
//set nets

//Second step is to init the Net objects, saving pesudo id and group1,group2 by scan all cells.



using netInfo = std::pair<Net*,std::list<int>::iterator>;
struct netCmp {
    bool operator()(const netInfo& lhs, const netInfo& rhs) const { 
        return lhs.first->NetId < rhs.first->NetId;
    }
};

class cluster : public Cell{
public:
    std::vector<Cell*>cells;
    std::set<netInfo,netCmp>nets;
    void clustering(cluster&v);
    void decluster();
    int getSize(){return cells.size();}
    void addNet(Net*net){std::cout<<"cluster doesn't support addNet\n";}//do nothing
};




void InitNets(std::vector<Cell>&cellVec,std::list<Net*>&nets);

// <from part,to part>
inline std::pair<int,int> GroupNum(const Net& net,const Cell& cell){
    return cell.group1? std::pair<int,int>{net.group1,net.group2} : std::pair<int,int>{net.group2,net.group1};
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
    void erase(Cell&cell);
    void push_front(Cell&cell);
};

std::pair<int,int> onePass(std::vector<Cell>&cellVec,std::pair<Bucket*,Bucket*>buckets,\
    std::pair<float,float>ratios,std::pair<int*,int*>groups,ties *tie = nullptr);

void FM(std::vector<Cell>&cellVec,float ratio1,float ratio2,ties*tie = nullptr);

int CutSize(std::list<Net*>&net);

#endif
