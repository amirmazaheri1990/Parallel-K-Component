//
//  main.cpp
//  AlgorithmProject
//
//  Created by Amir Mazaheri on 11/25/14.
//  Copyright (c) 2014 Amir Mazaheri. All rights reserved.
//

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <mutex>

using namespace std;

int numberofprocessors = 10;
int q_threshold = 10000;
bool ended = 0;
class vertex{

public:
  vertex(int n){
    this -> num = n;
  }
  int num;
  vector<vertex> neighbors;



};

class MyInput{
public:
  vector<vertex> *nodes;
  queue<int> *global_queue;
  int* seen;
  int* ancestors;
  int numberofnodes;
  int* myiterator;
};


mutex globalq_mutex;
pthread_t *threads;
mutex *mutexes;
vector<vertex> nodes;
queue<int> global_queue;

int biggestnodenumb = -1;
vector <int> ancestor;
int myiterator = 0;

void mylocker(mutex* m,int a, int b){
  //return;
  if (a>b){
    m[b].lock();
    m[a].lock();
    cout<<"lock "<<a<<" "<<b<<" "<<endl;
    return;
  }
  if(a<b){
    m[a].lock();
    m[b].lock();
    cout<<"lock "<<a<<" "<<b<<" "<<endl;
    return;
  }
  m[a].lock();
  cout<<"lock "<<a<<" "<<b<<" "<<endl;
  return;
}

void myunlocker(mutex* m,int a, int b){
  //return;
  if (a>b){
    m[a].unlock();
    m[b].unlock();
    //cout<<"unlock "<<a<<" "<<b<<" "<<endl;
    return;
  }
  if(a<b){
    m[b].unlock();
    m[a].unlock();
    //cout<<"unlock "<<a<<" "<<b<<" "<<endl;
    return;
  }
  m[a].unlock();
  //cout<<"unlock "<<a<<" "<<b<<" "<<endl;
  return;
}

void bfs(vector<vertex> *nodes,int num,int* seen,queue<int> *gq,int* ancestors, int myancestor,queue<int> *localq){
  queue<int> *q = localq;
  /*if (seen[num] == 1){
    ancestors[myancestor]=ancestors[num];
    return;
  }*/

  //mutexes[num].lock();
  //cout<<"lock "<<num<<endl;
  mylocker(mutexes,myancestor,num);
  if (seen[num] == 1){
    ancestors[myancestor]=ancestors[num];
    //cout<<"unlock "<<num<<endl;
    //mutexes[num].unlock();
    myunlocker(mutexes,myancestor,num);
    //cout<<"after unlock "<<num<<endl;
    return;
  }
  ancestors[num] = myancestor;
  seen[num] = 1;
  //cout<<"unlock "<<num<<endl;
//  mutexes[num].unlock();
myunlocker(mutexes,myancestor,num);
  //cout<<"after unlock "<<num<<endl;



  int numberofneighbors = nodes->operator[](num).neighbors.size();

  for (int i=0;i<numberofneighbors;i++){

    int neighbnum = nodes->operator[](num).neighbors[i].num;
    if (seen[neighbnum]==0){

      q->push(neighbnum);


    }
    else{
      mylocker(mutexes,myancestor,neighbnum);
      ancestors[myancestor]=ancestors[neighbnum];
      //mutexes[myancestor].unlock();
      myunlocker(mutexes,myancestor,neighbnum);
    }

  }

  while(q->size()!=0){
    int next = q->front();
    q->pop();
    bfs(nodes,next,seen,gq,ancestors,myancestor,localq);
    // Flush some part of local queue to the global queue
    if (q->size()>q_threshold){
      while(q->size()>q_threshold){
        int tmp = q->front();
        q->pop();
        globalq_mutex.lock();

        gq->push(tmp);

        globalq_mutex.unlock();

      }
    }
  }
  return;
}


void* processor(void* input){
  srand(time(0));
  MyInput* amir = (MyInput*)input;
  int num = rand()%(amir->numberofnodes);
  queue<int> localq;

  bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
  while(*(amir->myiterator)<amir->numberofnodes){
  //  cout<<*(amir->myiterator)<<endl;
    if(amir->global_queue->size()==0){
      num = *(amir->myiterator);
      bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
      continue;
    }
    //FETCH FROM GLOBAL Q
    globalq_mutex.lock();
    num = amir->global_queue->front();
    amir->global_queue->pop();
    globalq_mutex.unlock();
    bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
  }
  //cout<<"end of the processor!!!!!"<<endl;
  return 0;
}


int main(int argc, const char * argv[]) {
    //sem_init(&globalq_mutex,0,1);
    threads = new pthread_t[numberofprocessors];
    char strChar[200];
    cin.getline(strChar,100,'\n');
//cout<<flush<<"Building the graph..."<<endl;
    while(!cin.eof()){
        string str(strChar);
        istringstream iss(str);
        //cout<<flush<<str<<endl;
        bool change =0;
        while (!iss.eof()){
            int n1;
            int n2;
            iss>>n1;
            iss>>n2;
            int maxn = n1;
            if(n2>n1){
                maxn = n2;
            }
            if(maxn>biggestnodenumb){
                while (biggestnodenumb<maxn) {
                    vertex amir(biggestnodenumb+1);
                    nodes.push_back(amir);
                    biggestnodenumb = biggestnodenumb+1;
                }
            }
            nodes[n1].neighbors.push_back(n2);
            change = !change;
       }
        iss.str("");
        cin.getline(strChar,100,'\n');
    }
cout<<flush<<"graph is built...."<<endl;
// Initialize
cout<<flush<<"initializing..."<<endl;
    int numberofnodes = nodes.size();
    int *seen = new int[numberofnodes];
    int *ancestors = new int[numberofnodes];
    mutexes = new mutex[numberofnodes];
    for (int i=0;i<numberofnodes;i++){
        seen[i]=0;
        ancestors[i]=i;
        //sem_init(&(mutexes[i]), 0, 1);
    }
//Start the BFS threading
cout<<flush<<"starting the threads..."<<endl;
    queue<int> *q = new queue<int>;
    MyInput input;
    input.nodes = &nodes;
    input.global_queue=&global_queue;
    input.seen=seen;
    input.ancestors=ancestors;
    input.numberofnodes=numberofnodes;
    input.myiterator=&(myiterator);
    for (int i=0;i<numberofprocessors;i++){

      pthread_create(&threads[i],NULL,processor,&input);
      //cout<<flush<<"thread number "<<i<<" has been created"<<endl;
    }
    // update myiterator
    cout<<flush<<"Threads has been built..."<<endl;
    myiterator = 0;
    while(myiterator<numberofnodes){
      //cout<<"wait to end..."<<endl;
      //pause(9000);
      if(seen[myiterator]==1){
      //  cout<<"my iterator is "<<myiterator<<endl;
        myiterator = myiterator+1;
      }
    }
    cout<<"main process is done "<<myiterator<<endl;

//Count the found clusters
for (int i=0;i<numberofprocessors;i++){
    pthread_join(threads[i],NULL);
    //cout<<i<<"'th processor is done"<<endl;
  //cout<<flush<<"thread number "<<i<<" has been created"<<endl;
}
int numberofclusters = 0;
    for (int i=0;i<numberofnodes;i++){
      cout<<i<<" "<<ancestors[i]<<endl;
      if(ancestors[i]==i){
        numberofclusters=numberofclusters+1;
        //cout<<i<<" ";
      }
    }
    cout<<endl<<"this graph has "<<numberofclusters<<" parts."<<endl;

    return 0;
}
