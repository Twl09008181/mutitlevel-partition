#include "../include/FM.hpp"

#include <vector>
#include <iostream>
#include <map>
#include <utility>

#ifdef DEBUG
extern std::map<int,char>dict;
#endif









// used to move cellId in gainbucket.
void Bucket:: erase(Cell*cell){
    if(cell->freeze){// if cell is already freeze,then it can't be in gainbucket.
        std::cerr<<"carefull,cell.freeze = true";
        return ;
    }
    auto &lst = gainVec.at(cell->gain + maxPin);
    lst.erase(cell->it);
    cell->freeze = true;
    if(cell->gain == maxGain){//update max gain
        int idx;
        for(idx = maxGain + maxPin; idx >=0 && gainVec.at(idx).empty() ;idx--);
        maxGain = std::max(idx - maxPin,-maxPin);
    }
}

// return <id,gain>
std::pair<int,int> Bucket::front(ties *tie){
   
    auto &lst = gainVec.at(maxGain + maxPin);
   
    if(maxGain==-maxPin && lst.empty())
        return {-1,-1};//no cell can move
    else if (lst.empty()){
        std::cerr<<"Bucket::front error, maxGain:"<<maxGain<<" but empty\n";
        exit(1);
    }

    // if any rules for picking cell,then call tie , otherwise,return front.
    if(!tie)
        return {lst.front(),maxGain};
    else {
        std::list<int>::iterator ptr = lst.begin();
        int cellId = *ptr;
        for(;ptr!=lst.end();++ptr)cellId = tie(maxGain,maxGain,cellId,*ptr);
        return {cellId,maxGain};
    }
}

void Bucket::push_front(Cell*cell)
{
    int pos = maxPin + cell->gain; // min gain = -maxPin
    if(pos < 0 || pos > 2 * maxPin){
        std::cerr<<"gain range error , input gain:"<<cell->gain<< " min gain : "<< - maxPin << " max gain : "<<maxPin<<"\n";
        exit(1);
    }
    gainVec.at(pos).push_front(cell->sortId);
    cell->it = gainVec.at(pos).begin();     //saving linked-list iterator
    cell->freeze = false;
    maxGain = std::max(maxGain,cell->gain); //update maxGain of bucket
}

// O(maxPin) construct gainBucket
void initGainBucket(std::vector<Cluster>&cellVec,Bucket&b1,Bucket&b2)
{
    b1.gainVec.clear();b2.gainVec.clear();

    // Allocate
    static int maxPin = 0;
    if(maxPin == 0 )
    for(auto &cell:cellVec) maxPin = std::max(cell.netNum,maxPin);

    int size = maxPin * 2 + 1;
    if(b1.gainVec.size()!= size)b1.gainVec.resize(size);// index : [0,2*maxPin] , gain: [-maxPin,maxPin]
    if(b2.gainVec.size()!= size)b2.gainVec.resize(size);

    b1.maxGain = b2.maxGain = -maxPin;
    b1.maxPin  = b2.maxPin  = maxPin;

    // Build
    for(auto & ct :cellVec){
        Cell*cell = &ct; 
        if(!cell->isValid())continue;
        int gain = 0;
        for(auto net : cell->getNetlist()){
            auto From_To_Num = GroupNum(*net,cell);
            if(From_To_Num.first==1) gain++;  //From set == 1 
            if(From_To_Num.second==0)gain--; //To set == 0
        }
        cell->gain = gain;
        cell->group1 ? b1.push_front(cell) : b2.push_front(cell);
    }
}

void showGainBucket(Bucket*b,std::vector<Cell>&cellVec)
{
    for(int i = 2 * b->maxPin; i>=0;i--)
    {
        std::cout<<"gain:"<<i - b->maxPin <<" list:";
        for(auto id:b->gainVec.at(i))
        {
            #ifdef DEBUG
            std::cout<<dict[cellVec.at(id).id]<<" ";
            #endif

            #ifndef DEBUG
            std::cout<<id<<" ";
            #endif
        }   
        std::cout<<"\n";
    }
}


inline int group1Num(std::vector<Cluster>&cellVec){
    int n = 0;
    for(auto &cell:cellVec)
        if(cell.group1)
            n += cell.getSize();
    return n;
}

void Net::addCells(Cell*cell){
    cells.push_front(cell->sortId);
    cell->group1 ? (group1+=cell->getSize()) : (group2+=cell->getSize());
}
void InitNets(std::vector<Cluster*>&cellVec,std::list<Net*>&nets){
    // clear all
    for(auto n:nets){
        n->group1 = n->group2 = 0;
        n->cells.clear();
    }
    // build connect
    for(auto cell : cellVec){
        if(cell->isValid())
        for(auto net : cell->getNetlist())
            net->addCells(cell);
    }
}

// update cells in same netlist of moving cell.
void update(std::vector<Cluster>&cellVec,Net*net,Cell*movingcell,std::pair<Bucket*,Bucket*>&buckets,bool group1,bool increment,bool onlyOne){
    for(auto cellId:net->cells){
        Cell* cell = &cellVec.at(cellId);// get cell        
        if(group1 == cell->group1){ // the specified group.
            if(cell->freeze){
                if(onlyOne&& cell != movingcell)break;
                else continue;
            }
            Bucket * b = nullptr;
            cell->group1 ? (b = buckets.first) : (b = buckets.second);// find the bucket
            b->erase(cell); // erase 
            increment ? cell->gain++ : cell->gain--;
            b->push_front(cell); // re-push
            if(onlyOne)break;// do not need to check other cell.
        }
    }
}

void move(std::vector<Cluster>&cellVec,int cellId,std::pair<Bucket*,Bucket*>&buckets){
    Cell* cell = &cellVec.at(cellId);
    cell->group1 ? (buckets.first->erase(cell)):(buckets.second->erase(cell));
    bool from_group = cell->group1;
    for(auto net : cell->getNetlist()){
        auto From_To_Num = GroupNum(*net,cell);    
        int To = From_To_Num.second;
        if(To == 0) 
            update(cellVec,net,cell,buckets,from_group,true,false);//increment "from group" cells's gain. 
        else if (To == 1) // if to == 1
            update(cellVec,net,cell,buckets,!from_group,false,true);//decrement "to group" cell's gain. 
        //assume move
        int From = From_To_Num.first - 1;
        if(From == 0) // if From ==0
            update(cellVec,net,cell,buckets,!from_group,false,false);//decrement "to group" cells's gain. 
        else if (From == 1) // if From == 1
            update(cellVec,net,cell,buckets,from_group,true,true);//increment "from group" cell's gain. 
        //update net's group num.
        if(cell->group1){
            net->group1 -= cell->getSize();
            net->group2 += cell->getSize();
        }else{
            net->group1 += cell->getSize();
            net->group2 -= cell->getSize();
        }
    }
    //really move
    cell->group1 = !cell->group1;
}





std::pair<std::pair<int,int>,std::pair<int,int>> validStatePick(std::pair<Bucket*,Bucket*>buckets,std::pair<float,float>ratios,\
    std::pair<int*,int*>groups,ties *tie){

    float r1 = (float) (*groups.first - 1) / (*groups.first + *groups.second); // if move cell in partition1 to 2
    float r2 = (float) (*groups.first + 1) / (*groups.first + *groups.second); // if move cell in partition2 to 1

    Bucket *b1 = buckets.first;
    Bucket *b2 = buckets.second;
    auto c1 = b1->front(tie);//<id,gain>
    auto c2 = b2->front(tie);
    if(c1.first==-1 && c2.first==-1){return {{-1,0},{-1,0}};}
    if(c1.first==-1 && r2 > ratios.second){return {{-1,1},{-1,1}};} //只能動c2,但partition 1 已經太大
    if(c2.first==-1 && r1  < ratios.first){return {{-1,2},{-1,2}};}//只能動c1,但partition 1 已經太小
    if(r1 < ratios.first && r2 > ratios.second)return {{-1,3},{-1,3}};
    

    int gain1 = (c1.first!=-1 && r1 >= ratios.first) ? c1.second : -INT_MAX;
    int gain2 = (c2.first!=-1 && r2 <= ratios.second) ? c2.second : -INT_MAX;
    return {{c1.first,gain1},{c2.first,gain2}};
}



std::pair<std::pair<int,int>,std::pair<int,int>> getCandidate(std::pair<Bucket*,Bucket*>buckets,std::pair<float,float>ratios,\
    std::pair<int*,int*>groups,ties *tie){

    float r =  (float) (*groups.first) / (*groups.first + *groups.second);//ratio now

    // if this state is an invalid state.
    if(r < ratios.first || r > ratios.second) {
        Bucket *b = (r < ratios.first) ? buckets.second : buckets.first;
        auto c = b->front(tie); //recommend size 到時候新增,但為了簡單 只檢查一個range的gain max 滿足該size的
        if(c.first==-1){return {{-1,4},{-1,4}};}
        if(r < ratios.first)
            return {{-1,-INT_MAX},{c.first,c.second}} ;
        else 
            return {{c.first,c.second},{-1,-INT_MAX}};;
    }   

    // valid state
    return validStatePick(buckets,ratios,groups,tie);
}

//first : move id
//second : gain
std::pair<int,int> onePass(std::vector<Cluster>&cellVec,std::pair<Bucket*,Bucket*>buckets,\
    std::pair<float,float>ratios,std::pair<int*,int*>groups,ties *tie){
    auto candidates = getCandidate(buckets,ratios,groups,tie);// get
    if(candidates.first.first==-1 && candidates.second.first==-1)return candidates.first;// no cell can move

    int c1 = candidates.first.first    ,    c2 = candidates.second.first;
    int gain1 = candidates.first.second, gain2 = candidates.second.second;
    bool moveGp1 = tie ? (tie(gain1,gain2,c1,c2) == c1): (gain1 >= gain2); // choose cell to move
    if(moveGp1){
        move(cellVec,c1,buckets);
        *groups.first  -= cellVec.at(c1).getSize();
        *groups.second += cellVec.at(c1).getSize();
        return {c1,gain1};
    }else{
        move(cellVec,c2,buckets);
        *groups.first  += cellVec.at(c2).getSize();
        *groups.second -= cellVec.at(c2).getSize();
        return {c2,gain2};
    }
}

int getBestStage(std::vector<int>&gainAcc,std::vector<float>&ratioRecord,int iterations,std::pair<float,float>ratios){
    int maxGain = gainAcc.at(0);
    auto validRatio = [&ratioRecord,&ratios](int i){return ratioRecord.at(i) >= ratios.first && ratioRecord.at(i) <= ratios.second;};
    std::vector<int>candidates;

    int bestGain = -INT_MAX;
    for(int i = 0;i < iterations;i++){
        if(!validRatio(i))continue;//not valid ratio

        if(bestGain < gainAcc.at(i)){ // new best
            candidates.clear();
            candidates.push_back(i);
            bestGain  = gainAcc.at(i);
        }else if (bestGain == gainAcc.at(i)){// add candidate
            candidates.push_back(i);
        }
    }
    //find most close to half .
    float close_half = ratioRecord.at(candidates.at(0)) - 0.5;
    if(close_half < 0) close_half *=-1;
    int best = candidates.at(0);
    for(auto c : candidates){
        float diff_half = ratioRecord.at(c) - 0.5;
        if(diff_half < 0)diff_half*=-1;
        if(diff_half < close_half){
            close_half = diff_half;
            best = c;
        }
    }
    return best;
}


void RecoverToStage(std::vector<Cluster>&cellVec,std::vector<int>&moveRecord,int bestIteration,int totalIteration,int&g1Num,int&g2Num){
    for(int k = totalIteration-1; k > bestIteration;k--){
        int CellId = moveRecord.at(k);
        auto &cell = cellVec.at(CellId);
        int size = cell.getSize();
        if(cell.group1){//now in group1, recover to group2
            g1Num-=size;g2Num+=size;
            for(auto net:cell.getNetlist()){
                net->group1-=size;net->group2+=size;
            }
        }else{
            g1Num+=size;g2Num-=size;
            for(auto net:cell.getNetlist()){
                net->group1+=size;net->group2-=size;
            }
        }
        cell.group1 = !cell.group1;
    }
}


void FM(std::vector<Cluster>&cellVec,std::list<Net*>netlist,float ratio1,float ratio2,ties*tie)
{
    int totalNum = cellVec.size();

    int half1 = totalNum - totalNum/2;
    if(float(half1)/totalNum < ratio1 || float(half1)/totalNum > ratio2){
        std::cerr<<"your cell size can never reach the request ratio,please enter new ratio constraint.\n";
        exit(1);
    }


    int g1Num = group1Num(cellVec);
    int g2Num = cellVec.size() - g1Num;
    int maxGain;
    std::vector<int>gainAcc(cellVec.size(),-1);//accumulative gain 
    std::vector<int>moveRecord(cellVec.size(),-1);
    std::vector<float>ratioRecord(cellVec.size(),-1);

    do{
        Bucket b1,b2;
        initGainBucket(cellVec,b1,b2);
        int k = 0,gain = 0;
        for(; k < cellVec.size() ; k++){
            auto pass = onePass(cellVec,{&b1,&b2},{ratio1,ratio2},{&g1Num,&g2Num},tie);
            if(pass.first == -1)break; // no cell can move.
            gain += pass.second;
            gainAcc.at(k) = gain;
            moveRecord.at(k) = pass.first;
            ratioRecord.at(k) = (float) (g1Num) / (g1Num + g2Num);
        }
        // Pick best state 
        int bestIteration = getBestStage(gainAcc,ratioRecord,k,{ratio1,ratio2});
        maxGain = gainAcc.at(bestIteration);
        // Recover state to bestIteration.
        RecoverToStage(cellVec,moveRecord,bestIteration,k,g1Num,g2Num);

        //decluster();
    }while(maxGain > 0);

}
int CutSize(std::list<Net*>&net){
    int cut = 0;
    for(auto n:net){
        if(n->group1&&n->group2)
            cut++;
    }
    return cut;
}



int Cluster::clustering(Cluster*v){

    if(v==nullptr||!v->is_master()){
        std::cerr<<"warning in clustering, input v is invalid\n";
        return this->clusterId;
    }if(v->clusterId==this->clusterId)return this->clusterId;
    
    if(!this->is_master()){
        std::cerr<<"warning in clustering," <<sortId <<" is invalid,please use master in this cluster to cluster another vertex,master id :"<<this->clusterId<<"\n";
        return this->clusterId;
    }

    //flag setting
    v->clusterId = this->clusterId;
    this->valid = v->valid = false;
    v->iscluster = false;this->iscluster = true;

    // cluster
    for(auto c:v->cells)
        this->cells.push_back(c);

    //set1 is bigger than set2
    auto &set1 = (netNum >= v->netNum) ? this->clustersNetSet :v->clustersNetSet;
    auto &set2 = (netNum >= v->netNum) ? v->clustersNetSet : this->clustersNetSet;
    
    for(auto net:set2)
        if(set1.find(net)==set1.end())
            set1.insert(net);

    if(&clustersNetSet != &set1)  //carefully
        this->clustersNetSet = std::move(set1);
    //not necessary op
    v->clustersNetSet.clear();
    v->clustersNets.clear();
    return this->clusterId;
}

void Cluster::BuildClustersNets(){  
    if(iscluster){
        netNum = 0;
        clustersNets.clear();
        for(auto net:clustersNetSet){
            clustersNets.push_front(net);
            netNum++;
        }
        valid = true;
    }else{
        std::cerr<<"warning !  \
        only cluster cells need call void BuildClustersNets()\n";
    }
}

void Cluster::decluster(){
    if(iscluster){
        clustersNetSet.clear();
        clustersNets.clear();
        for(auto c : cells){
            c->valid = true;
            c->iscluster = false;
            c->clusterId = c->sortId;
            c->netNum = c->originNetNum;
            c->group1 = this->group1; // important 
            for(auto net:c->nets)  //not necessary,but for safe.
                c->clustersNetSet.insert(net);
            
        }
    }else{
        std::cerr<<"void Cluster::decluster() warning, cell "<<sortId<<" is not a cluster\n";
    }
}
