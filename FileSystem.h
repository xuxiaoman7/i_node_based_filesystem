#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include<vector>
#include<fstream>
#include"Inode.h"
#include"File.h"
#include<string>
#include<string.h>
#define BLOCK_COUNT 16384
#define INODE_COUNT (1024*8)
#define BLOCK_SIZE 1024
#define INODE_SIZE 128
#define INODE_BITMAP_SIZE 1024
#define BLOCK_BITMAP_SIZE 1024*2  //应该是两个块才对
#define INODE_TABLE_SIZE (1024*1024)
#define FILE_SIZE (MAX_FILENAME_LENGTH+4)    //目录下每个文件占据的大小
#define MAX_FILE_COUNT_PER_DIRECTORY (266*(BLOCK_SIZE/FILE_SIZE)) //单个目录下可以存几个文件名
#define FILE_PER_BLOCK (BLOCK_SIZE/FILE_SIZE)    //一个data block下可以放几个文件File
using namespace std;

struct superblock
{
    //块数
    int block_count = BLOCK_COUNT;
    //用了的块数
    int used_block_count = 0;
    //inode数
    int inode_count = INODE_COUNT;
    //用了的inode数
    int used_inode_count = 0;
};
class FileSystem
{
public:
    FileSystem();
    ~FileSystem();

    //初始化s
    void initial();
    //欢迎语
    void welcome();
    //帮助
    void help();
    //修改block bitmap中的某一个比特位(0变1，1变0)(参数：哪一位)
    void modify_block_bitmap(int);
    //修改inode bitmap中的某一个比特位(0变1，1变0)(参数：哪一位)
    void modify_inode_bitmap(int);
    //处理用户输入的命令
    void handle_command();
    //将string转换为int并判断是不是纯数字
    bool string_to_int(std::string,int&);
    //查找给定inode下的是否有给定目录（文件）存在，如果存在，返回inode，如果不存在，返回id为-1的inode
    //Inode findInode(Inode,std::string);
    //通过inodeid读取inode
    Inode read_inode(int pos);
    //根据bitmap找空闲的inode,并返回inode id
    int find_free_inode();
    //根据blockmap找空闲的block,并返回data block id
    int find_free_block();
    //把inode写进文件系统
    void save_inode(Inode);
    //分割路径和文件名(返回各级目录和文件名/目录名)
    std::vector<std::string> split_path(std::string filepath);
    //往新建的文件里写入xx字节的数据
    void write_random_string_to_file(int);
    //根据当前inode和文件路径寻找指定目录下的文件
    Inode find_inode_from_path(std::vector<std::string> cur_filepath, Inode inode);
    //判断当前block下是否存在File类并对inode进行处理
    void check_and_modify_inode(bool is_direct, int file_location, Inode &inode);
    //返回通过直接地址搜索得到的所有文件
    std::vector<File> search_file_by_direct_address(Inode inode, std::string filename,int &block_addr, int &offset);
    //返回通过间接地址搜索得到的所有文件
    std::vector<File> search_file_by_indirect_address(Inode inode, std::string filename,int &block_addr, int &offset);
    //往目录中加入文件名和文件id
    void add_file_to_dir(File&,Inode&);
    //创建新目录
    std::string createFile(std::string,int);
    //删除指定inode下存储的信息，包括inode bitmap, block bitmap, inode table, block的信息
    void delete_blockinfo_for_inode(Inode tar_inode);
    //删除目标路径下的某一文件
    void deleteFile(std::vector<std::string> spl_path, Inode tar_inode);
    //删除某一个目录
    std::string deleteDir(std::vector<std::string> spl_path, Inode tar_inode);
    //将文件由所在路径复制到目标路径
    void copy_path(std::string source_path, std::string target_path);
    //更改当前工作目录
    void changeDir(std::string file_path);
    //创建文件(文件名和文件大小，返回是否创建成功)
    std::string createDir(std::string);
    //输出文件内容（cat）
    std::string catFile(std::string);
    //列出当前工作目录下的子目录和文件
    std::string dir_list();
    //列出当前文件系统的使用情况
    void sum();
private:
    //超级块
    struct superblock superBlock;
    //文件
    std::fstream file;
    //当前所在工作目录(有涉及到更改工作目录的操作时需要修改，初始化为根目录/)
    Inode cur_inode;
    std::string cur_path = "";
    //根节点
    Inode root_inode;
};

#endif
