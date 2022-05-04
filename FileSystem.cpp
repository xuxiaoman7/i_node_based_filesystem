#include"FileSystem.h"
#include"Inode.h"
#include<vector>
#include<cstddef>
#include<iostream>
#include<fstream>
#include<sstream>
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
                cout<<"Please input the commands in correct format!"<<endl;
            }
            else
            {
                //如果想要创建的文件大小大于266kB，则驳回
                if(commands[2] > "266")
                {
                    cout<<"Fail to create! Please a smaller size!"<<endl;
                }
                else
                {
                    Inode temp_cur_inode; //记录创建文件的路径的节点
                    string filename = commands[1];
                    //判断是绝对路径还是相对路径
                    if(filename[0] == '/')              //绝对路径
                    {
                        temp_cur_inode = root_inode;
                        filename = filename.substr(1,filename.size()-1);
                    }
                    else                                //相对路径
                    {
                        temp_cur_inode = cur_inode;
                    }
                    //分割路径和文件名
                    string::size_type i = filename.find('/',0);
                    vector<string> names;
                    while(i != string::npos)
                    {
                        names.push_back(filename.substr(0,i));
                        filename = filename.substr(i+1,filename.length()-(i+1));
                        i = filename.find('/',0);
                    }
                    //文件名
                    string name = filename;
                    bool isValidPath = true;
                    //如果路径是空的
                    if(names.empty())
                    {
                        //说明就是根目录下，那路径一定存在
                        isValidPath = true;
                    }
                    else
                    {
                        //说明不是根目录下，要判断路径存不存在(从当前这个inode开始往下找)
                        //判断当前inode下有没有name[0]
                        //读取inode对应的blcok里的东西
                        //读inode里的地址
                        vector<int> direct_address;
                        for(int i = 0; i < 10; i++)
                        {
                            int t = temp_cur_inode.get_direct_block_address()[i];
                            if(t != -1)
                            {
                                direct_address.push_back(t);
                            }
                            else
                            {
                                break;
                            }
                        }
                        if(direct_address.size() == 10)
                        {
                            //判断一下间接地址
                        }
                    }
                    
                    //判断一下有没有文件重名

                }
            }
        }
        else if(commands[0] == "deleteFile")
        {

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