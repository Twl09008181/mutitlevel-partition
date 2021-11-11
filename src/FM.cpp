#include "../include/FM.hpp"

#include <vector>
#include <iostream>
#include <map>
#include <utility>
#include <queue>

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

    // Build
    for(auto  cell :cellVec){
        cell->gain = 0;
        if(!cell->isValid())continue;
        int gain = 0;
        for(auto net : cell->getNets()){
            int nid = net.first;
            int net_cellNum = netVec.at(nid)->cells.size();//differ to "really size", is "cluster or cell number"   
            if(net_cellNum==0)
            {
                std::cerr<<"error\n";
                exit(1);
            }

            //BUG
            // auto From_To_Num = GroupNum(*netVec.at(nid),cell); //GroupNum回傳的是真實size,不太可能等於1或0!!
            // if(From_To_Num.first==1&&net_cellNum !=1) gain++;  //From set == 1 
            // if(From_To_Num.second==0&&net_cellNum !=1)gain--; //To set == 0


            int FromNum = 0;
            int ToNum = 0;
            
            for(auto c:netVec.at(nid)->cells)
            {
                if(cellVec.at(c)->isValid())
                {
                    if(cellVec.at(c)->group1==cell->group1)
                        FromNum++;
                    else
                        ToNum++;
                }
            }

            if(FromNum==1&&net_cellNum!=1)gain++;
            if(ToNum==0&&net_cellNum!=1)gain--;

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
}




void InitNets(std::vector<Cluster*>&cellVec,std::vector<Net*>&nets){
    // clear all
    for(auto n:nets){
        n->group1 = n->group2 = 0;
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
        // auto From_To_Num = GroupNum(*net,cell); 
        int net_cellNum = net->cells.size();//differ to "really size", is "cluster or cell number"   
        // int To = From_To_Num.second;

            int From = 0;
            int To = 0;
            
            for(auto c:net->cells)
            {
                if(cellVec.at(c)->isValid())
                {
                    if(cellVec.at(c)->group1==cell->group1)
                        From++;
                    else
                        To++;
                }
            }

            

        if(To == 0 && net_cellNum!=1)//not only one cell 
            update(cellVec,net,cell,buckets,from_group,true,false);//increment "from group" cells's gain. 
        else if (To == 1) // if to == 1
            update(cellVec,net,cell,buckets,!from_group,false,true);//decrement "to group" cell's gain. 
        //assume move
        // int From = From_To_Num.first - 1;
        From = From-1;
        if(From == 0 && net_cellNum!=1) // if From ==0
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
                net->group1-=size;
                net->group2+=size;
            }
        }else{
            g1Num+=size;
            g2Num-=size;
            for(auto netinfo:cell.getNets()){
                int nid = netinfo.first;
                Net* net = netVec.at(nid);
                net->group1+=size;
                net->group2-=size;
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

void FM(std::vector<Cluster*>&cellVec,std::vector<Net*>netlist,float ratio1,float ratio2,int phase,ties*tie){
    // pre-check ratio constraints.
    int totalNum = cellVec.size();
    preCheckConstraint(totalNum,ratio1,ratio2);

    //allocate information Recored, idx start from 1 .
    std::vector<int>gainAcc(cellVec.size() + 1,-1);//accumulative gain 
    std::vector<int>moveRecord(cellVec.size() + 1,-1);
    std::vector<float>ratioRecord(cellVec.size() + 1,-1);
    // maintain groupNumber information for caculating ratio
    int g1Num = group1Num(cellVec),g2Num = cellVec.size() - g1Num;


    int maxGain;
    int cut = CutSize(netlist);
    std::cout<<"origin cutsize:"<<cut<<"\n";

  int validNum = 0;
    std::queue<int>declusertQ;
    for(auto cell:cellVec){
        if(cell->is_master()&&cell->is_cluster()){
            declusertQ.push(cell->clusterId);
            validNum++;
        }
    }

    std::cout<<"valid:"<<validNum<<"\n";

    int originPhase = phase;
    do{
        Bucket b1,b2;
        // init GainBucket every iteration.
        initGainBucket(cellVec,netlist,b1,b2);

        // std::cout<<"b1\n";
        // showGainBucket(&b1,cellVec);
        // std::cout<<"b2\n";
        // showGainBucket(&b2,cellVec);

        gainAcc.at(0) = 0;
        // OnePass
        int totalIteration = OnePass(OnepassArgs{&gainAcc,&moveRecord,&ratioRecord,&cellVec,&netlist,{&b1,&b2},{ratio1,ratio2},{&g1Num,&g2Num}},tie);


        


        // // Pick best state 
        int bestIteration = getBestStage(gainAcc,ratioRecord,totalIteration,{ratio1,ratio2});
        maxGain = gainAcc.at(bestIteration);

        
        

        // // Recover state to bestIteration.
        RecoverToStage(cellVec,netlist,moveRecord,bestIteration,totalIteration,g1Num,g2Num);


        for(int i = 0;i<=totalIteration;i++)
        {
            if(gainAcc.at(i)==-373)
            {
                std::cout<<"find 373\n";
            }

        }

        std::cout<<"maxGain:"<<maxGain<<"\n";
        std::cout<<"cut size:"<<CutSize(netlist)<<"\n";


        if(maxGain==-649)
        {
            std::cout<<"out\n";
            OutputtEST(cellVec);

            for(int p=phase;p>0;p--)
            {
                Decluster(declusertQ,cellVec,netlist,p);
            }

            std::cout<<"Cutsize:"<<CutSize(netlist);


            exit(1);

        }


    }while(maxGain > 0 || Decluster(declusertQ,cellVec,netlist,phase--));

    validNum = 0;
    for(auto cell:cellVec)
    {
        if(cell->isValid())validNum++;

    }
    std::cout<<"valid num:"<<validNum<<"\n";


// 
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
    // std::cout<<this->sortId<<" clustered "<<v->sortId<<"in phase "<<phase<<"\n";
    // Cluster all cells.   (setting flag is enough.)
    v->valid = false; // not valid until decluster
    v->clusterId = this->clusterId; //not a cluster master ever.
    v->clusterPhase = phase; // saving cluster phase for declustering.
    // this->iscluster = true;
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
            
            // if(cell->sortId==5067&&cell->clusterPhase==phase)
            // {
            //     std::cout<<"find 5067 in phase"<<phase<<"\n";
            // }
            
            if(cell != this && cell->clusterPhase == phase){//the specified phase.

                

                // std::cout<<this->sortId<<" decluster "<<cell->sortId<<"in phase"<<phase<<"\n";
                cell->valid = true;
                cell->clusterId = cell->sortId;
                // cell->iscluster = cell->cellsNum > 1;
                cell->group1 = this->group1; // important
                cell->clusterPhase = -1; 
                for(auto net:cell->getNets()){
                    auto it = this->NetSet.find(net.first);
                    if(!(it->second-=net.second)){
                        this->NetSet.erase(it);
                        // std::cout<<"erase net\n";
                    }
                    else if(it->second<0)
                    {
                        std::cerr<<"error\n";
                        exit(1);
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
