#ifndef INODE_H
#define INODE_H
#include<ctime>

#define FILE_TYPE 1
#define DIRECTORY_TYPE 0

class Inode
{
public:
    Inode();
    ~Inode();
    
    int get_id();
    time_t get_created_time();
    time_t get_modified_time();
    int get_byte_size();
    int* get_direct_block_address();
    int get_indirect_block_address();
    int get_file_type();
    int get_file_count();

    void set_id(int);
    void set_created_time(time_t);
    void set_modified_time(time_t);
    void set_byte_size(int);
    void set_direct_block_address(int[]);
    void set_indirect_block_address(int);
    void set_file_type(int);
    void set_file_count(int);
private:
    int id;
    int file_type;   //1表示文件，0表示目录
    int file_count;  //如果是目录的话，表示当前目录下的文件数量
    time_t created_time;
    time_t modified_time;
    int byte_size;                   
    int direct_block_address[10];
    int indirect_block_address;
    int used_block;
};
#endif