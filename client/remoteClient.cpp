#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER 1024

using namespace std;

void mkdris(string dir){
    string makeme;
    size_t len = dir.length();
    int pos;

    while(1){
        pos = dir.find("/");
        if (pos < 0) break;
        makeme += dir.substr(0,pos+1);
        mkdir(makeme.c_str(), 0777);
        dir = dir.substr(pos+1);
    }
}

void mkfiles(const char *name ,const char *data, int size){
    int d;
    remove(name);       //if file already exists, it wil be removed to make way for the new one
    if ((d = open(name , O_WRONLY | O_CREAT , 0777)) < 0){
        perror("cant make file");
    }
    if (write(d , data , size) < 0){
        perror("cant write in file");
    }
}


int main(int argc, char *argv[]){
    
    const char *ip;
    int port;
    char *dir;
    if (argc == 7){
        if (strcmp(argv[1],"-i") == 0){
            ip = argv[2];
        }else{
           perror("incorrect function call, terminating");
            return -1;
        }
        if (strcmp(argv[3],"-p") == 0){
            port = atoi(argv[4]);
        }else{
            perror("incorrect function call, terminating");
            return -1;
        }
        if (strcmp(argv[5],"-d") == 0){
            dir = argv[6];
        }else{
            perror("incorrect function call, terminating");
            return -1;
        }
    }else{
        perror("incorrect amount of arguments, terminating.");
        return -1;
    }
    cout << "Client's parameters are:" << endl;
    cout << "serverIP: " << ip << endl;
    cout << "port: " << port << endl;
    cout << "directory: " << dir << endl;

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket failed");
        return -1;
    }

    sockaddr_in address;
    memset(&address, '\0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0){
		perror("error connecting");
        return -2;
    }
    cout << "Connecting to " << ip <<" on port " << port << endl;

    char text[BUFFER] = "";
    char bsize[BUFFER] = "";
    char cnt[BUFFER] = "";
    strcat(text,dir);
    if (write(sock, text, strlen(text)) < 0){
        perror("error sending to server");
        return -1;
    }

    if (read(sock, bsize, BUFFER) < 0){
        perror("error receiving from server");
        return -1;
    }

    if (read(sock, cnt, BUFFER) < 0){
        perror("error receiving from server");
        return -1;
    }

    for (int i = 0; i < atoi(cnt); i++){
        char ptext[BUFFER] = "";
        char ltext[BUFFER] = "";
        if (read(sock, ptext, BUFFER) < 0){
            perror("error receiving from server");
            return -1;
        }
        cout << "Received: " << ptext << endl;

        int k = strlen(dir)-1;
        while(k != 0){                  //cuts off unnecessary dirs
            if(dir[k] == '/')
                break;
            k--;
        }
        string pstring = ptext;
        string substring;
        substring = pstring.substr(k, pstring.length());
        string fixeddir;                //ensures ./ at the start of the path
        if (substring[0] == '.'){
            fixeddir = "";
        }else if (substring[0] == '/'){
            fixeddir = ".";
        }else{
            fixeddir = "./";
        }
        fixeddir += substring;

        int count = 1;
        k = strlen(ptext)-1;
        while(k != 0){                  //cuts off the file name
            if(ptext[k] == '/')
                break;
            count++;
            k--;
        }
        substring = fixeddir.substr(0,fixeddir.length()-count+1);

        mkdris(substring);

        if (read(sock, ltext, BUFFER) < 0){
            perror("error receiving from server");
            return -1;
        }
        int length = atoi(ltext);
        string data;
        for (int j = 0; j<length; j+=atoi(bsize)){
            char ftext[BUFFER] = "";
            if (read(sock, ftext, atoi(bsize)) < 0){
                perror("error receiving from server");
                return -1;
            }
            data += ftext;
        }
        mkfiles(fixeddir.c_str(), data.c_str(), length);
    }
    return 0;
}