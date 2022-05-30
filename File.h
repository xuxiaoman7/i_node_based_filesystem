#ifndef FILE_H
#define FILE_H

#define MAX_FILENAME_LENGTH 12

class File
{
private:
    char FileName[MAX_FILENAME_LENGTH];            
    int InodeID;
public:
    File();
    File(char*,int);
    ~File();
    void set_InodeID(int);
    void set_FileName(char*);
    int get_InodeID();
    char* get_FileName();
};


#endif
