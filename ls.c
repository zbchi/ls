#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<pthread.h>
#include<dirent.h>

//列出列表
void list_directory(const char*dirpath)
{
    DIR*dp;
    dp=opendir(dirpath);

    if(dp==NULL)
    {
        perror("opendir error");
    }

    struct dirent*dir;
    while((dir=readdir(dp))!=NULL)
    {
        printf("%s\n",dir->d_name);
    }
    closedir(dp);
}
void sys_err(const char*str)
{
    perror(str);
    exit(1);
}
int main(int argc,char*argv[])
{
    //获取当前工作目录
    char *pwd=(char*)malloc(sizeof(char)*1024);
    if(getcwd(pwd,1024)==NULL)
    {
        sys_err("getcwd error");
    }

    list_directory(pwd);
    
    return 0;
}