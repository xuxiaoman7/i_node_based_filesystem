#include"FileSystem.h"
#include"Inode.h"
#include"File.h"
#include<vector>
#include<cstddef>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string.h>
#include<cmath>
#include<iomanip>
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
        cout<<"Initialize the file system successfully!"<<endl;
        //把文件扩充成16MB
        for(int i = 0; i < 16384; i++)                     //不能直接用一个16MB的buffer，因为程序可能支撑不了这么大的数组
        {
            fout.write(buffer,sizeof(buffer));             //一个buffer1024字节，文件系统一共16MB，所以写16384次
        }
        
        //初始化超级块并把超级块写进去(读取的时候把char*强制转换成struct就可以了)
        fout.seekp(0,ios::beg);
        fout.write((char*)&superBlock,sizeof(superBlock));

        //把inode位图写进去(全部初始化为0)
        fout.seekp(BLOCK_SIZE,ios::beg);       //指针移到第二个块上
        for(int i = 0; i < INODE_BITMAP_SIZE; i++)
        {
            char byte;           //一个字节
            memset(&byte,0,1);
            fout.write(&byte,sizeof(char));
        }
        //把block位图写进去
        fout.seekp(BLOCK_SIZE+INODE_BITMAP_SIZE,ios::beg);     //指针移到第三个块上
        for(int i = 0; i < BLOCK_BITMAP_SIZE; i++)
        {
            char byte;           //一个字节
            memset(&byte,0,1);
            fout.write(&byte,sizeof(char));
        }
        //关掉ofstream，用全局变量fstream来打开文件
        fout.close();
        file.open("filesystem.txt",ios::in|ios::out);

        file.seekg(BLOCK_SIZE,ios::beg);
        for(int i= 0 ; i < 1024; i++)
        {
            char c;
            file.read(&c,sizeof(char));
        }

        //把block位图中的前四块分别置为1(super block, inode bitmap, block bitmap)
        modify_block_bitmap(0);
        modify_block_bitmap(1);
        modify_block_bitmap(2);
        modify_block_bitmap(3);
        superBlock.used_block_count += 4;
        
        //把block位图中的inode table占据的块也设置为1(1024块)
        for(int i = 4; i < INODE_TABLE_SIZE/BLOCK_SIZE+4; i++)
        {
            modify_block_bitmap(i);
        }
        superBlock.used_block_count += 1024;

        //设置根目录
        //设置根目录占据的inode节点(只需要设置id，其他的属性初始化的时候会自动赋值)
        root_inode.set_id(0);
        //把inode bitmap中的第一个inode置为1
        modify_inode_bitmap(0);
        //在超级块中修改使用的inode数
        superBlock.used_inode_count++;
        //把inode写进去
        file.seekp(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE,ios::beg);
        file.write((char*)&root_inode,sizeof(INODE_SIZE));
        
        //设置当前工作目录
        cur_inode = root_inode;
    }
    //如果文件已存在
    else
    {
        cout<<"FileSystem is loading..."<<endl;
        //把superBlock读出来
        file.seekg(0,ios::beg);
        file.read((char*)&superBlock,sizeof(superBlock));
        //读出文件中的根目录inode
        file.seekg(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE,ios::beg);
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
    file.seekg(BLOCK_SIZE+INODE_BITMAP_SIZE+byte_position,ios::beg);
    char a;
    file.read(&a,sizeof(a));             //读出那个需要修改的字节
    a = (a^(1<<bit_position));
    file.seekp(BLOCK_SIZE+INODE_BITMAP_SIZE+byte_position,ios::beg);
    file.write(&a,sizeof(a));
}

//修改inode bitmap中的某一个比特位(参数：哪一位)
void FileSystem::modify_inode_bitmap(int position)
{
    int byte_position = position/8;
    int bit_position = position%8;
    file.seekg(BLOCK_SIZE+byte_position,ios::beg);
    char a;
    file.read(&a,sizeof(a));
    a = (a^(1<<bit_position));
    file.seekp(BLOCK_SIZE+byte_position,ios::beg);
    file.write(&a,sizeof(a));
}

void FileSystem::handle_command()
{
    //输出当前工作目录
    cout<<cur_path<<'$'<<' ';
    string command;
    
    // std::istringstream is(command);
    // string tmp;
    getline(cin,command);
    getline(cin,command);
    //while(getline(is,tmp))
    while(1)
    {
        //每次进来都重新读一下cur_inode，防止在创建过程中改变了
        //cout<<"下一轮命令id"<<cur_inode.get_id()<<"几个"<<cur_inode.get_file_count()<<endl;
        cur_inode = get_inode_byID(cur_inode.get_id());
        //cout<<"读完几个"<<cur_inode.get_file_count()<<endl;
        root_inode = get_inode_byID(0);
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
            if(commands.size() != 2)
            {
                cout<<"The number of parameters should be 2!"<<endl;
            }
            else
            {
                cout<<createDir(commands[1])<<endl;
            }
        }
        else if(commands[0] == "deleteDir")
        {

        }
        else if(commands[0] == "changeDir")
        {

        }
        else if(commands[0] == "dir")
        {
            if(commands.size() != 1)
            {
                cout<<"The number of parameters should be 1!"<<endl;
            }
            else
            {
                cout<<dir_list()<<endl;
            }
        }
        else if(commands[0] == "cp")
        {

        }
        else if(commands[0] == "sum")
        {
            if(commands.size() != 1)
            {
                cout<<"The number of parameters should be 1!"<<endl;
            }
            else
            {
                sum();
            }

        }
        else if(commands[0] == "cat")
        {
            if(commands.size() != 2)
            {
                cout<<"The number of parameters should be 2!"<<endl;
            }
            else
            {
                cout<<catFile(commands[1])<<endl;
            }
        }
        else if(commands[0] == "help")
        {
            help();
        }
        else if(commands[0] == "exit")
        {
            // cout<<"退出之前读一下"<<endl;
            // cout<<superBlock.used_block_count<<" "<<superBlock.used_inode_count<<endl;
            //把super block写回去
            file.seekp(0,ios::beg);
            file.write((char *)&superBlock,sizeof(superBlock));
            // cout<<"写进去之后读出来kk"<<endl;
            // struct superblock temp;
            // file.seekg(0,ios::beg);
            // file.read((char*)&temp,sizeof(temp));
            // cout<<temp.used_block_count<<" "<<temp.used_inode_count<<endl;
            exit(0);
        }
        else
        {
            cout<<"Please input the commands in correct format!"<<endl;
        }
        cout<<cur_path<<'$'<<' ';
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
    // string::size_type i = filepath.find('/',0);
    // vector<string> dirs;
    // while(i != string::npos)
    // {
    //     dirs.push_back(filepath.substr(0,i));
    //     filepath = filepath.substr(i+1,filepath.length()-(i+1));
    //     i = filepath.find('/',0);
    // }
    vector<string> dirs = segment_path(filepath);
        //分割完的东西
    cout<<"分割完的东西"<<endl;
    for(int i = 0; i< dirs.size(); i++)
    {
        cout<<dirs[i];
    }
    //把最后一个文件名拿出来
    string filename = dirs[dirs.size()-1];
    //没有输入文件名
    if(filename == "")
    {
        return "Please input the filename!";
    }
    
    //开始查找路径是否存在
    // for(int i = 0; i < dirs.size()-1; i++)
    // {
    //     Inode temp_inode = findInode(temp_cur_inode,dirs[i]);
    //     if(temp_inode.get_id() == -1)
    //     {
    //         return "Path is not exist!";
    //     }
    //     else if(temp_inode.get_file_type() == FILE_TYPE)           //找到的是文件而非目录
    //     {
    //         return "Path is not exist!";
    //     }
    //     else
    //     {
    //         temp_cur_inode = temp_inode;
    //     }
    // }
    Inode temp = isValidPath(dirs,temp_cur_inode);
    if(temp.get_id() == -1)
    {
        return "Path is not exist!";
    }
    else
    {
        temp_cur_inode = temp;
    }

    //判断文件名存不存在
    //cout<<"判断文件名存不存在"<<endl;
    //cout<<"当前temp_cur_inode id为"<<temp_cur_inode.get_id()<<endl;
    Inode temp_inode3 = findInode(temp_cur_inode,filename);
    if(temp_inode3.get_id() != -1)
    {
        return "File has been exist!";
    }

    //判断inode还有没有得用
    if(superBlock.inode_count == superBlock.used_inode_count)
    {
        return "You have created too much file! Please delete some files and try again.";
    }
    //判断目录下还有没有空间
    if(temp_cur_inode.get_file_count() >= MAX_FILE_COUNT_PER_DIRECTORY)
    {
        return "You have created too much file in current path! Please delete some and retry.";
    }
    //判断data block里剩余空间还够不够
    if(superBlock.block_count == superBlock.used_block_count)
    {
        return "You have created too much file! Please delete some files and try again.";
    }
    //开始创建文件
    File file_data;
    char filen[MAX_FILENAME_LENGTH];
    strcpy(filen,filename.c_str());
    //cout<<"现在写进去的文件名叫"<<filen<<endl;
    file_data.set_FileName(filen);
    //根据bitmap寻找没用到的inode
    int id = find_free_inode();
    //cout<<"找到的inodeid为"<<id<<endl;
    file_data.set_InodeID(id);
    modify_inode_bitmap(id);
    superBlock.used_inode_count++;
    //把文件名和inode写进当前目录(目录也会被更新后写进文件系统)  ！这里需要把inode也更新，传值进去没用
    add_file_to_dir(file_data,temp_cur_inode);
    //根据bitmap中block的使用情况来分配block
    //因为size的大小是KB，所以size是多大就需要用几块    //该文件需要用到多少块
    int block_count = size;
    int addr[10] = {0};
    int indirect_addr = -1;
    vector<int> indirect;
    for(int i = 0; i < block_count; i++)
    {
        //找空闲的block块
        if(i < 10)
        {
            addr[i] = find_free_block();
            modify_block_bitmap(addr[i]);
            superBlock.used_block_count++;
        }
        else
        {
            int temp_block_id = find_free_block();
            indirect.push_back(temp_block_id);
            modify_block_bitmap(temp_block_id);
            superBlock.used_block_count++;
        }
    }
    int min_block = min(10,block_count);
    for(int i = 0; i < min_block; i++)
    {
        write_random_string_to_file(addr[i]);
    }
    if(!indirect.empty())
    {
        //找一块来存放间接地址所指向的块
        int temp_block_id = find_free_block();
        indirect_addr = temp_block_id;
        modify_block_bitmap(temp_block_id);
        superBlock.used_block_count++;
        //把vector里的地址数据添进indirect_addr所指向的data block
        file.seekp(indirect_addr*BLOCK_SIZE,ios::beg);
        for(int i = 0; i < indirect.size();i++)
        {
            file.write((char*)&indirect[i],sizeof(int));
        }

        //把地址对应的data block写上随机数据
        for(int i = 0; i < indirect.size(); i++)
        {
            write_random_string_to_file(indirect[i]);
        }
    }
    
    Inode finode;
    finode.set_id(id);
    finode.set_byte_size(size*(int)pow(2,10));       //把KB转化成B
    finode.set_created_time(time(0));
    finode.set_modified_time(time(0));
    finode.set_file_type(FILE_TYPE);

    finode.set_direct_block_address(addr);
    finode.set_indirect_block_address(indirect_addr);

    //把文件对应的inode写进inode table里
    file.seekp(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE+id*INODE_SIZE,ios::beg);
    file.write((char*)&finode,sizeof(Inode));

    return "Create successfully!";
}

//在给定的inode下查找给定的filename是否存在，如果不存在，则返回id为-1的初始化inode，否则返回对应inode
Inode FileSystem::findInode(Inode inode,string filename)
{
    Inode resultInode;
    resultInode.set_id(-1);
    int file_count = inode.get_file_count();
    //读取inode的data block里的file
    int file_per_block = BLOCK_SIZE/sizeof(File);      
    int block_count;
    if(file_count%file_per_block == 0)
    {
        block_count = file_count/file_per_block;
    } 
    else
    {
        block_count = file_count/file_per_block+1;
    }
    //int block_count = file_count/file_per_block+1;      //一共用了几块data block
    int min_block = min(block_count,10);
    //先遍历前10个或更少的块
    int addr[10];
    memcpy(addr,inode.get_direct_block_address(),10*4);  //int占4个字节
    for(int i = 0; i < min_block; i++)
    {
        if(file_count<file_per_block)
        {
            //读file_count个文件
            for(int j = 0; j < file_count; j++)
            {
                file.seekg(addr[i]*BLOCK_SIZE+j*FILE_SIZE,ios::beg);
                File testFile;
                file.read((char*)&testFile,sizeof(File));
                //这里读不出出来，不知道为什么！
                cout<<"现在找到的文件inodeid为"<<testFile.get_InodeID()<<endl;
                cout<<"现在找到的文件名为"<<testFile.get_FileName()<<endl;
                cout<<filename.c_str()<<endl;
                if(strcmp(testFile.get_FileName(),filename.c_str()) == 0)
                {
                    //找到了
                    cout<<"相等了"<<endl;
                    return get_inode_byID(testFile.get_InodeID());
                }
            }
        }
        else
        {
            //读file_per_block个文件
            for(int j = 0; j < file_per_block; j++)
            {
                File file_data;
                file.seekg(addr[i]*BLOCK_SIZE,ios::beg);
                file.read((char*)&file_data,sizeof(File));
                if(strcmp(file_data.get_FileName(),filename.c_str()) == 0)
                {
                    //找到了
                    return get_inode_byID(file_data.get_InodeID());
                }
            }
            file_count -= file_per_block;
        }
    }
    //如果是10个块以上的话，继续遍历
    if(block_count > 10)
    {

    }
    
    return resultInode;
}

void FileSystem::deleteFile()
{
  cout<<"no problem"<<endl;
}

Inode FileSystem::get_inode_byID(int id)
{
    //前面有3个位图
    file.seekg(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE+id*INODE_SIZE,ios::beg);
    Inode inode;
    file.read((char*)&inode,sizeof(inode));
    return inode;
}

int FileSystem::find_free_inode()
{
    file.seekg(1*BLOCK_SIZE,ios::beg);
    char byte;
    //遍历整个位图
    cout<<"找空闲inode"<<endl;
    for(int i = 0; i < INODE_BITMAP_SIZE; i++)
    {
        file.read(&byte,sizeof(char));
        cout<<"读到的"<<(int)byte;
        //判断
        for(int j = 0; j < 8; j++)
        {
            //如果当前位是0，则返回1，否则返回0
            if((~(byte>>j))&1)
            {
                return i*8+j;   //返回id
            }
        }
    }
    cout<<"找完了"<<endl;
    //说明找不到,但一般不会，因为在此之前需要先通过superBlock判断有没有inode可用
    return -1;
}

 //往目录中加入文件名和文件id
void FileSystem::add_file_to_dir(File& file_data,Inode& inode)
{
    cout<<"现在要把文件往目录项里写了，文件名为"<<file_data.get_FileName()<<endl;
    int byte = inode.get_byte_size();
    byte += sizeof(File);
    inode.set_byte_size(byte);

    int addr[10];
    memcpy(addr,inode.get_direct_block_address(),10*4);

    int file_count = inode.get_file_count();
    if(file_count < 10*FILE_PER_BLOCK)
    {
        //查找block对应的地址
        int i = file_count/FILE_PER_BLOCK;
        int j = file_count%FILE_PER_BLOCK;
        //说明需要一个新块，需要给addr[i]一个新的data block块地址
        if(j == 0)
        {
            addr[i] = find_free_block();
            modify_block_bitmap(addr[i]);
            superBlock.used_block_count++;
            inode.set_direct_block_address(addr);
        }
        cout<<"写这块"<<addr[i]<<endl;
        cout<<"第几个文件"<<j<<endl;
        file.seekp(addr[i]*BLOCK_SIZE+j*FILE_SIZE,ios::beg);
        file.write((char*)&file_data,sizeof(File));
        File testFile;
        file.seekg(addr[i]*BLOCK_SIZE+j*FILE_SIZE,ios::beg);
        file.read((char*)&testFile,sizeof(File));
        cout<<"现在写进去了，再读出来看看"<<testFile.get_FileName()<<endl;
    }
    //找间接地址上的地址
    else
    {

    }
    file_count++;
    inode.set_file_count(file_count);
    
    inode.set_modified_time(time(0));

    //把inode写回去
    save_inode(inode);
    //写完读一下
    cout<<"id是多少"<<inode.get_id()<<" "<<inode.get_file_count()<<endl;
    cout<<"写完读一下"<<get_inode_byID(inode.get_id()).get_file_count()<<endl;
}

//根据blockmap找空闲的block,并返回data block id
int FileSystem::find_free_block()
{
    file.seekg(2*BLOCK_SIZE,ios::beg);
    char byte;
    //遍历整个位图
    for(int i = 0; i < BLOCK_BITMAP_SIZE; i++)
    {
        file.read(&byte,sizeof(char));
        //判断
        for(int j = 0; j < 8; j++)
        {
            //如果当前位是0，则返回1，否则返回0
            if((~(byte>>j))&1)
            {
                return i*8+j;   //返回id
            }
        }
    }
    //说明找不到,但一般不会，因为在此之前需要先通过superBlock判断有没有inode可用
    return -1;
}

//将inode写回文件
void FileSystem::save_inode(Inode inode)
{
    //cout<<sizeof(inode)<<endl;
    int id = inode.get_id();
    file.seekp(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE+id*INODE_SIZE,ios::beg);
    file.write((char*)&inode,sizeof(inode));
    cout<<"现在把目录inode保存回去，id"<<id<<"目录下有几个"<<inode.get_file_count()<<endl;
    //前面有3个位图
    Inode tinode;
    //file.seekg(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE+id*INODE_SIZE,ios::beg);
    //file.read((char*)&tinode,sizeof(tinode));
    //cout<<"t"<<tinode.get_id()<<tinode.get_file_count()<<endl;
}

//往新建的文件里写入xx字节的数据
void FileSystem::write_random_string_to_file(int block_id)
{
   
    file.seekp(block_id*BLOCK_SIZE,ios::beg);
    file.write((char*)&buffer,sizeof(buffer));
}

string FileSystem::createDir(string dirPath)
{
    Inode temp_cur_inode;
    //查找路径存不存在
    if(dirPath[0] == '/')
    {
        temp_cur_inode = root_inode;
        dirPath = dirPath.substr(1,dirPath.length()-1);
    }
    else
    {
        temp_cur_inode = cur_inode;
    }
    // //分割路径和目录名
    // string::size_type i = dirPath.find('/',0);
    // vector<string> dirs;
    // while(i != string::npos)
    // {
    //     dirs.push_back(dirPath.substr(0,i));
    //     dirPath = dirPath.substr(i+1,dirPath.length()-(i+1));
    //     i = dirPath.find('/',0);
    // }
    // //现在的dirpath就是目录名了
    // string dirname = dirPath;
    vector<string> dirs = segment_path(dirPath);
    string dirname = dirs[dirs.size()-1];
    //没有输入目录名
    if(dirname == "")
    {
        return "Please input the directory name!";
    }

    //开始查找路径是否存在
    // for(int i = 0; i < dirs.size(); i++)
    // {
    //     Inode temp_inode = findInode(temp_cur_inode,dirs[i]);
    //     if(temp_inode.get_id() == -1)
    //     {
    //         return "Path is not exist!";
    //     }
    //     else if(temp_inode.get_file_type() == FILE_TYPE)           //找到的是文件而非目录
    //     {
    //         return "Path is not exist!";
    //     }
    //     else
    //     {
    //         temp_cur_inode = temp_inode;
    //     }
    // }
    Inode temp = isValidPath(dirs,temp_cur_inode);
    if(temp.get_id() == -1)
    {
        return "Path is not exist!";
    }
    else
    {
        temp_cur_inode = temp;
    }

    //判断要创建的目录名存不存在
    Inode temp_inode = findInode(temp_cur_inode,dirname);
    if(temp_inode.get_id() != -1)
    {
        return "Directory has been exist!";
    }

    //判断inode还有没有得用
    if(superBlock.inode_count == superBlock.used_inode_count)
    {
        return "You have created too many files! Please delete some files and try again.";
    }
    //判断目录下还有没有空间
    if(temp_cur_inode.get_file_count() >= MAX_FILE_COUNT_PER_DIRECTORY)
    {
        return "You have created too many files in current path! Please delete some and retry.";
    }
    //判断data block里剩余空间还够不够
    if(superBlock.block_count == superBlock.used_block_count)
    {
        return "You have created too many files! Please delete some files and try again.";
    }

    //开始创建目录
    File file_data;
    char dirn[MAX_FILENAME_LENGTH];
    strcpy(dirn,dirname.c_str());
    cout<<"写进去的目录名叫"<<dirn<<endl;
    file_data.set_FileName(dirn);
    //根据bitmap寻找没用到的inode
    int id = find_free_inode();
    file_data.set_InodeID(id);
    modify_inode_bitmap(id);
    superBlock.used_inode_count++;
    //把目录名和inode写进当前目录(目录也会被更新后写进文件系统)
    add_file_to_dir(file_data,temp_cur_inode);
    
    //创建dir node对应的inode
    Inode dinode;
    dinode.set_file_type(DIRECTORY_TYPE);
    dinode.set_id(id);
    //把inode写进去
    file.seekp(BLOCK_SIZE+INODE_BITMAP_SIZE+BLOCK_BITMAP_SIZE+id*INODE_SIZE,ios::beg);
    file.write((char*)&dinode,sizeof(Inode));

    return "Create successfully!";
}


//输出文件内容（cat）
string FileSystem::catFile(string filePath)
{
    Inode temp_cur_inode;
    if(filePath[0] == '/')
    {
        temp_cur_inode = root_inode;
        filePath = filePath.substr(1,filePath.length()-1);
    }
    else
    {
        temp_cur_inode = cur_inode;
    }
    //分割路径和文件名
    cout<<"路径"<<filePath<<endl;
    vector<string> dirs = segment_path(filePath);
    string filename = dirs[dirs.size()-1];
    //如果没有输入文件名
    if(filename == "")
    {
        return "Please input the filename!";
    }
    //判断路径是否存在
    Inode temp = isValidPath(dirs,temp_cur_inode);
    if(temp.get_id() == -1)
    {
        return "Path is not exist!";
    }
    else
    {
        temp_cur_inode = temp;
    }
    //判断文件名存在吗
    Inode temp_inode = findInode(temp_cur_inode,filename);
    if(temp_inode.get_id() == -1)
    {
        return "File is not exist!";
    }
    else if(temp_inode.get_file_type() == DIRECTORY_TYPE)
    {
        return "It is not a file!";
    }
    else
    {
        temp_cur_inode = temp_inode;
    }
    //现在的temp_cur_inode就是文件的inode
    int addr[10];
    memcpy(addr,temp_cur_inode.get_direct_block_address(),10*4);
    int byte_size = temp_cur_inode.get_byte_size();
    //计算会用几个block
    int block_count = byte_size/BLOCK_SIZE;
    int min_block = min(10,block_count);
    for(int i = 0; i < min_block; i++)
    {
        file.seekg(addr[i]*BLOCK_SIZE,ios::beg);
        char buffer[BLOCK_SIZE];
        file.read((char*)buffer,BLOCK_SIZE);
        cout<<buffer;
        cout<<"读";
    }
    if(block_count > 10)
    {
        //读取间接地址
        int indirect_addr = temp_cur_inode.get_indirect_block_address();
        if(indirect_addr == -1)
        {
            return "error";
        }
        int remain_block = block_count-10;     //还有几个地址要读
        int remain_addr[remain_block];
        //找到存地址的block
        file.seekg(indirect_addr*BLOCK_SIZE,ios::beg);
        for(int i = 0; i < remain_block; i++)
        {
            file.read((char*)&remain_addr[i],sizeof(int));
        }
        //读对应地址里的block
        for(int i = 0; i < remain_block; i++)
        {
            file.seekg(remain_addr[i]*BLOCK_SIZE,ios::beg);
            char buffer[BLOCK_SIZE];
            file.read((char*)buffer,sizeof(buffer));
            cout<<buffer;   
            cout<<"读";
        }
    }
    cout<<endl;
    return "That is all of the file";
}

//分割路径和文件名
vector<string> FileSystem::segment_path(string filePath)
{
    string::size_type i = filePath.find('/',0);
    vector<string> dirs;
    while(i != string::npos)
    {
        dirs.push_back(filePath.substr(0,i));
        filePath = filePath.substr(i+1,filePath.length()-(i+1));
        i = filePath.find('/',0);
    }
    //现在filePath就是最后一个文件名/目录名了
    dirs.push_back(filePath);
    return dirs;
}

//查找路径是否存在(传入当前目录的inode，传入路径的各级目录，如果存在，返回最后一个目录的inode，如果不存在，返回id为-1的inode)
Inode FileSystem::isValidPath(vector<std::string>& dirs,Inode inode)
{
    Inode resultInode;
    resultInode.set_id(-1);
    //因为dirs里还有个最后的文件名/目录名，所以要减1
    for(int i = 0; i < dirs.size()-1; i++)
    {
        Inode temp_inode = findInode(inode,dirs[i]);
        if(temp_inode.get_id() == -1)
        {
            cout<<dirs[i]<<"在当前目录下不存在"<<endl;
            return resultInode;
        }
        else if(temp_inode.get_file_type() == FILE_TYPE)           //找到的是文件而非目录
        {
            cout<<dirs[i]<<"是文件而非目录"<<endl;
            return resultInode;
        }
        else
        {
            inode = temp_inode;
        }
    }
    return inode;
}

//列出当前工作目录下的子目录和文件
string FileSystem::dir_list()
{
    //读取当前inode下到底有多少个文件
    int file_count = cur_inode.get_file_count();
    //读取几个块
    int block_count = file_count/FILE_PER_BLOCK;
    if(file_count%FILE_PER_BLOCK != 0)
    {
        block_count++;
    }
    int min_block= min(block_count,10);
    int addr[10];
    memcpy(addr,cur_inode.get_direct_block_address(),10*4);
    for(int i = 0; i < min_block; i++)
    {
        //读取当前block
        file.seekg(addr[i]*BLOCK_SIZE,ios::beg);
        if(file_count < FILE_PER_BLOCK)
        {
            //读file_count个
            for(int j = 0; j < file_count; j++)
            {
                File file_data;
                file.read((char *)&file_data,sizeof(File));
                cout<<file_data.get_FileName()<<endl;
            }
        }
        else
        {
            //读FILE_PER_BLCOK个
            for(int j = 0; j < FILE_PER_BLOCK; j++)
            {
                File file_data;
                file.read((char *)&file_data,sizeof(File));
                cout<<file_data.get_FileName()<<endl;
            }
            file_count -= FILE_PER_BLOCK;
        }
    }
    if(block_count > 10)
    {
        //继续读取间接地址
        int indirect = cur_inode.get_indirect_block_address();
        if(indirect == -1)
        {
            return "error";
        }
        //还有几个地址要读
        int remain_block = block_count - 10;
        int remain_addr[remain_block];
        //找到存地址的block
        file.seekg(indirect*BLOCK_SIZE,ios::beg);
        for(int i = 0; i < remain_block; i++)
        {
            file.read((char*)&remain_addr[i],sizeof(int));
        }
        //读对应地址里的block
        for(int i = 0; i < remain_block; i++)
        {
            file.seekg(remain_addr[i]*BLOCK_SIZE,ios::beg);
            File file_data;
            file.read((char *)&file_data,sizeof(File));
            cout<<file_data.get_FileName()<<endl;
        }        
    }
    return "This is all of the files and sub-directories.";

}

void FileSystem::sum()
{
    cout<<"Usage of blocks:"<<setw(5)<<superBlock.used_block_count<<"/"<<superBlock.block_count<<endl;
    cout<<"Usage of inodes:"<<setw(5)<<superBlock.used_inode_count<<"/"<<superBlock.inode_count<<endl;
}