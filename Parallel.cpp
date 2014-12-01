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

using namespace std;

int numberofprocessors = 10;
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


sem_t globalq_mutex;
pthread_t *threads;
sem_t *mutexes;
vector<vertex> nodes;
queue<int> global_queue;

int biggestnodenumb = -1;
vector <int> ancestor;
int myiterator = 0;

void bfs(vector<vertex> *nodes,int num,int* seen,queue<int> *gq,int* ancestors, int myancestor,queue<int> *localq){
  int q_threshold = 1000000;

  queue<int> *q = localq;
  if (seen[num] == 1){
    ancestors[myancestor]=ancestors[num];
    return;
  }

  sem_wait(&mutexes[num]);

  if (seen[num] == 1){
    ancestors[myancestor]=ancestors[num];
    sem_post(&mutexes[num]);
    return;
  }
  ancestors[num] = myancestor;
  seen[num] = 1;

  sem_post(&mutexes[num]);




  int numberofneighbors = nodes->operator[](num).neighbors.size();

  for (int i=0;i<numberofneighbors;i++){

    int neighbnum = nodes->operator[](num).neighbors[i].num;
    if (seen[neighbnum]==0){

      q->push(neighbnum);


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
        sem_wait(&globalq_mutex);

        gq->push(tmp);
    
        sem_post(&globalq_mutex);

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
    if(amir->global_queue->size()==0){
      num = *(amir->myiterator);
      bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
      continue;
    }
    //FETCH FROM GLOBAL Q
    sem_wait(&globalq_mutex);
    num = amir->global_queue->front();
    amir->global_queue->pop();
    sem_post(&globalq_mutex);
    bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
  }
  //cout<<"end of the processor!!!!!"<<endl;
  pthread_exit(NULL);
}


int main(int argc, const char * argv[]) {
    sem_init(&globalq_mutex,0,1);
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
    mutexes = new sem_t[numberofnodes];
    for (int i=0;i<numberofnodes;i++){
        seen[i]=0;
        ancestors[i]=-1;
        sem_init(&(mutexes[i]), 0, 1);
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
    while(myiterator<numberofnodes){
      //cout<<"wait to end..."<<endl;
      //pause(9000);
      if(seen[myiterator]==1){
        myiterator = myiterator+1;
      }
    }
    cout<<"main process is done"<<endl;

//Count the found clusters
for (int i=0;i<numberofprocessors;i++){
  pthread_join(threads[i],NULL);
  //cout<<"i'th processor is done"<<endl;
  //cout<<flush<<"thread number "<<i<<" has been created"<<endl;
}
int numberofclusters = 0;
    for (int i=0;i<numberofnodes;i++){
      if(ancestors[i]==i){
        numberofclusters=numberofclusters+1;
      }
    }
    cout<<flush<<"this graph has "<<numberofclusters<<" parts."<<endl;

    return 0;
}
