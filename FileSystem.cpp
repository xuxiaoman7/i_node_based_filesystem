#include "FileSystem.h"
#include "Inode.h"
#include <vector>
#include <string>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <cstddef>
#include <cmath>
#include <iomanip>
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
    //初始化系统，尝试打开文件
    file.open("filesystem.txt", ios::in | ios::out);
    //如果文件不存在的话
    if (!file.is_open())
    {
        ofstream fout("filesystem.txt"); //创建文件，并且把文件扩充为16MB
        if (!fout)                       //如果创建失败
        {
            cout << "Fail to initialize file system..." << endl;
            exit(-1);
        }
        cout << "Initialize the file system successfully!" << endl;
        for (int i = 0; i < 16384; i++) //把文件扩充成16MB
        {
            fout.write(buffer, sizeof(buffer)); //一个buffer1024字节，文件系统一共16MB，所以写16384次
        }
        //初始化超级块并把超级块写进去
        fout.seekp(0, ios::beg);
        fout.write((char *)&superBlock, sizeof(superBlock));
        //把inode位图写进去(全部初始化为0)
        fout.seekp(BLOCK_SIZE, ios::beg);
        for (int i = 0; i < INODE_BITMAP_SIZE; i++)
        {
            char byte;
            memset(&byte, 0, 1);
            fout.write(&byte, sizeof(char));
        }
        //把block位图写进去(全部初始化为0)
        fout.seekp(BLOCK_SIZE + INODE_BITMAP_SIZE, ios::beg);
        for (int i = 0; i < 1024; i++)
        {
            char byte;
            memset(&byte, 0, 1);
            fout.write(&byte, sizeof(char));
        }
        //关掉ofstream，用全局变量fstream来打开文件
        fout.close();
        file.open("filesystem.txt", ios::in | ios::out);
        //把block位图中的前4块分别置为1(superblock, inode bitmap, block bitmap)
        //把block位图中的inode table占据的块也设置为1(1024块)
        for (int i = 0; i < INODE_TABLE_SIZE / BLOCK_SIZE + 4; i++)
        {
            modify_block_bitmap(i);
        }
        superBlock.used_block_count += (4 + 1024); //更新superblock
        //设置根目录占据的inode节点(只需要设置id，其他的属性初始化的时候会自动赋值)
        root_inode.set_id(0);
        modify_inode_bitmap(0);
        superBlock.used_inode_count++; //更新superblock
        //将inode保存
        file.seekg(BLOCK_SIZE + INODE_BITMAP_SIZE + BLOCK_BITMAP_SIZE, ios::beg);
        file.write((char *)&root_inode, sizeof(root_inode));
        //设置当前工作目录
        cur_inode = root_inode;
    }
    //如果文件已存在
    else
    {
        cout << "FileSystem is loading..." << endl;
        //把superBlock读出来
        file.seekg(0, ios::beg);
        file.read((char *)&superBlock, sizeof(superBlock));
        //读出文件中的根目录inode
        file.seekp(BLOCK_SIZE + INODE_BITMAP_SIZE + BLOCK_BITMAP_SIZE, ios::beg);
        file.read((char *)&root_inode, sizeof(root_inode));
        cur_inode = root_inode;
    }
    cur_path = "/";
    //现在文件已经存在了，开始接收用户输入的指令
    handle_command();
}

void FileSystem::welcome()
{
    cout << "Welcome to Our File System!" << endl;
    cout << "Name            StudentID" << endl;
    cout << "许晓漫          201930344213" << endl;
    cout << "姚禧            201930330247" << endl;
    cout << "For more information,please enter help" << endl;
    string s;
    cin >> s;
    if (s == "help")
    {
        help();
    }
}

void FileSystem::help()
{
    cout << "--createFile fileName fileSize    create a file with specified filesize(in KB) which is smaller than 266KB" << endl;
    cout << "--deleteFile fileName             delete a file" << endl;
    cout << "--createDir dirName               create a directory" << endl;
    cout << "--deleteDir dirName               delete a directory(except the current working directory)" << endl;
    cout << "--changeDir path                  change the working directory" << endl;
    cout << "--dir                             list all the files and sub-directories under current working directory" << endl;
    cout << "--cp file1 file2                  copy file1 to file2" << endl;
    cout << "--sum                             display the usage of storage space the 16MB" << endl;
    cout << "--cat fileName                    print out the file contents" << endl;
    cout << "--exit                            exit the system" << endl;
    cout << "DON'T USE CTRL+C TO EXIT!!!" << endl;
}

void FileSystem::handle_command()
{
    //输出当前工作目录
    cout << cur_path << "$ ";
    string command;
    getline(cin, command);
    getline(cin, command);
    while (1)
    {
        cur_inode = read_inode(cur_inode.get_id()); //更新cur_inode，防止在上一次操作中改变了
        root_inode = read_inode(0);                 //更新root_inode，防止在上一次操作中改变了
        vector<string> commands;                    //用来存以空格隔开的命令
        string::size_type i = command.find(" ", 0); //查找第一个空格所在的地方
        while (i != string::npos)
        {
            commands.push_back(command.substr(0, i));
            command = command.substr(i + 1, command.length() - (i + 1));
            i = command.find(" ", 0);
        }
        commands.push_back(command);
        //分析命令（检查命令的格式是否正确并调用相应函数
        if (commands[0] == "createFile")
        {
            if (commands.size() != 3)
            {
                cout << "The number of parameters should be 3!" << endl;
            }
            else
            {
                int size;
                if (!string_to_int(commands[2], size))
                {
                    cout << "Fail to create! File size should be a positive number!" << endl;
                }
                else
                {
                    if (size > 266)
                    {
                        cout << "Fail to create!Please use a smaller size!" << endl;
                    }
                    else
                    {
                        //判断文件名长度是否比12字节长
                        if (commands[1].size() > 12)
                            cout << "The length of file name is too long" << endl;
                        else
                            cout << createFile(commands[1], size) << endl;
                    }
                }
            }
        }
        else if (commands[0] == "deleteFile")
        {
            //判断参数数目
            if (commands.size() != 2)
            {
                cout << "Lack of parameter! Please try again." << endl;
                continue;
            }
            //判断当前路径是绝对路径还是相对路径,可删除不是当前目录下的文件
            // 如果是绝对路径，可从根节点下搜集
            string filepath = commands[1];
            Inode search_inode;
            if (filepath[0] == '/')
            {
                search_inode = root_inode;
            }
            else
            {
                search_inode = cur_inode;
            }

            //分割字符串
            vector<string> spl_path = split_path(filepath);
            deleteFile(spl_path, search_inode);
        }
        else if (commands[0] == "createDir")
        {
            if (commands.size() != 2)
            {
                cout << "The number of parameters should be 3!" << endl;
            }
            else
            {
                if (commands[1].size() > 12)
                    cout << "The length of the directory is too long" << endl;
                else
                    cout << createDir(commands[1]) << endl;
            }
        }
        else if (commands[0] == "deleteDir")
        {
            //判断参数数目
            if (commands.size() != 2)
            {
                cout << "Lack of parameter! Please try again." << endl;
                continue;
            }
            //判断当前路径是绝对路径还是相对路径,可删除不是当前目录下的文件
            string filepath = commands[1];
            if (filepath == cur_path)
            {
                cout << "You are not allowed to delete the current working directory! Please try other options.\n";
                continue;
            }
            Inode search_inode;
            // 如果是绝对路径，可从根节点下搜集
            if (filepath[0] == '/')
            {
                search_inode = root_inode;
            }
            else
            {
                search_inode = cur_inode;
            }

            //分割字符串
            vector<string> spl_path = split_path(filepath);
            cout << deleteDir(spl_path, search_inode) << endl;
        }
        else if (commands[0] == "changeDir")
        {
            //判断参数数目
            if (commands.size() != 2)
            {
                cout << "Lack of parameter! Please try again." << endl;
                continue;
            }
            changeDir(commands[1]);
        }
        else if (commands[0] == "dir")
        {
            if (commands.size() != 1)
            {
                cout << "The number of parameters should be 1!" << endl;
            }
            else
            {
                cout << dir_list() << endl;
            }
        }
        else if (commands[0] == "cp")
        {
            if (commands.size() != 3)
            {
                cout << "Something was wrong for the format of the commands. There should be 3 parameters." << endl;
                continue;
            }
            copy_path(commands[1], commands[2]);
        }
        else if (commands[0] == "sum")
        {
            if (commands.size() != 1)
            {
                cout << "The number of parameters should be 1!" << endl;
            }
            else
            {
                sum();
            }
        }
        else if (commands[0] == "cat")
        {
            if (commands.size() != 2)
            {
                cout << "The number of parameters should be 2!" << endl;
            }
            else
            {
                cout << catFile(commands[1]) << endl;
            }
        }
        else if (commands[0] == "help")
        {
            help();
        }
        else if (commands[0] == "exit")
        {
            //把super block写回去
            file.seekp(0, ios::beg);
            file.write((char *)&superBlock, sizeof(superBlock));
            file.close();
            exit(0);
        }
        else
        {
            cout << "Please input the commands in correct format!" << endl;
        }
        cout << cur_path << "$ ";
        getline(cin, command); //获取下一个指令
    }
}

//修改block bitmap中的某一个比特位(参数：哪一位)
void FileSystem::modify_block_bitmap(int position)
{
    int byte_position = position / 8; //表示当前这个要修改的位在哪个字节上
    int bit_position = position % 8;  //表示当前这个要修改的位在字节上的哪个位置
    file.seekg(2 * 1024 + byte_position, ios::beg);
    char a;
    file.read(&a, sizeof(a)); //读出那个需要修改的字节
    a = (a ^ (1 << bit_position));
    file.seekp(2 * 1024 + byte_position, ios::beg);
    file.write(&a, sizeof(a));
}

//修改inode
// bitmap中的某一个比特位(参数：哪一位)
void FileSystem::modify_inode_bitmap(int position)
{
    int byte_position = position / 8;
    int bit_position = position % 8;
    file.seekg(1024 + byte_position, ios::beg);
    char a;
    file.read(&a, sizeof(a));
    a = (a ^ (1 << bit_position));
    file.seekp(1024 + byte_position, ios::beg);
    file.write(&a, sizeof(a));
}

//根据blockmap找空闲的block,并返回data block id
int FileSystem::find_free_block()
{
    file.seekg(2 * BLOCK_SIZE, ios::beg);
    char byte;
    //遍历整个位图
    for (int i = 0; i < BLOCK_BITMAP_SIZE; i++)
    {
        file.read(&byte, sizeof(char));
        //判断
        for (int j = 0; j < 8; j++)
        {
            //如果当前位是0，则返回1，否则返回0
            if ((~(byte >> j)) & 1)
            {
                return i * 8 + j; //返回id
            }
        }
    }
    //说明找不到,但一般不会，因为在此之前需要先通过superBlock判断有没有inode可用
    return -1;
}

//根据bitmap找空闲的inode,并返回inode id
int FileSystem::find_free_inode()
{
    file.seekg(1 * BLOCK_SIZE, ios::beg);
    char byte;
    //遍历整个位图
    for (int i = 0; i < INODE_BITMAP_SIZE; i++)
    {
        file.read(&byte, sizeof(char));
        //判断
        for (int j = 0; j < 8; j++)
        {
            //如果当前位是0，则返回1，否则返回0
            if ((~(byte >> j)) & 1)
            {
                return i * 8 + j; //返回id
            }
        }
    }
    //说明找不到,但一般不会，因为在此之前需要先通过superBlock判断有没有inode可用
    return -1;
}

//读取inode节点的相关信息
Inode FileSystem::read_inode(int pos)
{
    Inode res;
    file.seekg(BLOCK_SIZE + INODE_BITMAP_SIZE + BLOCK_BITMAP_SIZE + pos * INODE_SIZE, ios::beg);
    file.read((char *)&res, sizeof(Inode));
    return res;
}

//将inode写回文件
void FileSystem::save_inode(Inode inode)
{
    int id = inode.get_id();
    file.seekp(BLOCK_SIZE + INODE_BITMAP_SIZE + BLOCK_BITMAP_SIZE + id * INODE_SIZE, ios::beg);
    file.write((char *)&inode, sizeof(inode));
}

//将string转换为int并判断是不是纯数字
bool FileSystem::string_to_int(string s, int &size)
{
    size = 0;
    int n = s.length();
    bool isD = true;
    for (int i = 0; i < n; i++)
    {
        if (!isdigit(s[i]))
        {
            isD = false;
            break;
        }
        else
        {
            int temp = (int)(s[i] - '0');
            size += temp * (int)pow(10, n - i - 1);
        }
    }
    return isD;
}

//路径分割
vector<string> FileSystem::split_path(string filepath)
{
    vector<string> res_s;
    //路径拆分
    char *fip = (char *)filepath.c_str();
    char *p = strtok(fip, "/");
    string tmp;
    while (p != NULL)
    {
        tmp = p;
        res_s.push_back(tmp);
        p = strtok(NULL, "/");
    }
    return res_s;
}

//判断并确定当前工作目录对应的inode节点，要求的输入路径
Inode FileSystem::find_inode_from_path(vector<string> res_s, Inode tmp_inode)
{
    Inode res_inode;
    bool if_not_exist = false;
    for (int i = 0; i < res_s.size(); ++i)
    {
        int d = -1, offset = -1;
        vector<File> dirFile = search_file_by_direct_address(tmp_inode, res_s[i], d, offset);
        if (dirFile.size() == 0)
        {

            vector<File> indirFile = search_file_by_indirect_address(tmp_inode, res_s[i], d, offset);

            if (indirFile.size() > 0)
            {
                File tmp = indirFile[0];
                tmp_inode = read_inode(tmp.get_InodeID());
            }
        }
        else
        {
            int p = dirFile[0].get_InodeID();
            tmp_inode = read_inode(p);
        }

        if (d < 0 || offset < 0)
        {
            if_not_exist = true;
            break;
        }
    }
    if (!if_not_exist)
        res_inode = tmp_inode;
    return res_inode;
}

//通过inode在direct address下找目标File类，如果filenanme为""，则寻找该inode下的所有类，否则返回与filename名字相同的File类
vector<File> FileSystem::search_file_by_direct_address(Inode inode, string filename, int &block_addr, int &offset)
{
    block_addr = -1, offset = -1;
    vector<File> res_file;
    bool tar = true;
    if (filename == "")
    {
        tar = true;
    }
    else
        tar = false;

    int addr;
    int size = 0; //记录当前在一个data block中已经遍历的File数目
    bool haveFile = false;
    for (int j = 0; j < 10; ++j)
    {
        if (inode.get_direct_block_address()[j] == -1)
            continue;
        addr = inode.get_direct_block_address()[j];
        // inode对应的Dir类读取
        while (size < 1024)
        {
            File test_f;
            file.seekg(addr * 1024 + size, ios::beg);
            file.read((char *)&test_f, sizeof(File));
            if (tar)
            {
                if (test_f.get_InodeID() >= 0)
                    res_file.push_back(test_f);
            }
            else
            {
                //找到对应的文件信息
                if (test_f.get_FileName() == filename)
                {
                    haveFile = true;
                    block_addr = addr * 1024;
                    offset = size;
                    res_file.push_back(test_f);
                    break;
                }
            }
            size += sizeof(File);
        }
        if (haveFile)
            break;
        size = 0;
    }
    return res_file;
}

//通过inode在indirect address下找目标File类，如果filenanme为""，则寻找该inode下的所有类，否则返回与filename名字相同的File类
vector<File> FileSystem::search_file_by_indirect_address(Inode inode, string filename, int &block_addr, int &offset)
{
    block_addr = -1, offset = -1;
    vector<File> res_file;
    bool tar = true, haveFile = false;
    int size = 0, dir_tmp;
    if (filename == "")
    {
        tar = true;
    }
    else
        tar = false;

    dir_tmp = inode.get_indirect_block_address();
    if (dir_tmp != -1)
    {

        for (int d = 0; d < 256; ++d)
        {
            file.seekg(dir_tmp * 1024 + d * 4, ios::beg);
            file.read((char *)&dir_tmp, sizeof(int));
            if (dir_tmp != -1)
            {
                while (size < 1024)
                {
                    File test_f;
                    file.seekg(dir_tmp * 1024 + size, ios::beg);
                    file.read((char *)&test_f, sizeof(File));
                    // test_f = read_fileinfo(dir_tmp * 1024 + size);
                    if (tar)
                    {
                        if (test_f.get_InodeID() >= 0)
                            res_file.push_back(test_f);
                    }
                    else
                    {
                        if (test_f.get_FileName() == filename)
                        {
                            haveFile = true;
                            block_addr = dir_tmp * 1024;
                            offset = size;
                            res_file.push_back(test_f);
                            break;
                        }
                    }
                    size += sizeof(File);
                }
            }
            //跳出256个循环
            if (haveFile)
                break;
            size = 0;
        }
    }

    return res_file;
}

//往新建的文件里写入xx字节的数据
void FileSystem::write_random_string_to_file(int block_id)
{
    file.seekp(block_id * BLOCK_SIZE, ios::beg);
    file.write((char *)&buffer, sizeof(buffer));
}

//往目录中加入文件名和文件id
void FileSystem::add_file_to_dir(File &file_data, Inode &inode)
{
    int byte = inode.get_byte_size();
    byte += sizeof(File);
    inode.set_byte_size(byte);

    int addr[10];
    memset(addr, -1, sizeof(addr));
    memcpy(addr, inode.get_direct_block_address(), 10 * 4);

    int file_count = inode.get_file_count();

    //因为删除File类后，原来存储的位置变为空，需要搜索一下空置的部分，以便后面重新利用该部分
    File fe;
    bool find = false;
    for (int a = 0; a < 10; ++a)
    {
        if (addr[a] == -1)
        {
            //找到了一个新块，要给这个新块地址
            int add = find_free_block();
            modify_block_bitmap(add);
            superBlock.used_block_count++;
            addr[a] = add;
            inode.set_direct_block_address(addr);
        }
        file.seekg(addr[a] * BLOCK_SIZE + 0 * FILE_SIZE, ios::beg);
        for (int b = 0; b < FILE_PER_BLOCK; ++b)
        {
            file.read((char *)&fe, sizeof(File));
            //找到空闲的File类
            if (fe.get_InodeID() <= 0)
            {
                file.seekp(addr[a] * BLOCK_SIZE + b * FILE_SIZE, ios::beg);
                file.write((char *)&file_data, sizeof(File));
                find = true;
                break;
            }
        }
        if (find)
            break;
    }
    //如果没找到，说明要去间接地址里放
    if (!find)
    {
        int indirect = inode.get_indirect_block_address();
        if (indirect == -1)
        {
            int add = find_free_block();
            modify_block_bitmap(add);
            superBlock.used_block_count++;
            indirect = add;
            inode.set_indirect_block_address(add);
            //加入地址块中的第一个地址
            file.seekp(indirect * BLOCK_SIZE, ios::beg);
            int first_add = find_free_block();
            modify_block_bitmap(first_add);
            superBlock.used_block_count++;
            file.write((char *)&first_add, sizeof(int));
            //往第一个地址指向的块中添加File_data
            file.seekp(first_add * BLOCK_SIZE, ios::beg);
            file.write((char *)&file_data, sizeof(FILE));
        }
        else
        {
            int in_addr[256];
            file.seekg(indirect * BLOCK_SIZE, ios::beg);
            for (int i = 0; i < 256; i++)
            {
                file.read((char *)&in_addr[i], sizeof(int));
            }
            bool isFound = false;
            for (int i = 0; i < 256; i++)
            {
                file.seekg(in_addr[i] * BLOCK_SIZE, ios::beg);
                for (int j = 0; j < FILE_PER_BLOCK; j++)
                {
                    File fdata;
                    file.read((char *)&fdata, sizeof(File));
                    if (fdata.get_InodeID() <= 0)
                    {
                        //找到了，加进去
                        file.seekp(in_addr[i] * BLOCK_SIZE + j * FILE_SIZE, ios::beg);
                        file.write((char *)&file_data, sizeof(FILE));
                        isFound = true;
                        break;
                    }
                }
                if (isFound)
                    break;
            }
        }
    }

    file_count++;
    inode.set_file_count(file_count);
    inode.set_modified_time(time(0));
    //把inode写回去
    save_inode(inode);
}

//返回成功创建的信息或错误信息
std::string FileSystem::createFile(string filepath, int size)
{
    Inode temp_cur_inode;
    if (filepath[0] == '/')
    {
        temp_cur_inode = root_inode;
        filepath = filepath.substr(1, filepath.length() - 1);
    }
    else
    {
        temp_cur_inode = cur_inode;
    }

    vector<string> dirs = split_path(filepath); //分割路径
    string filename = dirs[dirs.size() - 1];
    if (filename == "") //判断是否输入文件名
        return "Please input the filename!";
    //查找路径是否存在
    dirs.erase(dirs.end() - 1);
    Inode temp = find_inode_from_path(dirs, temp_cur_inode);
    if (temp.get_id() == -1) //路径不存在
        return "Path is not exist!";
    else if (temp.get_file_type() == FILE_TYPE) //路径中含有文件名
        return "There is a file with same name!";
    else //路径存在
        temp_cur_inode = temp;

    //判断文件名存不存在
    vector<string> n = {filename};
    Inode temp2 = find_inode_from_path(n, temp_cur_inode);
    if (temp2.get_id() != -1)
        return "File has been exist!";

    //判断inode还有没有得用
    if (superBlock.inode_count == superBlock.used_inode_count)
    {
        return "You have created too much file! Please delete some files and try again.";
    }
    //判断目录下还有没有空间
    if (temp_cur_inode.get_file_count() >= MAX_FILE_COUNT_PER_DIRECTORY)
    {
        return "You have created too much file in current path! Please delete some and retry.";
    }
    //判断data block里剩余空间还够不够
    if (superBlock.block_count == superBlock.used_block_count)
    {
        return "You have created too much file! Please delete some files and try again.";
    }

    //创建File对象写进目录里
    File file_data;
    char filen[MAX_FILENAME_LENGTH];
    strcpy(filen, filename.c_str());
    file_data.set_FileName(filen);
    //根据bitmap寻找空闲的inode
    int id = find_free_inode();
    file_data.set_InodeID(id);
    modify_inode_bitmap(id); //修改bitmap位图
    //把文件名和inode写进当前目录(目录也会被更新后写进文件系统)
    add_file_to_dir(file_data, temp_cur_inode);

    //根据bitmap中block的使用情况来分配block
    int addr[10];
    memset(addr, -1, sizeof(addr)); //直接地址10个全部初始化为-1
    int indirect_addr = -1;
    vector<int> indirect;
    for (int i = 0; i < size; i++)
    {
        if (i < 10)
        {
            addr[i] = find_free_block();
            modify_block_bitmap(addr[i]); //修改block bitmap
        }
        else
        {
            int temp_block_id = find_free_block();
            indirect.push_back(temp_block_id);
            modify_block_bitmap(temp_block_id);
        }
    }
    for (int i = 0; i < min(10, size); i++)
    {
        write_random_string_to_file(addr[i]); //写入随机数据
    }
    if (!indirect.empty()) //如果间接地址块有被用到
    {
        int temp_block_id = find_free_block(); //寻找一块空闲block存放间接地址指向的256个地址
        indirect_addr = temp_block_id;
        modify_block_bitmap(temp_block_id);
        //把vector里的地址数据添进indirect_addr所指向的data block
        file.seekp(indirect_addr * BLOCK_SIZE, ios::beg);
        for (int i = 0; i < indirect.size(); i++)
        {
            file.write((char *)&indirect[i], sizeof(int));
        }
        //把地址对应的data block写上随机数据
        for (int i = 0; i < indirect.size(); i++)
        {
            write_random_string_to_file(indirect[i]);
        }
    }

    //创建Inode对象并写进inode table里
    Inode finode;
    finode.set_id(id);
    finode.set_byte_size(size * 1024); //把KB转化成B
    finode.set_created_time(time(0));
    finode.set_modified_time(time(0));
    finode.set_file_type(FILE_TYPE);
    finode.set_direct_block_address(addr);
    finode.set_indirect_block_address(indirect_addr);
    //把文件对应的inode写进inode table里
    file.seekp(BLOCK_SIZE + INODE_BITMAP_SIZE + BLOCK_BITMAP_SIZE + id * INODE_SIZE, ios::beg);
    file.write((char *)&finode, sizeof(Inode));

    // superblock更新
    superBlock.used_block_count += size;
    superBlock.used_inode_count += 1;
    return "Create successfully!";
}

//创建新目录
string FileSystem::createDir(string dirPath)
{
    Inode temp_cur_inode;
    //查找路径存不存在
    if (dirPath[0] == '/')
    {
        temp_cur_inode = root_inode;
        dirPath = dirPath.substr(1, dirPath.length() - 1);
    }
    else
    {
        temp_cur_inode = cur_inode;
    }

    //分割路径和目录名
    vector<string> dirs = split_path(dirPath);
    string dirname = dirs[dirs.size() - 1];
    //没有输入目录名
    if (dirname == "")
    {
        return "Please input the directory name!";
    }
    // cout<<"dirs"<<endl;
    // for(int i = 0; i < dirs.size(); i++)
    // {
    //     cout<<dirs[i]<<" ";
    // }
    // cout<<endl;
    //开始查找路径是否存在
    dirs.erase(dirs.end() - 1);
    Inode temp = find_inode_from_path(dirs, temp_cur_inode);
    if (temp.get_id() == -1)
        return "Path is not exist!";
    else
        temp_cur_inode = temp;

    //判断要创建的目录名存不存在
    vector<string> n = {dirname};
    Inode temp2 = find_inode_from_path(n, temp_cur_inode);
    if (temp2.get_id() != -1)
        if (temp2.get_file_type() == FILE_TYPE)
            return "There has been a file with the same name!";
        else
            return "Directory has been exist!";

    //判断inode还有没有得用
    if (superBlock.inode_count == superBlock.used_inode_count)
    {
        return "You have created too many files! Please delete some files and try again.";
    }
    //判断目录下还有没有空间
    if (temp_cur_inode.get_file_count() >= MAX_FILE_COUNT_PER_DIRECTORY)
    {
        return "You have created too many files in current path! Please delete some and retry.";
    }
    //判断data block里剩余空间还够不够
    if (superBlock.block_count == superBlock.used_block_count)
    {
        return "You have created too many files! Please delete some files and try again.";
    }

    //开始创建目录
    File file_data;
    char dirn[MAX_FILENAME_LENGTH];
    strcpy(dirn, dirname.c_str());
    file_data.set_FileName(dirn);
    //根据bitmap寻找没用到的inode
    int id = find_free_inode();
    file_data.set_InodeID(id);
    modify_inode_bitmap(id);
    //把目录名和inode写进当前目录(目录也会被更新后写进文件系统)
    add_file_to_dir(file_data, temp_cur_inode);

    // superblock更新
    superBlock.used_inode_count += 1;

    //创建dir node对应的inode
    Inode dinode;
    dinode.set_file_type(DIRECTORY_TYPE);
    dinode.set_id(id);
    //把inode写进去
    file.seekp(BLOCK_SIZE + INODE_BITMAP_SIZE + BLOCK_BITMAP_SIZE + id * INODE_SIZE, ios::beg);
    file.write((char *)&dinode, sizeof(Inode));

    return "Create successfully!";
}

//删除关于inode下所有块的相关信息
void FileSystem::delete_blockinfo_for_inode(Inode tar_node)
{
    //地址搜寻
    int dir_addr;
    int indir_addr;
    for (int i = 0; i < 10; ++i)
    {
        //可以判断一下是否有存地址信息
        dir_addr = tar_node.get_direct_block_address()[i];
        if (dir_addr != -1)
        {
            //移动到对应的datablock位置，进行清空
            file.seekp(dir_addr * BLOCK_SIZE, ios::beg);
            file.write(buffer, sizeof(buffer));
            // block_bitmap置0
            modify_block_bitmap(dir_addr);
        }
    }
    //清空存储间接地址的datablock及其相关内容
    indir_addr = tar_node.get_indirect_block_address();
    //先清空内容
    if (indir_addr != -1 && indir_addr != 0)
    {
        file.seekg(indir_addr * BLOCK_SIZE, ios::beg);
        vector<int> inres;
        for (int i = 0; i < 256; ++i)
        {
            int tmp;
            file.read((char *)&tmp, sizeof(int));
            if (tmp > 0)
                inres.push_back(tmp);
        }
        for (int j = 0; j < inres.size(); ++j)
        {
            file.seekp(inres[j] * BLOCK_SIZE, ios::beg);
            file.write(buffer, sizeof(buffer));
            modify_block_bitmap(inres[j]);
        }
        //再清空地址
        file.seekp(indir_addr * BLOCK_SIZE, ios::beg);
        modify_block_bitmap(indir_addr);
        file.write(buffer, sizeof(buffer));
    }
    int id = tar_node.get_id();
    file.seekp(1024 + id, ios::beg);
    //清空inode table
    Inode ino;
    file.write((char *)&ino, sizeof(Inode));
    modify_inode_bitmap(id);
}

void FileSystem::check_and_modify_inode(bool is_direct, int file_location, Inode &inode)
{
    file.seekg(file_location, ios::beg);
    bool ifempty = true;
    for (int i = 0; i < 1024; i += sizeof(File))
    {
        //检查读出的文件是否存在
        File check;
        file.read((char *)&check, sizeof(File));
        if (check.get_InodeID() > 0)
        {
            ifempty = false;
            break;
        }
    }
    //此块为空
    if (ifempty)
    {
        //修改此块对应的blcok_bitmap
        modify_block_bitmap(file_location / 1024);
        superBlock.used_block_count -= 1;
        //修改final_inode1对应的地址为-1
        //如果是直接地址的话
        if (is_direct)
        {
            int find_direct[10] = {0};
            for (int i = 0; i < 10; ++i)
            {

                find_direct[i] = inode.get_direct_block_address()[i];
                if (find_direct[i] == file_location / 1024)
                {
                    find_direct[i] = -1;
                    // break;
                }
            }
            inode.set_direct_block_address(find_direct);
        }
        else
        {
            //间接地址比较麻烦
            //首先找到间接地址位置
            int find_indir = inode.get_indirect_block_address();
            //遍历256个块（要全部遍历）
            file.seekg(find_indir * 1024, ios::beg);
            bool is_exist_block = false;
            for (int j = 0; j < 256; ++j)
            {
                int indir_num = -1, wr_add = -1;
                file.read((char *)&indir_num, sizeof(int));
                //判断是否为要找的地址同时还要判断目前间接地址中是否有其他块

                if (indir_num != -1)
                {
                    if (indir_num == file_location / 1024)
                    {
                        file.seekp(find_indir * 1024 + j * sizeof(int), ios::beg);
                        file.write((char *)&wr_add, sizeof(int));
                    }
                    else
                    {
                        is_exist_block = true;
                    }
                }
            }
            if (!is_exist_block)
            {
                inode.set_indirect_block_address(-1);
            }
        }
    }
}
//删除文件
void FileSystem::deleteFile(vector<string> spl_path, Inode search_inode)
{

    //判断当前路径是否存在，即在当前inode下搜寻是否存在该文件名
    Inode final_inode = find_inode_from_path(spl_path, search_inode); //判断是否存在该文件名
    if (final_inode.get_id() == -1)
    {
        cout << "There is no such file path! Something is wrong for your input." << endl;
        return;
    }
    if (final_inode.get_file_type() == DIRECTORY_TYPE)
    {
        cout << "There is only a directory with same name! Something is wrong for youe input." << endl;
        return;
    }
    int byte = final_inode.get_byte_size();
    //根据inode删除datablock,block bitmap,inode table, inode bitmap的信息
    delete_blockinfo_for_inode(final_inode);

    //删除存储该inode的上一级inode的地址空间信息
    //求上级目录的inode
    string final_filename = spl_path[spl_path.size() - 1];
    spl_path.pop_back();

    Inode final_inode1 = find_inode_from_path(spl_path, search_inode);
    int file_location, block_offset;
    bool is_direct = true;

    vector<File> daddr = search_file_by_direct_address(final_inode1, final_filename, file_location, block_offset);
    vector<File> idaddr;
    if (daddr.size() == 0)
    {
        idaddr = search_file_by_indirect_address(final_inode1, final_filename, file_location, block_offset);
        is_direct = false;
    }
    //通过直接地址找到
    if (file_location >= 0 && block_offset >= 0)
    {
        int file_count = final_inode1.get_file_count();
        file_count -= 1;
        final_inode1.set_file_count(file_count);

        File repl;
        file.seekp(file_location + block_offset, ios::beg);
        file.write((char *)&repl, sizeof(File));
    }

    //将inode写回
    save_inode(final_inode1);

    //上面基本删除完毕后，还要考虑对应data blcok下的文件数目是否为0
    //得到文件存储对应的块file_location
    check_and_modify_inode(is_direct, file_location, final_inode1);

    save_inode(final_inode1);
    // superblock更新
    superBlock.used_block_count -= byte / 1024;
    superBlock.used_inode_count -= 1;
    cout << "Delete the file with name " << final_filename << " successfully!" << endl;
}

//删除目录
std::string FileSystem::deleteDir(vector<string> spl_path, Inode tar_inode)
{

    //分别删除指定目录下的所有内容
    //判断当前路径是否存在，即在当前inode下搜寻是否存在该目录
    Inode final_inode = find_inode_from_path(spl_path, tar_inode); //判断是否存在该目录
    if (final_inode.get_id() == -1)
    {
        return "There is no such directory! Something is wrong for your input.";
    }

    if (final_inode.get_file_type() == FILE_TYPE)
    {
        return "There is a file with the same name! Something is wrong for your input.";
    }
    int byte = final_inode.get_byte_size();

    //对该目录下的所有信息进行判断，有多少Directory和File
    //找到所有文件信息
    int file_addr, offset;
    vector<File> dirFile = search_file_by_direct_address(final_inode, "", file_addr, offset);
    vector<File> indirFile = search_file_by_indirect_address(final_inode, "", file_addr, offset);
    //对直接地址进行解析
    for (int i = 0; i < dirFile.size(); ++i)
    {
        File tmp = dirFile[i];
        Inode tmp_inode = read_inode(tmp.get_InodeID());
        vector<string> res_name;
        string tmp_filename = tmp.get_FileName();
        res_name.push_back(tmp_filename);
        if (tmp_inode.get_file_type() == FILE_TYPE)
        {

            deleteFile(res_name, final_inode);
        }

        if (tmp_inode.get_file_type() == DIRECTORY_TYPE)
        {

            deleteDir(res_name, final_inode);
        }
        res_name.pop_back();
    }
    //对间接地址进行解析并删除
    for (int j = 0; j < indirFile.size(); ++j)
    {
        File tmp = dirFile[j];
        Inode tmp_inode = read_inode(tmp.get_InodeID());
        string tmp_filename = tmp.get_FileName();
        vector<string> res_name;
        res_name.push_back(tmp_filename);

        if (tmp_inode.get_file_type() == FILE_TYPE)
        {
            deleteFile(res_name, final_inode);
        }
        if (tmp_inode.get_file_type() == DIRECTORY_TYPE)
        {

            deleteDir(res_name, final_inode);
        }
        res_name.pop_back();
    }
    //删除存储该inode的上一级inode的地址空间信息
    //求上级目录的inode
    string final_filename = spl_path[spl_path.size() - 1];
    spl_path.pop_back();
    Inode final_inode1 = find_inode_from_path(spl_path, tar_inode);
    bool is_direct = true;
    int file_location_, offset_;
    int file_count = final_inode1.get_file_count();
    file_count -= 1;
    final_inode1.set_file_count(file_count);
    vector<File> daddr = search_file_by_direct_address(final_inode1, final_filename, file_location_, offset_);
    vector<File> idaddr;
    if (daddr.size() == 0)
    {
        idaddr = search_file_by_indirect_address(final_inode1, final_filename, file_location_, offset_);
        is_direct = false;
    }
    if (file_location_ >= 0 && offset_ >= 0)
    {
        File repl;
        file.seekp(file_location_ + offset_, ios::beg);
        file.write((char *)&repl, sizeof(File));
    }
    save_inode(final_inode1);
    check_and_modify_inode(is_direct, file_location_, final_inode1);
    save_inode(final_inode1);

    superBlock.used_block_count -= byte / 1024;
    superBlock.used_inode_count -= 1;
    return "Delete the directory successfully!";
}

//输出文件内容（cat）
string FileSystem::catFile(string filePath)
{
    Inode temp_cur_inode;
    if (filePath[0] == '/')
    {
        temp_cur_inode = root_inode;
        filePath = filePath.substr(1, filePath.length() - 1);
    }
    else
    {
        temp_cur_inode = cur_inode;
    }
    //分割路径和文件名
    vector<string> dirs = split_path(filePath);
    string filename = dirs[dirs.size() - 1];
    if (filename == "")
        return "Please input the filename!";
    //判断路径是否存在
    Inode temp_inode = find_inode_from_path(dirs, temp_cur_inode);
    if (temp_inode.get_id() == -1)
        return "File is not exist!";
    else if (temp_inode.get_file_type() == DIRECTORY_TYPE)
        return "It is not a file!";
    else
        temp_cur_inode = temp_inode; //现在的temp_cur_inode就是文件的inode

    int addr[10]; //读取直接地址
    memcpy(addr, temp_cur_inode.get_direct_block_address(), 10 * 4);
    int block_count = temp_cur_inode.get_byte_size() / BLOCK_SIZE; //计算有几个data block
    for (int i = 0; i < min(10, block_count); i++)
    {
        file.seekg(addr[i] * BLOCK_SIZE, ios::beg); //指向存放文件内容的data block
        file.read((char *)buffer, BLOCK_SIZE);      //读取到1024字节的字符数组buffer
        for (int i = 0; i < 1024; i++)              //逐字节输出文件内容
            cout << (int)buffer[i];
    }
    if (block_count > 10)
    {
        //读取间接地址
        int indirect_addr = temp_cur_inode.get_indirect_block_address();
        if (indirect_addr == -1)
            return "error";
        int remain_block = block_count - 10; //还有几个地址要读
        int remain_addr[remain_block];
        file.seekg(indirect_addr * BLOCK_SIZE, ios::beg); //找到存间接地址的block
        for (int i = 0; i < remain_block; i++)
            file.read((char *)&remain_addr[i], sizeof(int));
        for (int i = 0; i < remain_block; i++) //读对应地址里的block
        {
            file.seekg(remain_addr[i] * BLOCK_SIZE, ios::beg);
            file.read((char *)buffer, sizeof(buffer));
            for (int i = 0; i < 1024; i++)
                cout << (int)buffer[i];
        }
    }
    cout << endl;
    return "That is all of the file";
}

//列出当前文件系统的使用情况
void FileSystem::sum()
{
    cout << "Usage of blocks:" << setw(5) << superBlock.used_block_count << "/" << superBlock.block_count << endl;
    cout << "Usage of inodes:" << setw(5) << superBlock.used_inode_count << "/" << superBlock.inode_count << endl;
}

//列出当前工作目录下的子目录和文件
string FileSystem::dir_list()
{
    int addr[10]; //读取直接地址
    memcpy(addr, cur_inode.get_direct_block_address(), 10 * 4);
    cout << setw(12) << "Name";
    cout << setw(10) << "Size(KB)";
    cout << setw(10) << "Type" << endl;
    //读取当前inode直接地址指向的10个块
    for (int i = 0; i < 10; i++)
    {
        if (addr[i] == -1)
            continue;
        for (int j = 0; j < FILE_PER_BLOCK; j++)
        {
            file.seekg(addr[i] * BLOCK_SIZE + j * FILE_SIZE, ios::beg);
            File file_data;
            file.read((char *)&file_data, sizeof(File)); //读取“File”对象
            if (file_data.get_InodeID() <= 0)
                continue;
            Inode p1 = read_inode(file_data.get_InodeID()); //读取“Inode”对象
            int type = p1.get_file_type();
            cout << setw(12) << file_data.get_FileName() << setw(10) << p1.get_byte_size() / 1024; //输出
            if (type == FILE_TYPE)
                cout << setw(10) << "FILE" << endl;
            else
                cout << setw(10) << "DIR" << endl;
        }
    }
    //判断并读取间接地址
    int indirect = cur_inode.get_indirect_block_address(); //读取间接地址
    if (indirect != -1)
    {
        int remain_addr[256];                        //存256个地址
        file.seekg(indirect * BLOCK_SIZE, ios::beg); //指向间接地址指向的存256个地址的块
        for (int i = 0; i < 256; i++)
            file.read((char *)&remain_addr[i], sizeof(int));
        for (int i = 0; i < 256; i++) //读对应地址里的block
        {
            if (remain_addr[i] == -1)
                continue;
            for (int j = 0; j < FILE_PER_BLOCK; j++)
            {
                file.seekg(remain_addr[i] * BLOCK_SIZE + j * FILE_SIZE, ios::beg);
                File file_data;
                file.read((char *)&file_data, sizeof(File)); //读取“File”对象
                if (file_data.get_InodeID() <= 0)
                    continue;
                Inode p1 = read_inode(file_data.get_InodeID()); //读取“Inode”对象
                int type = p1.get_file_type();
                cout << setw(12) << file_data.get_FileName() << setw(10) << p1.get_byte_size() / 1024; //输出
                if (type == FILE_TYPE)
                    cout << setw(10) << "FILE" << endl;
                else
                    cout << setw(10) << "DIR" << endl;
            }
        }
    }
    return "This is all of the files and sub-directories.";
}

//将文件内容从一个路径复制到目标路径
void FileSystem::copy_path(string source_path, string target_path)
{
    char bu;
    //一个文件的最大容量是266KB
    vector<char> content;
    Inode source_search_inode;
    //先寻找source path存储的内容
    //如果是绝对路径，可从根节点下搜集
    if (source_path[0] == '/')
    {
        source_search_inode = root_inode;
    }
    else
    {
        source_search_inode = cur_inode;
    }
    //分割字符串
    vector<string> spl_path = split_path(source_path);
    string source_filename = spl_path[spl_path.size() - 1];
    //获取源路径的inode
    Inode source_inode = find_inode_from_path(spl_path, source_search_inode);
    if (source_inode.get_id() < 0)
    {
        cout << "There is no such source filepath to copy. Check your input again." << endl;
        return;
    }
    if (source_inode.get_file_type() == DIRECTORY_TYPE)
    {
        cout << "There is a directory with the same name. Check your input again." << endl;
        return;
    }

    int byte_size = source_inode.get_byte_size();
    // copy所有内容
    int addr;
    for (int i = 0; i < 10; ++i)
    {
        addr = source_inode.get_direct_block_address()[i];

        if (addr > 0)
        {
            file.seekg(addr * BLOCK_SIZE, ios::beg);
            file.read((char *)&bu, sizeof(char));
            for (int j = 0; j < 1024; ++j)
            {
                file.read((char *)&bu, sizeof(char));
                content.push_back(bu);
            }
        }
    }
    if (byte_size > 10 * BLOCK_SIZE)
    {
        //间接地址
        addr = source_inode.get_indirect_block_address();
        if (addr > 0)
        {
            int iaddr;
            for (int j = 0; j < 256; ++j)
            {
                file.seekg(addr * BLOCK_SIZE + j * 4, ios::beg);

                file.read((char *)&iaddr, sizeof(int));
                if (iaddr > 0)
                {
                    file.seekg(iaddr * 1024, ios::beg);
                    for (int j = 0; j < 1024; ++j)
                    {
                        file.read((char *)&bu, sizeof(char));
                        content.push_back(bu);
                    }
                }
            }
        }
    }
    //再寻找目标路径
    //按路径创建文件
    Inode target_search_inode;
    if (target_path[0] == '/')
    {
        target_search_inode = root_inode;
    }
    else
    {
        target_search_inode = cur_inode;
    }
    vector<string> spl_tar_path = split_path(target_path);
    //获取新建文件的inode信息
    Inode target_inode = find_inode_from_path(spl_tar_path, target_search_inode);
    //将文件内容写入
    //说明存在该文件（无论是）
    if (target_inode.get_id() > 0)
    {
        cout << "There exist a directory or file in the target path. Check your input again.\n";
        return;
    }
    else
        createFile(target_path, byte_size / 1024);

    //再次判断
    target_inode = find_inode_from_path(spl_tar_path, target_search_inode);
    if (target_inode.get_id() > 0)
    {
        //小于10KB
        int i = 0;
        while (i < 10 && i < content.size() / 1024)
        {
            int dir_addr = target_inode.get_direct_block_address()[i];
            file.seekp(dir_addr * BLOCK_SIZE, ios::beg);
            if (dir_addr > 0)
            {
                for (int j = 0; j < 1024; ++j)
                {
                    file.write((char *)&content[i * 1024 + j], sizeof(bu));
                }
            }
            i++;
        }
        //存放在间接地址的情况
        if (content.size() / 1024 >= 10)
        {
            int iaddr = target_inode.get_indirect_block_address();
            for (int j = 0; j < content.size() / 1024 - i; ++j)
            {
                //间接地址获取
                int iaddr1;
                file.seekg(iaddr * BLOCK_SIZE + j * 4, ios::beg);
                file.read((char *)&iaddr1, sizeof(int));
                //前往该间接地址
                file.seekp(iaddr1 * BLOCK_SIZE, ios::beg);
                for (int k = 0; k < 1024; ++k)
                    file.write((char *)&content[(i + j) * 1024 + k], sizeof(char));
            }
        }
    }
    else
    {
        cout << "Something was wrong for creating files.\n";
        return;
    }
    cout << "Successfully copy the file!\n";
}

//更改工作目录
void FileSystem::changeDir(string file_path)
{
    //判断当前路径是绝对路径还是相对路径,可删除不是当前目录下的文件
    Inode search_inode;
    string res_cur_path;
    vector<string> spl_path;
    //判断是否为..
    if (file_path == "..")
    {
        //判断当前目录是否是root inode
        if (cur_path == "/")
        {
            cur_path = "/";
            cur_inode = root_inode;
        }
        else
        {
            spl_path = split_path(cur_path);
            //当前目录的前一个目录
            spl_path.pop_back();
            cur_inode = find_inode_from_path(spl_path, root_inode);
            if (spl_path.size() == 0)
                cur_path = "/";
            else
            {
                for (int i = 0; i < spl_path.size(); ++i)
                {
                    res_cur_path += "/";
                    res_cur_path += spl_path[i];
                }
                cur_path = res_cur_path;
            }
        }
        return;
    }

    // 如果是绝对路径，可从根节点下搜集
    if (file_path[0] == '/')
    {
        search_inode = root_inode;
    }
    else
    {
        search_inode = cur_inode;
    }

    //分割字符串
    spl_path = split_path(file_path);
    //首先判断该路径是否存在
    Inode ch_inode = find_inode_from_path(spl_path, search_inode);
    //检查id
    if (ch_inode.get_id() < 0)
    {
        cout << "There is no such filepath! Check if there is something wrong for your input." << endl;
        return;
    }
    //检查文件类型
    if (ch_inode.get_file_type() == FILE_TYPE)
    {
        cout << "There is a file with the same name. Something is wrong for your input." << endl;
        return;
    }

    //此时更改cur_inode
    cur_inode = ch_inode;
    //更改工作路径
    if (spl_path.size() == 0)
        res_cur_path += "/";
    for (int i = 0; i < spl_path.size(); ++i)
    {
        res_cur_path += "/";
        res_cur_path += spl_path[i];
    }

    if (file_path[0] == '/')
    {
        cur_path = res_cur_path;
    }
    else
    {
        if (cur_path == "/")
            cur_path = res_cur_path;
        else
            cur_path += res_cur_path;
    }
}