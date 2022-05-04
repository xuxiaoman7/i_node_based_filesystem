#ifndef INODE_H
#define INODE_H
#include<ctime>
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

    void set_id(int);
    void set_created_time(time_t);
    void set_modified_time(time_t);
    void set_byte_size(int);
    void set_direct_block_address(int[]);
    void set_indirect_block_address(int);
private:
    int id;
    time_t created_time;
    time_t modified_time;
    int byte_size;
    int direct_block_address[10];
    int indirect_block_address;
};
#endif