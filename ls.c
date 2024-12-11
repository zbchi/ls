#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>

#define MAX_NAME_LEN 256

// 定义文件信息结构体
struct fileinfo
{
    char name[256];
    ino_t inode;
    off_t size;
    time_t mtime;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    nlink_t nlink;
    blkcnt_t blocks;
};
// 错误函数
void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

// 名称对比函数
int compare_name(const void *a, const void *b)
{
    struct fileinfo *A = (struct fileinfo *)a;
    struct fileinfo *B = (struct fileinfo *)b;

    return strcmp(A->name, B->name);
}
// 时间对比函数
int compare_time(const void *a, const void *b)
{
    struct fileinfo *A = (struct fileinfo *)a;
    struct fileinfo *B = (struct fileinfo *)b;

    if (A->mtime > B->mtime)
        return -1;
    else if (A->mtime < B->mtime)
        return 1;
    else
        return 0;
}

//-t反转函数
void reserve(struct fileinfo *arr, int count)
{
    for (int i = 0; i < count / 2; i++)
    {
        struct fileinfo temp = arr[i];
        arr[i] = arr[count - 1 - i];
        arr[count - 1 - i] = temp;
    }
}

// 打印权限函数
void printPermissions(mode_t mode)
{
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

// 打印文件类型
void printType(mode_t mode)
{
    if (S_ISDIR(mode))
    {
        printf("d");
    }
    else if (S_ISLNK(mode))
    {
        printf("l");
    }
    else if (S_ISREG(mode))
    {
        printf("-");
    }
    else if (S_ISCHR(mode))
    {
        printf("c");
    }
    else if (S_ISBLK(mode))
    {
        printf("b");
    }
    else if (S_ISFIFO(mode))
    {
        printf("p");
    }
    else
    {
        printf("?");
    }
}

// 打印输出函数，询问-l -i -s
void print(struct fileinfo *fp, int l, int i, int s, int maxLinkLen, int maxSizeLen, int maxBlocksLen)
{
    if (l)
    {
        if (i)
        {
            printf("%ld ", fp->inode);
        }
        if (s)
        {
            printf("%*ld ", maxBlocksLen, fp->blocks);
        }
        printType(fp->mode);
        printPermissions(fp->mode);
        printf(" %*ld", maxLinkLen, fp->nlink);

        struct passwd *pwd = getpwuid(fp->uid);
        printf(" %s", pwd->pw_name);
        struct group *grp = getgrgid(fp->gid);
        printf(" %s", grp->gr_name);

        printf(" %*ld", maxSizeLen, fp->size);

        struct tm *time;
        time = localtime(&(fp->mtime));
        char timeBuff[30];
        strftime(timeBuff, sizeof(timeBuff), "%m月 %d %H:%M", time);
        printf(" %s", timeBuff);

        printf(" %s", fp->name);

        printf("\n");
    }
    else
    {
        if (i)
        {
            printf("%ld ", fp->inode);
        }
        if (s)
        {
            printf("%*ld ", maxBlocksLen, fp->blocks);
        }
        printf(" %s", fp->name);
        printf("\n");
    }
}
// 主要函数
void list_directory(const char *dirpath, int a, int l, int R, int t, int r, int i, int s)
{
    if (dirpath == NULL)
        return;

    if (R)
    {
        printf("\n");
        if (dirpath[0] == '/' && dirpath[1] == '/')
            printf("%s:\n", dirpath + 1);
        else
            printf("%s:\n", dirpath);
    }

    DIR *dp = opendir(dirpath);
    if (dp == NULL)
    {
        perror("opendir error");
        return;
    }

    // 分配初始内存
    int capasity = 1024;
    struct fileinfo *fileinfos = (struct fileinfo *)malloc(sizeof(struct fileinfo) * capasity);
    if (fileinfos == NULL)
    {
        perror("malloc error");
    }

    int count = 0;
    struct stat filestat;
    struct dirent *dirent;
    while ((dirent = readdir(dp)) != NULL)
    {
        // 动态扩容
        if (count >= capasity)
        {
            capasity *= 2;
            fileinfos = realloc(fileinfos, capasity * (sizeof(struct fileinfo)));
        }
        if (fileinfos == NULL)
        {
            perror("realloc error");
        }

        // 实现-a，过滤隐藏文件
        if (!a && dirent->d_name[0] == '.')
        {
            continue;
        }

        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirpath, dirent->d_name);
        if (lstat(filePath, &filestat) == -1)
        {
            perror("stat error");
            // continue;
        }

        strcpy(fileinfos[count].name, dirent->d_name);
        fileinfos[count].inode = filestat.st_ino;
        fileinfos[count].size = filestat.st_size;
        fileinfos[count].mtime = filestat.st_mtime;
        fileinfos[count].mode = filestat.st_mode;
        fileinfos[count].uid = filestat.st_uid;
        fileinfos[count].gid = filestat.st_gid;
        fileinfos[count].nlink = filestat.st_nlink;
        fileinfos[count].blocks = filestat.st_blocks;
        count++;
    }

    // 按照名排序
    if (!t)
    {
        qsort(fileinfos, count, sizeof(struct fileinfo), compare_name);
    }

    // 实现-t参数，按照时间排序
    if (t)
    {
        qsort(fileinfos, count, sizeof(struct fileinfo), compare_time);
    }

    // 实现-r参数，反转顺序
    if (r)
    {
        reserve(fileinfos, count);
    }

    // 找出最大的长度以对齐
    int maxLink = 0, maxLinkLen = 0;
    int maxSize = 0, maxSizeLen = 0;
    int maxBlocks = 0, maxBlocksLen = 0;
    for (int j = 0; j < count; j++)
    {
        if (fileinfos[j].nlink > maxLink)
            maxLink = fileinfos[j].nlink;

        if (fileinfos[j].size > maxSize)
            maxSize = fileinfos[j].size;

        if (fileinfos[j].blocks > maxBlocks)
            maxBlocks = fileinfos[j].blocks;
    }
    while (maxLink != 0)
    {
        maxLink /= 10;
        maxLinkLen++;
    }
    while (maxSize != 0)
    {
        maxSize /= 10;
        maxSizeLen++;
    }
    while (maxBlocks != 0)
    {
        maxBlocks /= 10;
        maxBlocksLen++;
    }

    //*********核心打印**********
    for (int j = 0; j < count; j++)
    {
        print(&fileinfos[j], l, i, s, maxLinkLen, maxSizeLen, maxBlocksLen);
    }

    // printf("是目录的是：\n");
    int dirCount = 0;
    for (int j = 0; j < count; j++)
    {
        if (S_ISDIR(fileinfos[j].mode) && strcmp(fileinfos[j].name, ".") != 0 && strcmp(fileinfos[j].name, "..") != 0)
            dirCount++; // 计算目录数量
    }
    char **dirNames = (char **)malloc(dirCount * sizeof(char *));
    for (int j = 0, k = 0; j < count; j++)
    {
        if (S_ISDIR(fileinfos[j].mode) && strcmp(fileinfos[j].name, ".") != 0 && strcmp(fileinfos[j].name, "..") != 0)
        {
            // printf("%s\n",fileinfos[j].name);
            dirNames[k] = (char *)malloc(sizeof(char) * MAX_NAME_LEN);
            strcpy(dirNames[k++], fileinfos[j].name);
        }
    }
    free(fileinfos);
    fileinfos = NULL;

    // 实现-R，递归
    if (R)
    {
        for (int j = 0; j < dirCount; j++)
        {
            char newdirPath[1024];
            snprintf(newdirPath, sizeof(newdirPath), "%s/%s", dirpath, dirNames[j]);
            list_directory(newdirPath, a, l, R, t, r, i, s);
        }
    }

    free(dirNames);
    dirNames = NULL;
    closedir(dp);
}

int main(int argc, char *argv[])
{
    int opt;
    int a = 0, l = 0, R = 0, t = 0, r = 0, i = 0, s = 0;
    // 解析命令行参数
    // 获取当前工作目录
    char *path = ".";

    int pathCount = 0;

    while (optind < argc)
    {
        while ((opt = getopt(argc, argv, "alRtris")) != -1)
        {
            switch (opt)
            {
            case 'a':
                a = 1;
                break;

            case 'l':
                l = 1;
                break;

            case 'R':
                R = 1;
                break;

            case 't':
                t = 1;
                break;

            case 'r':
                r = 1;
                break;

            case 'i':
                i = 1;
                break;

            case 's':
                s = 1;
                break;
            }
        }

        if (pathCount > 0)
            optind++;

        if (optind < argc)
        {
            path = argv[optind];
            optind++;
            if (R == 0)
                printf("%s:\n", path);
            list_directory(path, a, l, R, t, r, i, s);
            pathCount++;
        }
    }

    if (pathCount == 0)
    {
        list_directory(path, a, l, R, t, r, i, s);
    }
    return 0;
}