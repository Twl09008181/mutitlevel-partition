
#ifndef FM_HPP
#define FM_HPP
#include <list>
#include <utility>
#include <vector>

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
    void addNet(Net*net){nets.push_front(net);netNum++;}

    int gain = 0;
    bool freeze = true;
    std::list<int>::iterator it;//to access gain bucket.
};//using std::vector<Cell> to save all Cells.   

//First step is to build CellArray (sorted by id),and save sortId.
//need set group1 flag by any way.
//set nets

//Second step is to init the Net objects, saving pesudo id and group1,group2 by scan all cells.


// <from part,to part>
inline std::pair<int,int> GroupNum(const Net& net,const Cell& cell){
    return cell.group1? std::pair<int,int>{net.group1,net.group2} : std::pair<int,int>{net.group2,net.group1};
}


struct Bucket{
    using CellId = int;//pesudo id.
    std::vector<std::list<CellId>>gainVec;
    int maxGain;   
    int maxPin;
    std::pair<int,int> front();//return <max-gain cellID,gain>
    void pop_front();
    void erase(Cell&cell);
    void push_front(Cell&cell);
};

void FM(std::vector<Cell>&cellVec,float ratio1,float ratio2);



#endif
