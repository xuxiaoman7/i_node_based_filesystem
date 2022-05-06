#include"Inode.h"
#include<string.h>
Inode::Inode()
{
    id = -1;
    file_type = DIRECTORY_TYPE;   //默认为目录
    file_count = 0;
    created_time = time(0);
    modified_time = created_time;
    byte_size = 0;
    for(int i = 0; i < 10; i++)
    {
        direct_block_address[i] = -1;
    }
    //memset(direct_block_address,0,sizeof(direct_block_address));
    indirect_block_address = -1;
}

Inode::~Inode()
{

}

int Inode::get_id()
{
    return id;
}

int Inode::get_byte_size()
{
    return byte_size;
}

time_t Inode::get_created_time()
{
    return created_time;
}

time_t Inode::get_modified_time()
{
    return modified_time;
}

int* Inode::get_direct_block_address()
{
    return direct_block_address;
}

int Inode::get_indirect_block_address()
{
    return indirect_block_address;
}

int Inode::get_file_type()
{
    return file_type;
}

int Inode::get_file_count()
{
    return file_count;
}

void Inode::set_id(int id)
{
    this->id = id;
}

void Inode::set_byte_size(int byte_size)
{
    this->byte_size = byte_size;
}

void Inode::set_created_time(time_t created_time)
{
    this->created_time = created_time;
}

void Inode::set_modified_time(time_t modified_time)
{
    this->modified_time = modified_time;
}

void Inode::set_direct_block_address(int direct_block_address[])
{
    memcpy(this->direct_block_address,direct_block_address,10*sizeof(int));
}

void Inode::set_indirect_block_address(int indirect_block_address)
{
    this->indirect_block_address = indirect_block_address;
}

void Inode::set_file_type(int type)
{
    file_type = type;
}

void Inode::set_file_count(int count)
{
    file_count = count;
}