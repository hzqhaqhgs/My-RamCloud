#include <string>
#include "test_inode.h"
#include <ramcloud/RamCloud.h>
#include <iostream>


#define BIG_CONSTANT(x) (x##LLU)
uint64_t murmur64( const void * key, int len, uint64_t seed )
{
  const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
  const int r = 47; 

  uint64_t h = seed ^ (len * m); 

  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data++;

    k *= m;  
    k ^= k >> r;  
    k *= m;  
    
    h ^= k;
    h *= m;  
  }
}

void InitStat(tablefs::tfs_stat_t &statbuf,
                       tablefs::tfs_inode_t inode,
                       mode_t mode,
                       dev_t dev) {
  statbuf.st_ino = inode;////节点 
  statbuf.st_mode = mode;//文件对应的模式，文件，目录，权限等
  statbuf.st_dev = dev;//特殊设备号码 
  statbuf.st_gid = 0;//组ID 
  statbuf.st_uid = 0;//用户ID 

  statbuf.st_size = 0;//文件字节数(文件大小) 
  statbuf.st_blksize = 0;//块大小(文件系统的I/O 缓冲区大小) 
  statbuf.st_blocks = 0; //块数 
  statbuf.st_nlink = 1;//文件的硬链接数
  
  time_t now = time(NULL);
  statbuf.st_atim.tv_nsec = 0;//最后一次访问时间微秒
  statbuf.st_mtim.tv_nsec = 0;//最后一次修改时间微秒
  statbuf.st_ctim.tv_sec = now;//最后一次状态改变时间S
  statbuf.st_ctim.tv_nsec = 0;//最后一次状态改变时间微秒
}

using namespace RAMCloud;
int main(int argc, char *argv[]){
	if (argc !=2){
		fprintf(stderr, "Usage: %s coordinatorLocator\n",argv[0]);
	}
	RamCloud cluster(argv[1],"__unamed__");
	uint64_t table=cluster.createTable("mytest");

	tablefs::tfs_meta_key_t mykey;
	tablefs::tfs_inode_val_t myval;
	int i;
	int len;
	char mypath[1024];

	mykey.inode_id=cluster.incrementInt64(table,"fileid",6,1);
	sprintf(mypath,"/testfs/%d/yyy/zzzz/testnode",1);///输入字符串
	len=strlen(mypath);
	mykey.hash_id=murmur64(mypath, len, 123); ///字符串转64hash值
	printf ("mypath: %s(%d) my inode id:%ld, myhash_id:%ld\n",mypath,len,mykey.inode_id, mykey.hash_id);
	std::string rmkey=mykey.ToString();
	myval.size=tablefs::TFS_INODE_HEADER_SIZE + len + 1;	
	myval.value= new char[myval.size];
	tablefs::tfs_inode_header* myheader = reinterpret_cast<tablefs::tfs_inode_header*>(myval.value);
	InitStat(myheader->fstat, mykey.inode_id, 666, 122);
	char* name_buffer = myval.value + tablefs::TFS_INODE_HEADER_SIZE;
	memcpy(name_buffer, mypath, len);
	myheader->has_blob = 0;
	myheader->namelen = len;
	name_buffer[myheader->namelen] = '\0';
	printf("%s\n",name_buffer);
	cluster.write(table,rmkey.c_str(),rmkey.size(),myval.value,myval.size);
	Buffer buffer;
	cluster.read(table,rmkey.c_str(),rmkey.size(), &buffer);
	//const char* result=static_cast<const char*>(buffer.getRange(0,buffer.size()));
	const tablefs::tfs_stat_t* result=reinterpret_cast<const tablefs::tfs_stat_t*>(buffer.getRange(0,buffer.size()));
	printf ("%d,%d\n",result->st_ino,result->st_mode);
	const tablefs::tfs_inode_header* result1=reinterpret_cast<const tablefs::tfs_inode_header*>(buffer.getRange(0,buffer.size()));
	printf ("%d,%d\n",result1->namelen,result1->has_blob);
	const char* result2=static_cast<const char*>(buffer.getRange(0,buffer.size()));
	const char* s=result2+tablefs::TFS_INODE_HEADER_SIZE;
	printf("With key %s filename is %s\n",rmkey.c_str(),s);
	const tablefs::tfs_meta_key_t* result_key=reinterpret_cast<const tablefs::tfs_meta_key_t*>(rmkey.c_str());
	printf ("%ld,%ld\n",result_key->inode_id,result_key->hash_id);
	

}
