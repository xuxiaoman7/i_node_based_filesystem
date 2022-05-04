#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include<fstream>
#include"Inode.h"
#include<string.h>
#define BLOCK_COUNT 16384
#define INODE_COUNT (1024*8)
#define BLOCK_SIZE 1024
#define INODE_SIZE 128
#define INODE_BITMAP_SIZE 1024
#define BLOCK_BITMAP_SIZE 1024
#define INODE_TABLE_SIZE (1024*1024)


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

    //初始化
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