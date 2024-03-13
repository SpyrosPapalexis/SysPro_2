#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "queue.hpp"

#define BUFFER 1024
#define BIGBUFFER 1024
#define BACKLOG 1280

using namespace std;

map<int, pthread_mutex_t> locks;        //maps each socket with its mutex
pthread_mutex_t qlock;                  //mutex to control the queue
pthread_mutex_t outlock;                //mutex to control the out stream prints
pthread_cond_t empty_cond,full_cond;    //conditions for empty and full queue
Queue queue;                            //the queue

void dirr(const char* target, vector<string>&files){
    DIR* dir;                               //pointer to directory
    struct dirent* direntry;

    if (!(dir = opendir(target))) return;   //stop recursing when target isn't a directory anymore

    while(direntry = readdir(dir)){
        string str_target = target;
        string str_dirent = direntry->d_name;
        str_target += "/";
        str_target += str_dirent;

        struct stat stats;
        if (stat(str_target.c_str(), &stats) < 0) return;

        if (direntry->d_type == DT_DIR){    //direntry is a directory
            if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0) continue;
            dirr(str_target.c_str(), files);

        }else files.push_back(str_target);  //reached a file
    }
    closedir(dir);
}

void *communication(void *param){
    int sock = *(int *)param;
    char text[BUFFER] = "";
    if (read(sock, text, BUFFER) < 0){
        perror("cant receive");
    }
    pthread_mutex_lock(&outlock);
    cout << "[Thread: " << pthread_self() << "]: About to scan directory " << text << endl;
    pthread_mutex_unlock(&outlock);
    vector<string> files;

    dirr(text,files);                               //search for directories and files recursively

    if (write(sock, to_string(files.size()).c_str(), strlen(to_string(files.size()).c_str())) < 0){
        perror("can't send count");
    }

    for (int i = 0; i<files.size();i++){
        pthread_mutex_lock(&qlock);
        while (queue.isfull()){                     //check if queue is full
            pthread_cond_wait (&full_cond, &qlock);
        }
        pthread_mutex_lock(&outlock);
        cout << "[Thread: " << pthread_self() << "]: Adding file " << files[i] << " to the queue..." << endl;
        pthread_mutex_unlock(&outlock);
        queue.push(files[i],sock);
        pthread_cond_broadcast (&empty_cond);
        pthread_mutex_unlock(&qlock);
    }
}

void *worker(void *param){
    int blocksize = *(int *)param;
    while(1){
        pthread_mutex_lock(&qlock);
        while (queue.isempty()){                    //check if queue is empty
            pthread_cond_wait (&empty_cond, &qlock);
        }
        Obj *o = queue.pop();
        pthread_cond_broadcast (&full_cond);
        pthread_mutex_unlock(&qlock);

        const char *path = o->path.c_str();
        pthread_mutex_lock(&outlock);
        cout << "[Thread: " << pthread_self() << "]: Received task: <" << path << ", " << o->sock << ">" << endl;
        pthread_mutex_unlock(&outlock);
        int f;
        if ((f = open(path, O_RDONLY)) < 0){
            perror("can't open file");
        }
        struct stat dummystat;
        fstat(f, &dummystat);
        int length = dummystat.st_size;

        char buffer[length] = "";
        const char *bp = buffer;
        pthread_mutex_lock(&outlock);
        cout << "[Thread: " << pthread_self() << "]: About to read file " << path << endl;
        pthread_mutex_unlock(&outlock);
        if (read(f,buffer,length) < 0){
            perror("can't read file");
        }

        pthread_mutex_lock(&locks[o->sock]);

        if (write(o->sock, path, BUFFER) < 0){
            perror("can't send path");
        }

        if (write(o->sock, to_string(length).c_str(), BUFFER) < 0){
            perror("can't send length");
        }

        for (int i=0; i<length; i+=blocksize){
            if (write(o->sock, bp, blocksize) < 0){
                perror("can't send block");
            }
            bp += blocksize;
        }
            
        delete o;
        pthread_mutex_unlock(&locks[o->sock]);
    }
}

int main(int argc, char *argv[]){
    
    int port;
    int size;
    int qsize;
    int bsize;
    char *bsizec;
    if (argc == 9){
        if (strcmp(argv[1],"-p") == 0){
            port = atoi(argv[2]);
        }else{
            perror("incorrect function call, terminating");
            return -1;
        }
        if (strcmp(argv[3],"-s") == 0){
            size = atoi(argv[4]);
        }else{
            perror("incorrect function call, terminating");
            return -1;
        }
        if (strcmp(argv[5],"-q") == 0){
            qsize = atoi(argv[6]);
            queue.setmax(qsize);
        }else{
            perror("incorrect function call, terminating");
            return -1;
        }
        if (strcmp(argv[7],"-b") == 0){
            bsize = atoi(argv[8]);
            bsizec = argv[8];
        }else{
            perror("incorrect function call, terminating");
            return -1;
        }
    }else{
        perror("incorrect amount of arguments, terminating.");
        return -1;
    }
    cout << "Server's parameters are:" << endl;
    cout << "port: " << port << endl;
    cout << "thread_pool_size: " << size << endl;
    cout << "queue_size: " << qsize << endl;
    cout << "Block_size: " << bsize << endl;

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket failed");
        return -1;
    }

    sockaddr_in address;
    memset(&address, '\0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr *)&address, sizeof(address)) < 0){
        perror("error binding");
        return -2;
    }
    cout << "Server was successfully initialized..." << endl;

    if(listen(sock, BACKLOG) << 0){
		perror("error binding");
        return -2;
	}
    cout << "Listening for connections to port " << port << endl;

    pthread_mutex_init(&qlock, NULL);
    pthread_mutex_init(&outlock, NULL);
    pthread_cond_init(&empty_cond, NULL);
    pthread_cond_init(&full_cond, NULL);

    vector<pthread_t> comms;
    pthread_t workers[size];

    for (int i = 0; i < size; i++){
        pthread_create(&workers[i++], NULL, worker, &bsize);
    }

    sockaddr_in address1;
    socklen_t ssize = sizeof(address1);
    int i = 0;
    while(1){
        int sock1;
        if ((sock1 = accept(sock, (struct sockaddr*)&address1, &ssize)) < 0){
            perror("failed accepting connection");
            return -1;
        }
        pthread_mutex_lock(&outlock);
        cout << "Accepted connection from localhost" << endl;
        pthread_mutex_unlock(&outlock);
        
        if (write(sock1, bsizec, sizeof(bsizec)) < 0){
            perror("error sending blocksize");
            return -1;
        }

        pthread_mutex_t tempmutex;
        locks.insert(pair<int, pthread_mutex_t>(sock1,tempmutex));
        pthread_mutex_init(&locks[sock1], NULL);
        pthread_t tempthread;
        comms.push_back(tempthread);
        pthread_create(&comms[i++], NULL, communication, &sock1);
    }
}