
# Partition - FM with coarsen and uncoarsen stage      

## Make    

```
$ make 
```
## Execute

```
$ ./Lab2.exe <INPUT>

```



## Example	

```
$ time ./Lab2 input2.txt.hgr
Coarsen to only 3947 cells
total coarsen stage:1
final cutsize:33

real    0m0.252s
user    0m0.000s
sys     0m0.031s

$ time ./Lab2 input3.txt.hgr
Coarsen to only 56859 cells
total coarsen stage:1
final cutsize:18

real    0m2.582s
user    0m0.000s
sys     0m0.000s

```



## IDEA	

參考hMetisKway-karypis-dac99的multi-level partition



![image](https://user-images.githubusercontent.com/52790122/141340024-4eb0442a-152a-4fcc-b5d8-42662d9722a0.png)


Coarsen方式:使用Edge coarsen.

Partition Refinement: FM

核心演算法 : FM 流程

```
   int maxGain;
    int Tolerance = 0;
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
        // std::cout<<"maxGain:"<<maxGain<<"\n";
        // std::cout<<"cut size:"<<CutSize(netlist)<<"\n";

        //Climbing,如果<=0 會試著移動看看  
        if(maxGain <= 0)
            Tolerance++;
        else 
            Tolerance = 0;
    }while(Tolerance < 3 || Decluster(declusterQ,cellVec,netlist,phase--)); 
```





