#include "../include/FM.hpp"

#include <vector>
#include <iostream>
#include <map>

#ifdef DEBUG
extern std::map<int,char>dict;
#endif

void insertGain(Bucket&b,int gain,Cell&cell)
{
    int pos = b.maxPin + gain; // min gain = -maxPin
    if(pos < 0 || pos > 2 * b.maxPin){
        std::cerr<<"gain range error , input gain:"<<gain<< " min gain : "<< - b.maxPin << " max gain : "<<b.maxPin<<"\n";
        exit(1);
    }
    b.gainVec.at(pos).push_front(cell.sortId);
    b.maxGain = std::max(b.maxGain,gain); //update
}

void initGainBucket(std::vector<Cell>&cellVec,Bucket&b1,Bucket&b2)
{
    int maxPin = 0;
    for(auto &cell:cellVec) maxPin = std::max(cell.netNum,maxPin);
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
        cell.group1 ? insertGain(b1,gain,cell) : insertGain(b2,gain,cell);
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

void FM(std::vector<Cell>&cellVec)
{
    Bucket b1,b2;
    initGainBucket(cellVec,b1,b2);

    #ifdef DEBUG
    
    std::cout<<"gainbucket1:\n";

    showGainBucket(&b1,cellVec);
    std::cout<<"gainbucket2:\n";
    showGainBucket(&b2,cellVec);

    #endif
}
