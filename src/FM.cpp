#include "../include/FM.hpp"

#include <vector>
#include <iostream>
#include <map>
#include <utility>

#ifdef DEBUG
extern std::map<int,char>dict;
#endif

void Bucket:: erase(Cell&cell)
{
    auto &lst = gainVec.at(cell.gain + maxPin);
    lst.erase(cell.it);
    cell.freeze = true;
}

std::pair<int,int> Bucket::front(){
    auto &lst = gainVec.at(maxGain + maxPin);//if no rules for solving ties, return first
    if(maxGain==-maxPin && lst.empty())
        return {-1,-1};//no cell can move
    else if (lst.empty())
    {
        std::cerr<<"Bucket::front error, maxGain:"<<maxGain<<" but empty\n";
        exit(1);
    }
    return {lst.front(),maxPin};//normal
}
void Bucket::pop_front(){
    auto &lst = gainVec.at(maxGain + maxPin);

    if(!lst.empty())lst.pop_front();

    for(int idx = maxGain + maxPin; idx >=0 ;idx--)
    {
        if(!gainVec.at(idx).empty())//update
            maxGain = idx + maxPin;
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
    cell.it = gainVec.at(pos).begin();
    cell.freeze = false;
    maxGain = std::max(maxGain,cell.gain); //update
}

void initGainBucket(std::vector<Cell>&cellVec,Bucket&b1,Bucket&b2)
{
    int maxPin = 0;
    for(auto &cell:cellVec) maxPin = std::max(cell.netNum,maxPin);
    b1.gainVec.clear();b2.gainVec.clear();
    b1.gainVec.resize(maxPin * 2 + 1);// index : [0,2*maxPin] , gain: [-maxPin,maxPin]
    b2.gainVec.resize(maxPin * 2 + 1);
    b1.maxGain = b2.maxGain = -maxPin;
    b1.maxPin = b2.maxPin = maxPin;
    for(auto &cell:cellVec)
    {
        int gain = 0;
        for(auto net:cell.nets)
        {
            auto From_To_Num = GroupNum(*net,cell);
            if(From_To_Num.first==1)gain++;  //From set == 1 
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
        std::cout<<"gain:"<<i - b->maxGain <<" list:";
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


void update(std::vector<Cell>&cellVec,Net*net,Bucket*b,bool group1,int val = 1){
    for(auto cellId:net->cells)
    {
        Cell& cell = cellVec.at(cellId);
        if(cell.freeze)continue;

        if(group1==cell.group1){//指定的group
            b->erase(cell);
            cell.gain += val;
            b->push_front(cell);
        }
    }
}

void move(std::vector<Cell>&cellVec,int cellId,Bucket*b)
{
    Cell& cell = cellVec.at(cellId);
    for(auto net:cell.nets)
    {
        auto From_To_Num = GroupNum(*net,cell);    

        int To = From_To_Num.second;
        if(To==0) // if to == 0
            update(cellVec,net,b,cell.group1,1);//update same group cells
        else if (To==1) // if to == 1
            update(cellVec,net,b,!cell.group1,-1);//update differnt group cells
        
        int From = From_To_Num.first - 1;

        //assume after move 
        if(From == 0) // if From ==0
            update(cellVec,net,b,!cell.group1,-1);//update To group cells
        else if (From==1) // if From == 1
            update(cellVec,net,b,cell.group1,1);//update From group cell.

        //update net's group num.
        if(cell.group1)
        {
            net->group1--;
            net->group2++;
        }
        else{
            net->group1++;
            net->group2--;
        }
    }
    //really move
    cell.group1 = !cell.group1;
}


//first : move id
//second : gain
std::pair<int,int> onePass(std::vector<Cell>&cellVec,std::pair<Bucket*,Bucket*>buckets,\
    std::pair<float,float>ratios,std::pair<int*,int*>groups)
{
    //pre-check
    float r = (float) (*groups.first) / (*groups.first + *groups.second);
    if(r < ratios.first || r > ratios.second)
    {
        std::cerr<<"error in onePass, error ratio state!\n";
        exit(1);
    }
    // the ratio of partition1 after two differnt move
    float r1 = (float) (*groups.first - 1) / (*groups.first + *groups.second); // if move cell in partition1 to 2
    float r2 = (float) (*groups.first + 1) / (*groups.first + *groups.second); // if move cell in partition2 to 1
    Bucket *b1 = buckets.first;
    Bucket *b2 = buckets.second;

    auto c1 = b1->front();//<id,gain>
    auto c2 = b2->front();
    //can't move
    if(c1.first==-1 && c2.first==-1){return {-1,0};}
    if(c1.first==-1 && r2 > ratios.second){return {-1,0};} //只能動c2,但partition 1 已經太大
    if(c2.first==-1 && r1  < ratios.first){return {-1,0};}//只能動c1,但partition 1 已經太小
    int gain1 = (c1.first!=-1) ? c1.second : -INT_MAX;
    int gain2 = (c2.first!=-1) ? c2.second : -INT_MAX;
    bool moveGp1 = (gain1 >= gain2);
    int gain,cellId;
    if(moveGp1){
        b1->pop_front();gain = gain1;cellId = c1.first;
        move(cellVec,cellId,b1);
        *groups.first-=1;
        *groups.second+=1;
    }
    else{
        b2->pop_front();gain = gain2;cellId = c2.first;
        move(cellVec,cellId,b2);
        *groups.first+=1;
        *groups.second-=1;
    }
    return {cellId,gain};
}

void FM(std::vector<Cell>&cellVec,float ratio1,float ratio2)
{
    Bucket b1,b2;
    initGainBucket(cellVec,b1,b2);
    #ifdef DEBUG
    std::cout<<"gainbucket1:\n";
    showGainBucket(&b1,cellVec);
    std::cout<<"gainbucket2:\n";
    showGainBucket(&b2,cellVec);
    #endif

    int g1Num = group1Num(cellVec);
    int g2Num = cellVec.size() - g1Num;




    std::vector<int>gainAcc{cellVec.size(),0};//accumulative gain 
    std::vector<int>moveRecord{cellVec.size(),-1};
    int gain = 0;
    for(int i = 0; i < cellVec.size() ; i++){
        auto pass = onePass(cellVec,{&b1,&b2},{ratio1,ratio2},{&g1Num,&g2Num});
        if(pass.first==-1)break;
        gain+=pass.second;
        gainAcc.at(i) = gain;
        moveRecord.at(i) = pass.first;
    }
    
    //pick max i step...... 
    //需要紀錄哪些被移動了......
    //做Recover 後重新產生gainbucket

}
