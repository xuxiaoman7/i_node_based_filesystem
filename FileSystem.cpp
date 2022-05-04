#include"FileSystem.h"
#include<iostream>
#include<fstream>
#include<string.h>
using namespace std;
char buffer[1024];
FileSystem::FileSystem()
{

}
FileSystem::~FileSystem()
{

}
void FileSystem::initial()
{
    welcome();
    //开始初始化
    //打开文件
    file.open("filesystem.txt",ios::in|ios::out);
    //ifstream fin("filesystem.txt",ios::in|ios::binary);
    //如果文件不存在的话
    if(!file.is_open())
    //if(!fin.is_open())
    {
        //创建文件，并且把文件扩充为16MB
        ofstream fout("filesystem.txt");
        if(!fout)                        //如果创建失败
        {
            cout<<"Fail to initialize file system..."<<endl;
        }
        //把文件扩充成16MB
        for(int i = 0; i < 16384; i++)                     //不能直接用一个16MB的buffer，因为程序可能支撑不了这么大的数组
        {
            fout.write(buffer,sizeof(buffer));             //一个buffer1024字节，文件系统一共16MB，所以写16384次
        }
        
        //初始化超级块并把超级块写进去(读取的时候把char*强制转换成struct就可以了)
        fout.seekp(0,ios::beg);
        fout.write((char*)&superBlock,sizeof(superBlock));

        //把inode位图写进去(全部初始化为0)
        fout.seekp(1024,ios::beg);       //指针移到第二个块上
        for(int i = 0; i < 1024; i++)
        {
            char byte;           //一个字节
            memset(&byte,0,1);
            fout.write(&byte,sizeof(char));
        }

        //把block位图写进去
        fout.seekp(2*1024,ios::beg);     //指针移到第三个块上
        for(int i = 0; i < 1024; i++)
        {
            char byte;           //一个字节
            memset(&byte,0,1);
            fout.write(&byte,sizeof(char));
        }
        //关掉ofstream，用全局变量fstream来打开文件
        fout.close();
        file.open("filesystem.txt",ios::in|ios::out);

        //把block位图中的前三块分别置为1(super block, inode bitmap, block bitmap)
        modify_block_bitmap(0);
        modify_block_bitmap(1);
        modify_block_bitmap(2);
        
        //把block位图中的inode table占据的块也设置为1(1024块)
        for(int i = 3; i < 1027; i++)
        {
            modify_block_bitmap(i);
        }

        //设置根目录
        //设置根目录占据的inode节点
    }
}
void FileSystem::welcome()
{
    cout<<"Welcome to Our File System!"<<endl;
    cout<<"Name            StudentID"<<endl;
    cout<<"许晓漫          201930344213"<<endl;
    cout<<"姚禧            201930330247"<<endl;
    cout<<"For more information,please enter help"<<endl;
    string s;
    cin>>s;
    if(s == "help")
    {
        help();
    }
}
void FileSystem::help()
{
    cout<<"--createFile fileName fileSize    create a file"<<endl;
    cout<<"--deleteFile fileName             delete a file"<<endl;
    cout<<"--createDir dirName               create a directory"<<endl;
    cout<<"--deleteDir dirName               delete a directory(except the current working directory)"<<endl;
    cout<<"--changeDir path                  change the working directory"<<endl;
    cout<<"--dir                             list all the files and sub-directories under current working directory"<<endl;
    cout<<"--cp file1 file2                  copy file1 to file2"<<endl;
    cout<<"--sum                             display the usage of storage space the 16MB"<<endl;
    cout<<"--cat fileName                    print out the file contents"<<endl;
}

//修改block bitmap中的某一个比特位(参数：哪一位)
void FileSystem::modify_block_bitmap(int position)
{
    int byte_position = position/8;      //表示当前这个要修改的位在哪个字节上
    int bit_position = position%8;       //表示当前这个要修改的位在字节上的哪个位置
    file.seekg(2*1024+byte_position,ios::beg);
    char a;
    file.read(&a,sizeof(a));             //读出那个需要修改的字节
    a = (a^(1<<bit_position));
    file.seekp(2*1024+byte_position,ios::beg);
    file.write(&a,sizeof(a));
}

//修改inode bitmap中的某一个比特位(参数：哪一位)
void FileSystem::modify_inode_bitmap(int position)
{
    int byte_position = position/8;
    int bit_position = position%8;
    file.seekg(1024+byte_position,ios::beg);
    char a;
    file.read(&a,sizeof(a));
    a = (a^(1<<bit_position));
    file.seekp(1024+byte_position,ios::beg);
    file.write(&a,sizeof(a));
}