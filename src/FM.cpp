#include "../include/FM.hpp"

#include <vector>
#include <iostream>
#include <map>
#include <utility>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <float.h>
#include <climits>
#ifdef DEBUG
extern std::map<int,char>dict;
#endif









// used to move cellId in gainbucket.
void Bucket:: erase(Cell*cell){
    if(cell->freeze){// if cell is already freeze,then it can't be in gainbucket.
        std::cerr<<"carefull,cell.freeze = true, it can't in gainBucket\n";
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
void initGainBucket(std::vector<Cluster*>&cellVec,std::vector<Net*>&netVec,Bucket&b1,Bucket&b2)
{
    b1.gainVec.clear();b2.gainVec.clear();

    // Allocate
    int maxPin = 0;
    for(auto cell:cellVec) maxPin = std::max(cell->netNum,maxPin);

    int size = maxPin * 2 + 1;
    if(b1.gainVec.size()!= size)b1.gainVec.resize(size);// index : [0,2*maxPin] , gain: [-maxPin,maxPin]
    if(b2.gainVec.size()!= size)b2.gainVec.resize(size);

    b1.maxGain = b2.maxGain = -maxPin;
    b1.maxPin  = b2.maxPin  = maxPin;

    // Build gainBucket
    for(auto  cell :cellVec){
        cell->gain = 0;
        if(!cell->isValid())continue;
        int gain = 0;
        for(auto netinfo : cell->getNets()){
            int nid = netinfo.first;
            Net* net = netVec.at(nid);
            int cellNum = net->group1Num + net->group2Num;//differ to "really size", is "cluster or cell number"
            int FromNum = cell->group1 ? (net->group1Num) :(net->group2Num);
            int ToNum = cellNum - FromNum;
            if(FromNum==1&&cellNum!=1)gain++;
            if(ToNum==0&&cellNum!=1)gain--;
        }
        cell->gain = gain;
        cell->group1 ? b1.push_front(cell) : b2.push_front(cell);
    }
}

void showGainBucket(Bucket*b,std::vector<Cluster*>&cellVec)
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


inline int group1Num(std::vector<Cluster*>&cellVec){
    int n = 0;
    for(auto cell:cellVec)
        if(cell->isValid()&&cell->group1)
            n += cell->getSize();
    return n;
}

void Net::addCells(Cell*cell){
    cells.push_front(cell->sortId);
    cell->group1 ? (group1+=cell->getSize()) : (group2+=cell->getSize());
    cell->group1 ? (group1Num++) : (group2Num++); //new 
}




void InitNets(std::vector<Cluster*>&cellVec,std::vector<Net*>&nets){
    // clear all
    for(auto n:nets){
        n->group1 = n->group2 = n->group1Num = n->group2Num = 0;
        n->cells.clear();
    }
    // build connect
    // only consider Valid cell.
    for(auto cell : cellVec){
        if(cell->isValid()){
            for(auto net : cell->getNets()){
                nets.at(net.first)->addCells(cell);
            }
        }
    }
}

// update cells in same netlist of moving cell.
void update(std::vector<Cluster*>&cellVec,Net*net,Cell*movingcell,std::pair<Bucket*,Bucket*>&buckets,bool group1,bool increment,bool onlyOne){
    for(auto cellId:net->cells){
        Cell* cell = cellVec.at(cellId);// get cell        
        if(!cell->isValid())
        {
            std::cerr<<"error\n";exit(1);
        }
        if(cell != movingcell && group1 == cell->group1){ // the specified group.
            if(cell->freeze){
                if(onlyOne)break;
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

void move(std::vector<Cluster*>&cellVec,std::vector<Net*>&netVec,int cellId,std::pair<Bucket*,Bucket*>&buckets){
    Cell* cell = cellVec.at(cellId);
    cell->group1 ? (buckets.first->erase(cell)):(buckets.second->erase(cell));
    bool from_group = cell->group1;
    for(auto netinfo : cell->getNets()){
        Net* net = netVec.at(netinfo.first);
        int cellNum = net->group1Num + net->group2Num;//differ to "really size", is "cluster or cell number"
        int From = cell->group1 ? (net->group1Num) :(net->group2Num);
        int To = cellNum - From;
        if(To == 0 && cellNum!=1)//not only one cell 
            update(cellVec,net,cell,buckets,from_group,true,false);//increment "from group" cells's gain. 
        else if (To == 1) // if to == 1
            update(cellVec,net,cell,buckets,!from_group,false,true);//decrement "to group" cell's gain. 
     
        From = From-1;

        if(From == 0 && cellNum!=1) // if From ==0
            update(cellVec,net,cell,buckets,!from_group,false,false);//decrement "to group" cells's gain. 
        else if (From == 1) // if From == 1
            update(cellVec,net,cell,buckets,from_group,true,true);//increment "from group" cell's gain. 
        
        // update net's group num.
        if(cell->group1)
            net->updateGroup(cell->getSize(),true);
        else
            net->updateGroup(cell->getSize(),false);
    }
    //really move
    cell->group1 = !cell->group1;
}





std::pair<std::pair<int,int>,std::pair<int,int>> validStatePick(std::pair<Bucket*,Bucket*>buckets,std::pair<float,float>ratios,\
    std::pair<int*,int*>groups,ties *tie){

    int minSize = 1;
    float r1 = (float) (*groups.first - minSize) / (*groups.first + *groups.second); // if move cell in partition1 to 2
    float r2 = (float) (*groups.first + minSize) / (*groups.first + *groups.second); // if move cell in partition2 to 1

    //future feature.
    //Size limit suggest, this is not necessary, because we have the decluster method.
    // but maybe it can help
    Bucket *b1 = buckets.first;
    Bucket *b2 = buckets.second;
    auto c1 = b1->front(tie);//<id,gain>
    auto c2 = b2->front(tie);

    //min Size pre-check
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
        else if(r > ratios.second)
            return {{c.first,c.second},{-1,-INT_MAX}};;
    }   

    // valid state
    return validStatePick(buckets,ratios,groups,tie);
}

//first : move id
//second : gain
std::pair<int,int> oneMove(std::vector<Cluster*>&cellVec,std::vector<Net*>&netVec,std::pair<Bucket*,Bucket*>buckets,\
    std::pair<float,float>ratios,std::pair<int*,int*>groups,ties *tie){
    auto candidates = getCandidate(buckets,ratios,groups,tie);// get
    if(candidates.first.first==-1 && candidates.second.first==-1)return candidates.first;// no cell can move

    int c1 = candidates.first.first    ,    c2 = candidates.second.first;
    int gain1 = candidates.first.second, gain2 = candidates.second.second;
    bool moveGp1 = tie ? (tie(gain1,gain2,c1,c2) == c1): (gain1 >= gain2); // choose cell to move
    if(moveGp1){
        move(cellVec,netVec,c1,buckets);
        *groups.first  -= cellVec.at(c1)->getSize();
        *groups.second += cellVec.at(c1)->getSize();
        return {c1,gain1};
    }else{
        move(cellVec,netVec,c2,buckets);
        *groups.first  += cellVec.at(c2)->getSize();
        *groups.second -= cellVec.at(c2)->getSize();
        return {c2,gain2};
    }
}




//要再去檢查candidate size = 0的情況
//原因是目前ratio還沒支援到新的版本

int getBestStage(std::vector<int>&gainAcc,std::vector<float>&ratioRecord,int iterations,std::pair<float,float>ratios){
    
    auto validRatio = [&ratioRecord,&ratios](int i){return ratioRecord.at(i) >= ratios.first && ratioRecord.at(i) <= ratios.second;};
    std::vector<int>candidates;
    
    int bestGain = -INT_MAX;
    for(int i = 1;i <= iterations;i++){                 //only compare 1 to iteration , not 0.
        
        if(!validRatio(i))continue;//not valid ratio

        if(bestGain < gainAcc.at(i)){ // new best
            candidates.clear();
            candidates.push_back(i);
            bestGain  = gainAcc.at(i);
        }else if (bestGain == gainAcc.at(i)){// add candidate
            candidates.push_back(i);
        }
    }

    if(candidates.size()==0)return 0; // all ratio are invalid,recover.
    
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


void RecoverToStage(std::vector<Cluster*>&cellVec,std::vector<Net*>&netVec,std::vector<int>&moveRecord,int bestIteration,int totalIteration,int&g1Num,int&g2Num){
    for(int k = totalIteration; k > bestIteration;k--){
        int CellId = moveRecord.at(k);
        auto &cell = *cellVec.at(CellId);
        int size = cell.getSize();
        if(cell.group1){//now in group1, recover to group2
            g1Num-=size;
            g2Num+=size;
            for(auto netinfo:cell.getNets()){
                int nid = netinfo.first;
                Net* net = netVec.at(nid);
                net->updateGroup(size,true);
            }
        }else{
            g1Num+=size;
            g2Num-=size;
            for(auto netinfo:cell.getNets()){
                int nid = netinfo.first;
                Net* net = netVec.at(nid);
                net->updateGroup(size,false);
            }
        }
        cell.group1 = !cell.group1;
    }
}


void preCheckConstraint(int totalNum,float ratio1,float ratio2)
{
    int half1 = totalNum - totalNum/2;
    if(float(half1)/totalNum < ratio1 || float(half1)/totalNum > ratio2){
        std::cerr<<"your cell size can never reach the request ratio,please enter new ratio constraint.\n";
        exit(1);
    }
}


struct OnepassArgs{
    std::vector<int>*GainAcc;
    std::vector<int>*MoveRecord;
    std::vector<float>*RatioRecord;

    std::vector<Cluster*>*CellVec;
    std::vector<Net*>*NetVec;
    std::pair<Bucket*,Bucket*>buckets;
    std::pair<float,float>ratios;
    std::pair<int*,int*>groups;
};


// //return iteration
int OnePass(const OnepassArgs&args,ties *tie){
    int k = 1,gain = 0;
    for(; k <= args.CellVec->size() ; k++){
        auto pass = oneMove(*args.CellVec,*args.NetVec,args.buckets,args.ratios,args.groups,tie);
        if(pass.first == -1)break; // no cell can move.
        gain += pass.second;
        args.GainAcc->at(k) = gain;
        args.MoveRecord->at(k) = pass.first;
        args.RatioRecord->at(k) = (float) (*args.groups.first) / (*args.groups.first + *args.groups.second);
    }
    return k - 1;
}

//return true if decluster happen.
bool Decluster(std::queue<int>&declusterQ,std::vector<Cluster*>&cellVec,std::vector<Net*>&netlist,int phase){

    if(declusterQ.empty()||phase<=0)return false;

    
    std::queue<int>nextstage;

    while(!declusterQ.empty()){
        int cid = declusterQ.front();
        declusterQ.pop();
        for(auto clst:cellVec.at(cid)->decluster(phase)){
            // std::cout<<clst<<" is released\n";
            nextstage.push(clst);
        }
        if(cellVec.at(cid)->is_cluster())
           nextstage.push(cid);
    }


    int validnum = 0;
    for(auto cell:cellVec)
    {
        if(cell->is_master())
        {
            // std::cout<<cell->id<<"is master\n";
            validnum++;
        }
    }
    std::cout<<"after decluster,validNum:"<<validnum<<"\n";

    
    InitNets(cellVec,netlist);
    declusterQ = std::move(nextstage);


    std::cout<<"after decluster,cutsize:"<<CutSize(netlist)<<"\n";

    return true;
}

#include <fstream>
void OutputtEST(std::vector<Cluster*>&cellVec){
    std::ofstream out{"outputDebug.txt"};
    for(auto c:cellVec){
        
        Cluster*ptr = c;
        while(!ptr->isValid())
        {
            ptr = cellVec.at(ptr->clusterId);
        }
        out << ptr->group1<<"\n";
    }
    out.close();
}


std::queue<int>getDeclusterQ(std::vector<Cluster*>&cellVec){
    std::queue<int>declusertQ;
    for(auto cell:cellVec)
        if(cell->is_master()&&cell->is_cluster())
            declusertQ.push(cell->clusterId);
    return declusertQ;
}

void FM(std::vector<Cluster*>&cellVec,std::vector<Net*>netlist,float ratio1,float ratio2,int phase,ties*tie){
    // pre-check ratio constraints.
    int totalNum = cellVec.size();
    preCheckConstraint(totalNum,ratio1,ratio2);

    //allocate information Recored, idx start from 1 .
    std::vector<int>gainAcc(cellVec.size() + 1,-1);//accumulative gain 
    std::vector<int>moveRecord(cellVec.size() + 1,-1);
    std::vector<float>ratioRecord(cellVec.size() + 1,-1);

    // maintain groupNumber information for caculating ratio
    int g1Size = 0,g2Size;
    std::for_each(cellVec.begin(),cellVec.end(),[&g1Size](const Cluster*c){if(c->group1)g1Size++;});
    g2Size = cellVec.size() - g1Size;

    
    //get declusterQ
    std::queue<int> declusterQ = getDeclusterQ(cellVec); 

  
    int maxGain;
    do{
        Bucket b1,b2;
        //  init GainBucket every iteration.
        initGainBucket(cellVec,netlist,b1,b2);
        gainAcc.at(0) = 0;
        //  OnePass
        int totalIteration = OnePass(OnepassArgs{&gainAcc,&moveRecord,&ratioRecord,&cellVec,&netlist,{&b1,&b2},{ratio1,ratio2},{&g1Size,&g2Size}},tie);
        //  Pick best state 
        int bestIteration = getBestStage(gainAcc,ratioRecord,totalIteration,{ratio1,ratio2});
        
        //  Recover state to bestIteration.
        RecoverToStage(cellVec,netlist,moveRecord,bestIteration,totalIteration,g1Size,g2Size);

        maxGain = gainAcc.at(bestIteration);
        std::cout<<"maxGain:"<<maxGain<<"\n";
        std::cout<<"cut size:"<<CutSize(netlist)<<"\n";

    }while(maxGain > 0 || Decluster(declusterQ,cellVec,netlist,phase--)); 
}


int CutSize(std::vector<Net*>&net){
    int cut = 0;
    for(auto n:net){
        if(n->group1&&n->group2)
            cut++;
    }
    return cut;
}



int Cluster::clustering(Cluster*v,int phase){

    if(v==nullptr||!v->is_master()){
        std::cerr<<"warning in clustering, input v is invalid\n";
        return this->clusterId;
    }if(v->clusterId==this->clusterId)return this->clusterId;
    
    if(!this->is_master()){
        std::cerr<<"warning in clustering," <<sortId <<" is invalid,please use master in this cluster to cluster another vertex,master id :"<<this->clusterId<<"\n";
        return this->clusterId;
    }
   
    // Cluster all cells.   (setting flag is enough.)
    v->valid = false; // not valid until decluster
    v->clusterId = this->clusterId; //not a cluster master ever.
    v->clusterPhase = phase; // saving cluster phase for declustering.
    this->cells.push_back(v);   //add only v,do not add v's cells.
    this->cellsNum+=v->cellsNum;
    
    // update the NetSet.
    int newNetNum = 0;
    auto hint = NetSet.begin();
    for(auto net:v->NetSet){  //v->Netset is sorted,can use hint for speeding up.
        auto pos = NetSet.find(net.first);
        if(pos ==NetSet.end()){
            hint = NetSet.insert(hint,{net.first,net.second});
            newNetNum++;
        }
        else{
            pos->second += net.second;//bug-Fix
        }
    }
    this->netNum += newNetNum;
    return this->clusterId;
}


std::vector<int> Cluster::decluster(int phase){
    
    if(is_master()){
        std::vector<int>declusterID;
        std::list<Cluster*>remain;
        for(Cluster* cell : cells){
            
            if(cell != this && cell->clusterPhase == phase){//the specified phase.
                cell->valid = true;
                cell->clusterId = cell->sortId;
                cell->group1 = this->group1; // important
                cell->clusterPhase = -1; 
                for(auto net:cell->getNets()){
                    auto it = this->NetSet.find(net.first);
                    if(!(it->second-=net.second)){
                        this->NetSet.erase(it);
                    }
                }
                declusterID.push_back(cell->clusterId);
                this->cellsNum-=cell->cellsNum;
            }
            else{
                remain.push_back(cell);
            }
        }
        cells.clear();
        cells = std::move(remain);
      
        return declusterID;
    }else{
        std::cerr<<"void Cluster::decluster() warning, cell "<<sortId<<" is not a cluster master\n";
        std::cerr<<"masterid : "<<clusterId<<"\n";
        return {};
    }
}






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

    int targetNum = 200;
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
