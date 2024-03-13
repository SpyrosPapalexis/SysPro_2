#include <iostream>
#include "queue.hpp"

Obj::Obj(string s, int i){
    path = s;
    sock = i;
}


Node::Node(string s, int i){
    item = new Obj(s, i);
    next = NULL;
}

Queue::Queue(){
    start = end = NULL;
    count = max = 0;
}

    void Queue::push(string s, int i){
        Node* t = new Node(s, i);
        count++;
 
        if (end == NULL){
            start = end = t;
            return;
        }
 
        end->next = t;
        end = t;
    }

    void Queue::setmax(int i){
        max = i;
    }
 
    Obj *Queue::pop(){
        if (start == NULL) return NULL;

        count--;
        Node* t = start;
        start = start->next;
 
        if (start == NULL) end = NULL;
        return t->item;
    }

    bool Queue::isempty(){
        if (start == NULL) return true;
        return false;
    }

    bool Queue::isfull(){
        if (count == max) return true;
        return false;
    }

Queue::~Queue(){
    while (!isempty()){
        pop();
    }
}
