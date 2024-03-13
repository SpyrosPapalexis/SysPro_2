using namespace std;

struct Obj{
    string path;
    int sock;
    Obj(string, int);
};

struct Node{
    Obj* item;
    Node* next;
    Node(string, int);
};

struct Queue{
    Node *start, *end;
    int count, max;
    Queue();
    void setmax(int);
    void push(string, int);
    Obj *pop();
    bool isempty();
    bool isfull();
    ~Queue();
};