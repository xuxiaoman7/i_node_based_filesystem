#include"FileSystem.h"
#include"Inode.h"
#include<vector>
#include<cstddef>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string.h>
#include<cmath>
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
        //设置根目录占据的inode节点(只需要设置id，其他的属性初始化的时候会自动赋值)
        root_inode.set_id(0);
        //把inode bitmap中的第一个inode置为1
        modify_inode_bitmap(0);
        //把inode写进去(前面有超级块，两个位图)
        file.seekg(3*1024,ios::beg);
        file.write((char*)&root_inode,sizeof(root_inode));
        
        //设置当前工作目录
        cur_inode = root_inode;
    }
    //如果文件已存在
    else
    {
        //读出文件中的根目录inode
        file.seekp(3*1024,ios::beg);
        file.read((char*)&root_inode,sizeof(root_inode));
        cur_inode = root_inode;
    }
    cur_path = "/";
    //现在文件已经存在了，开始接收用户输入的指令
    handle_command();
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
    cout<<"--createFile fileName fileSize    create a file with specified filesize(in KB) which is smaller than 266KB"<<endl;
    cout<<"--deleteFile fileName             delete a file"<<endl;
    cout<<"--createDir dirName               create a directory"<<endl;
    cout<<"--deleteDir dirName               delete a directory(except the current working directory)"<<endl;
    cout<<"--changeDir path                  change the working directory"<<endl;
    cout<<"--dir                             list all the files and sub-directories under current working directory"<<endl;
    cout<<"--cp file1 file2                  copy file1 to file2"<<endl;
    cout<<"--sum                             display the usage of storage space the 16MB"<<endl;
    cout<<"--cat fileName                    print out the file contents"<<endl;
}

//将string转换为int并判断是不是纯数字
bool FileSystem::string_to_int(string s,int& size)
{
    size = 0;
    int n = s.length();
    bool isD = true;
    for(int i = 0; i < n; i++)
    {
        if(!isdigit(s[i]))
        {
            isD = false;
            break;
        }
        else
        {
            int temp = (int)(s[i]-'0');
            size += temp*(int)pow(10,n-i-1);
        }
    }
    return isD;
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

void FileSystem::handle_command()
{
    //输出当前工作目录
    cout<<cur_path<<'$';
    string command;
    
    // std::istringstream is(command);
    // string tmp;
    getline(cin,command);
    getline(cin,command);
    //while(getline(is,tmp))
    while(command != "exit")
    {
        //用来存以空格隔开的命令行
        vector<string> commands;
        //对命令行进行处理
        string::size_type i = command.find(" ",0);   //查找第一个空格所在的地方
        while (i != string::npos)
        {
            commands.push_back(command.substr(0,i));
            command = command.substr(i+1,command.length()-(i+1));
            i = command.find(" ",0);   
        }
        commands.push_back(command);
        cout<<"------"<<endl;
        for(int i = 0; i  < commands.size(); i++)
        {
            cout<<commands[i]<<endl;
        }
        cout<<"------"<<endl;

        //分析第一个到底是什么
        if(commands[0] == "createFile")
        {
           if(commands.size() != 3)
           {
               cout<<"The number of parameters should be 3!"<<endl;
           }
           else
           {
               int size;
               if(!string_to_int(commands[2],size))
               {
                   cout<<"Fail to create! File size should be a positive number!"<<endl;
               }
               else
               {
                   if(size > 266)
                   {
                       cout<<"Fail to create!Please use a smaller size!"<<endl;
                   }
                   else
                   {
                       cout<<createFile(commands[1],size)<<endl;
                   }
               }
           }
        }
        else if(commands[0] == "deleteFile")
        {
           deleteFile();
        }
        else if(commands[0] == "createDir")
        {

        }
        else if(commands[0] == "deleteDir")
        {

        }
        else if(commands[0] == "changeDir")
        {

        }
        else if(commands[0] == "dir")
        {

        }
        else if(commands[0] == "cp")
        {

        }
        else if(commands[0] == "sum")
        {

        }
        else if(commands[0] == "cat")
        {

        }
        else
        {
            cout<<"Please input the commands in correct format!"<<endl;
        }
        cout<<cur_path<<'$';
        getline(cin,command);
    }

}

//返回成功创建的信息或错误信息
std::string FileSystem::createFile(string filepath,int size)
{
    Inode temp_cur_inode;
    if(filepath[0] == '/')
    {
        temp_cur_inode = root_inode;
        filepath = filepath.substr(1,filepath.length()-1);
    }
    else
    {
        temp_cur_inode = cur_inode;
    }

    //分割路径和文件名
    string::size_type i = filepath.find('/',0);
    vector<string> dirs;
    while(i != string::npos)
    {
        dirs.push_back(filepath.substr(0,i));
        filepath = filepath.substr(i+1,filepath.length()-(i+1));
        i = filepath.find('/',0);
    }
    //现在的filepath就是文件名了
    string filename = filepath;
    //没有输入文件名
    if(filename == "")
    {
        return "Please input the filename!";
    }
    
    //开始查找路径是否存在
    for(int i = 0; i < dirs.size(); i++)
    {
        Inode temp_inode = findInode(temp_cur_inode,dirs[i]);
        if(temp_inode.get_id() == -1)
        {
            return "Path is not exist!";
        }
        else
        {
            temp_cur_inode = temp_inode;
        }
    }

    //判断文件名存不存在
    Inode temp_inode = findInode(temp_cur_inode,filename);
    if(temp_inode.get_id() != -1)
    {
        return "File has been exist!";
    }

    //判断inode还有没有得用
    
    //判断目录下还有没有空间

    //判断data block里剩余空间还够不够

    

    return "Create successfully!";
}

Inode FileSystem::findInode(Inode inode,string filename)
{
    Inode resultInode;
    return resultInode;
}

void FileSystem::deleteFile()
{
  cout<<"no problem"<<endl;
}
