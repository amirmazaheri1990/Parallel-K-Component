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
#include <algorithm>

using namespace std;

int numberofprocessors = 10;
int q_threshold = 1000000;
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


pthread_mutex_t globalq_mutex;
pthread_t *threads;
//mutex *mutexes;
pthread_mutex_t *mutexes;
vector<vertex> nodes;
queue<int> global_queue;

int biggestnodenumb = -1;
vector <int> ancestor;
int myiterator = 0;

void mylocker(pthread_mutex_t *m,int a, int b){
  if (a>b){
    pthread_mutex_lock(&m[b]);
    pthread_mutex_lock(&m[a]);
    return;
  }
  if(a<b){
    pthread_mutex_lock(&m[a]);
    pthread_mutex_lock(&m[b]);
    return;
  }
  pthread_mutex_lock(&m[a]);
  return;
}

void myunlocker(pthread_mutex_t *m,int a, int b){
  if (a>b){
    pthread_mutex_unlock(&m[a]);
    pthread_mutex_unlock(&m[b]);
    return;
  }
  if(a<b){
    pthread_mutex_unlock(&m[b]);
    pthread_mutex_unlock(&m[a]);
    return;
  }
  pthread_mutex_unlock(&m[a]);
  return;
}
int findoriginate(int* ancestors,int input){
  int out=input;
  out = ancestors[input];
  while(out!=ancestors[out]){
    if(out==input){
      ancestors[input]=out;
      continue;
    }
    out = ancestors[out];
  }
  return out;
}
void bfs(vector<vertex> *nodes,int num,int* seen,queue<int> *gq,int* ancestors, int myancestor,queue<int> *localq){
  queue<int> *q = localq;
  mylocker(mutexes,myancestor,num);
  if (seen[num] == 1){
    if(ancestors[num]==num){
      ancestors[num]=myancestor;
    }
    int localmin = min(num,num);
    int k = findoriginate(ancestors,localmin);
    ancestors[myancestor]=ancestors[k];
    ancestors[num]=ancestors[k];
    myunlocker(mutexes,myancestor,num);
    return;
  }
   ancestors[num]=myancestor ;
  seen[num] = 1;
  myunlocker(mutexes,myancestor,num);
  int numberofneighbors = nodes->operator[](num).neighbors.size();
  for (int i=0;i<numberofneighbors;i++){
    int neighbnum = nodes->operator[](num).neighbors[i].num;
    //mylocker(mutexes,myancestor,neighbnum);
    q->push(neighbnum);
    //ancestors[myancestor]=ancestors[neighbnum];
    //myunlocker(mutexes,myancestor,neighbnum);
  }
  while(q->size()!=0){
    int next = q->front();
    q->pop();
    bfs(nodes,next,seen,gq,ancestors,ancestors[num],localq);///changed
    if (q->size()>q_threshold){
      while(q->size()>q_threshold){
        int tmp = q->front();
        q->pop();
        pthread_mutex_lock(&globalq_mutex);
        gq->push(tmp);
        pthread_mutex_unlock(&globalq_mutex);
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
    if(amir->global_queue->size()==0||1){//Changeeeee
      num = *(amir->myiterator);
      bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
      continue;
    }
    pthread_mutex_lock(&globalq_mutex);
    num = amir->global_queue->front();
    amir->global_queue->pop();
    pthread_mutex_unlock(&globalq_mutex);
    bfs(amir->nodes,num,amir->seen,amir->global_queue,amir->ancestors, num,&localq);
  }
  return 0;
}
int main(int argc, const char * argv[]) {
    pthread_mutex_init(&globalq_mutex,0);
    threads = new pthread_t[numberofprocessors];
    char strChar[200];
    cin.getline(strChar,100,'\n');
    while(!cin.eof()){
        string str(strChar);
        istringstream iss(str);
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
       }
        iss.str("");
        cin.getline(strChar,100,'\n');
    }
cout<<flush<<"graph is built...."<<endl;
cout<<flush<<"initializing..."<<endl;
    int numberofnodes = nodes.size();
    if(numberofprocessors>numberofnodes){
      numberofprocessors = numberofnodes;
    }
    int *seen = new int[numberofnodes];
    int *ancestors = new int[numberofnodes];
    mutexes = new pthread_mutex_t[numberofnodes];
    for (int i=0;i<numberofnodes;i++){
        seen[i]=0;
        ancestors[i]=i;
        pthread_mutex_init(&mutexes[i],0);
    }
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
    }

cout<<flush<<"Threads has been built..."<<endl;
    myiterator = 0;
    while(myiterator<numberofnodes){
      if(seen[myiterator]==1){
        myiterator = myiterator+1;
      }
    }
    cout<<"main process is done "<<endl;
    for (int i=0;i<numberofnodes;i++){
    }
int numberofclusters = 0;
sleep(1);
    for (int i=0;i<numberofnodes;i++){
      if(ancestors[i]==i){
        numberofclusters=numberofclusters+1;
      }
    }
    cout<<endl<<"this graph has "<<numberofclusters<<" parts."<<endl;
    return 0;
}
