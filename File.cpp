#include"File.h"
#include<string.h>
File::File()
{
    strcpy(FileName,"");
    InodeID = -1;
}

File::File(char* fileName,int InodeID)
{
    strcpy(this->FileName,fileName);
    this->InodeID = InodeID;
}

File::~File()
{
}

void File::set_InodeID(int InodeID)
{
    this->InodeID = InodeID;
}
void File::set_FileName(char* FileName)
{
    strcpy(this->FileName,FileName);
}
int File::get_InodeID()
{
    return InodeID;
}
char* File::get_FileName()
{
    return FileName;
}