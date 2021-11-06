#include "../include/FM.hpp"

#include <vector>
#include <iostream>
#include <map>
#include <utility>

#ifdef DEBUG
extern std::map<int,char>dict;
#endif









// used to move cellId in gainbucket.
void Bucket:: erase(Cell&cell){
    if(cell.freeze){// if cell is already freeze,then it can't be in gainbucket.
        std::cerr<<"carefull,cell.freeze = true";
        return ;
    }
    auto &lst = gainVec.at(cell.gain + maxPin);
    lst.erase(cell.it);
    cell.freeze = true;
    if(cell.gain == maxGain){//update max gain
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

void Bucket::push_front(Cell&cell)
{
    int pos = maxPin + cell.gain; // min gain = -maxPin
    if(pos < 0 || pos > 2 * maxPin){
        std::cerr<<"gain range error , input gain:"<<cell.gain<< " min gain : "<< - maxPin << " max gain : "<<maxPin<<"\n";
        exit(1);
    }
    gainVec.at(pos).push_front(cell.sortId);
    cell.it = gainVec.at(pos).begin();     //saving linked-list iterator
    cell.freeze = false;
    maxGain = std::max(maxGain,cell.gain); //update maxGain of bucket
}

// O(maxPin) construct gainBucket
void initGainBucket(std::vector<Cell>&cellVec,Bucket&b1,Bucket&b2)
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
    for(auto &cell:cellVec){
        int gain = 0;
        for(auto net : cell.nets){
            auto From_To_Num = GroupNum(*net,cell);
            if(From_To_Num.first==1) gain++;  //From set == 1 
            if(From_To_Num.second==0)gain--; //To set == 0
        }
        cell.gain = gain;
        cell.group1 ? b1.push_front(cell) : b2.push_front(cell);
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


inline int group1Num(std::vector<Cell>&cellVec){
    int n = 0;
    for(auto &cell:cellVec)
        if(cell.group1)n++;
    return n;
}


void InitNets(std::vector<Cell>&cellVec,std::list<Net*>&nets){
    for(auto n:nets)
        n->group1 = n->group2 = 0;
    for(auto &cell : cellVec){
        for(auto net : cell.nets){
            net->cells.push_back(cell.sortId);
            cell.group1 ? (net->group1++):(net->group2++);
        }
    }
}

// update cells in same netlist of moving cell.
void update(std::vector<Cell>&cellVec,Net*net,Cell*movingcell,std::pair<Bucket*,Bucket*>&buckets,bool group1,bool increment,bool onlyOne){
    for(auto cellId:net->cells){
        Cell& cell = cellVec.at(cellId);// get cell        
        if(group1 == cell.group1){ // the specified group.
            if(cell.freeze){
                if(onlyOne&& &cell != movingcell)break;
                else continue;
            }
            Bucket * b = nullptr;
            cell.group1 ? (b = buckets.first) : (b = buckets.second);// find the bucket
            b->erase(cell); // erase 
            increment ? cell.gain++ : cell.gain--;
            b->push_front(cell); // re-push
            if(onlyOne)break;// do not need to check other cell.
        }
    }
}

void move(std::vector<Cell>&cellVec,int cellId,std::pair<Bucket*,Bucket*>&buckets){
    Cell& cell = cellVec.at(cellId);
    cell.group1 ? (buckets.first->erase(cell)):(buckets.second->erase(cell));
    bool from_group = cell.group1;
    for(auto net:cell.nets){
        auto From_To_Num = GroupNum(*net,cell);    
        int To = From_To_Num.second;
        if(To == 0) 
            update(cellVec,net,&cell,buckets,from_group,true,false);//increment "from group" cells's gain. 
        else if (To == 1) // if to == 1
            update(cellVec,net,&cell,buckets,!from_group,false,true);//decrement "to group" cell's gain. 
        //assume move
        int From = From_To_Num.first - 1;
        if(From == 0) // if From ==0
            update(cellVec,net,&cell,buckets,!from_group,false,false);//decrement "to group" cells's gain. 
        else if (From == 1) // if From == 1
            update(cellVec,net,&cell,buckets,from_group,true,true);//increment "from group" cell's gain. 
        //update net's group num.
        if(cell.group1){
            net->group1--;
            net->group2++;
        }else{
            net->group1++;
            net->group2--;
        }
    }
    //really move
    cell.group1 = !cell.group1;
}


inline bool ratioPrecheck(const std::pair<float,float>&ratios,const std::pair<int*,int*>&groups){
    float r = (float) (*groups.first) / (*groups.first + *groups.second);

    // std::cout<<"gp1:"<<*groups.first<<" gp2: "<<*groups.second<<"\n";
    return (r >= ratios.first && r <= ratios.second);
}


std::pair<std::pair<int,int>,std::pair<int,int>> getCandidate(std::pair<Bucket*,Bucket*>buckets,std::pair<float,float>ratios,\
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

//first : move id
//second : gain
std::pair<int,int> onePass(std::vector<Cell>&cellVec,std::pair<Bucket*,Bucket*>buckets,\
    std::pair<float,float>ratios,std::pair<int*,int*>groups,ties *tie){
    //pre-check current ratio
    if(!ratioPrecheck(ratios,groups)){
        std::cerr<<"error in onePass, error ratio state! , you must have any  valid init solution.\n";
        exit(1);
    }
    auto candidates = getCandidate(buckets,ratios,groups,tie);// get
    if(candidates.first.first==-1 && candidates.second.first==-1)return candidates.first;// no cell can move

    int c1 = candidates.first.first    ,    c2 = candidates.second.first;
    int gain1 = candidates.first.second, gain2 = candidates.second.second;
    bool moveGp1 = tie ? (tie(gain1,gain2,c1,c2) == c1): (gain1 >= gain2); // choose cell to move
    if(moveGp1){
        move(cellVec,c1,buckets);
        *groups.first -=1;
        *groups.second+=1;
        return {c1,gain1};
    }else{
        move(cellVec,c2,buckets);
        *groups.first +=1;
        *groups.second-=1;
        return {c2,gain2};
    }
}

int getBestStage(std::vector<int>&gainAcc,std::vector<float>&ratioRecord,int iterations){
    int maxGain = gainAcc.at(0);
    std::vector<int>candidates;candidates.push_back(0);
    for(int i = 0;i < iterations;i++){
        if(gainAcc.at(candidates.at(0)) < gainAcc.at(i)){ // new best
            candidates.clear();
            candidates.push_back(i);
        }else if (gainAcc.at(candidates.at(0)) == gainAcc.at(i)){// add candidate
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


void RecoverToStage(std::vector<Cell>&cellVec,std::vector<int>&moveRecord,int bestIteration,int totalIteration,int&g1Num,int&g2Num){
    for(int k = totalIteration-1; k > bestIteration;k--){
        int CellId = moveRecord.at(k);
        auto &cell = cellVec.at(CellId);
        if(cell.group1){//now in group1, recover to group2
            g1Num--;g2Num++;
            for(auto net:cell.nets){
                net->group1--;net->group2++;
            }
        }else{
            g1Num++;g2Num--;
            for(auto net:cell.nets){
                net->group1++;net->group2--;
            }
        }
        cell.group1 = !cell.group1;
    }
}


void FM(std::vector<Cell>&cellVec,float ratio1,float ratio2,ties*tie)
{
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
        int bestIteration = getBestStage(gainAcc,ratioRecord,k);
        maxGain = gainAcc.at(bestIteration);
        // Recover state to bestIteration.
        RecoverToStage(cellVec,moveRecord,bestIteration,k,g1Num,g2Num);
        std::cout<<"maxGain:"<<maxGain<<"\n";
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
