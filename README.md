
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

if setting coarsen stage = 6

$ time ./Lab2.exe input3.txt.hgr
Coarsen to only 1539 cells
total coarsen stage:6
final cutsize:19

real    0m24.490s
user    0m0.000s
sys     0m0.031s


$ time ./Lab2.exe input2.txt.hgr
Coarsen to only 111 cells
total coasen stage:6
final cutsize:33

real    0m1.293s
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

    }while(maxGain > 0 || Decluster(declusterQ,cellVec,netlist,phase--)); 
```





