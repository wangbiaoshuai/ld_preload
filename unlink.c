#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "unlink.h"
#include <errno.h>

#define __GNU_SOURCE

typedef int (*orig_unlink_f_type)(const char *pathname);
typedef int (*orig_unlink_at_f_type)(int dirfd, const char *pathname, int flags);
typedef int (*orig_rename_f_type)(const char *oldpath, const char *newpath);
typedef int (*orig_rename_at_f_type)(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
typedef ssize_t(*orig_write_f_type)(int fd, const void *buf, size_t count);
typedef int (*org_kill_f_type)(pid_t pid, int sig);

static const char* g_fix_path[] = {"/usr/local/service",
                                   "/usr/local/mysql/data/cems",
                                   "/usr/local/FastDFS/data",
                                   "\0"};
static const char* g_unlimit_proc[] = {"cems",
                                       "mysql",
                                       "mysqld",
                                       "mysqld_safe",
                                       "fdfs",
                                       "elasticsearch",
                                       "ftp",
                                       "redis",
                                       "\0"};
//static int g_fix_size = sizeof(g_fix_path) - 1;
static const char g_evil_env[] = "AVOID_PLD_DEDP_LONG_NAME_WILL_BE_HARDER_FOR_TYPE_AND_WILL_GET_MORE_TYPO";
//static const char g_fix_path_bin[] = "/opt/edp_vrv/bin";
//static const char g_fix_path_lib[] = "/opt/edp_vrv/lib";
typedef enum
{
	true=1, false=0
}bool;

static int __preload_flag = 0;

void __attribute__ ((constructor)) __attach(void);
void __attribute__ ((destructor)) __detach(void);

bool is_protect_dir(const char* path)
{
    if(path == NULL)
    {
        return false;
    }
    const char** protect_path = g_fix_path;
    while(strlen(*protect_path) != 0)
    {
        if(strncmp(*protect_path, path, strlen(*protect_path)) == 0)
        {
            return true;
        }
        protect_path ++;
    }
    return false;
}

bool is_unlimit_proc(const char* proc)
{
    if(proc == NULL)
    {
        return false;
    }

    const char** unlimit_proc = g_unlimit_proc;
    while(strlen(*unlimit_proc) != 0)
    {
        if(strstr(proc, *unlimit_proc) != NULL)
        {
            return true;
        }
        unlimit_proc ++;
    }
    return false;
}


void __attach(void) {
/*    char *penv = getenv(g_evil_env);
    if(penv == NULL) {
        return;
    }
    if(atoi(penv)) {
        __preload_flag = 1;
    }
    return;*/
    
    char buf[512] = {0};
    int res = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if(res < 0)
    {
        printf("readlink failed.\n");
        return;
    }
    
   // if(strstr(buf, "cems") != NULL)
    if(is_unlimit_proc(buf))
    {
        __preload_flag = 1;
    }
    else
    {
        __preload_flag = 0;
    }
    return;
}

void __detach(void) {
    __preload_flag = 0;
}

#if 0
#define GET_DIR_INODE(dir_path, inode_read) { \
    DIR *pdir = opendir(dir_path); \
    struct dirent *pdirent = readdir(pdir); \
    ino_t inode_read = pdirent->d_ino; \
    closedir(pdir); \
    pdir = NULL; \
    pdirent = NULL \
}
#endif
void reverse(char  *str, int len)           //字符串的逆序函数
{
	char temp;
	int i;
	for(i = 0; i < len / 2; i++)
	{
		temp = *(str + i);
		*(str + i) = *(str + len - i - 1);
		*(str + len - 1 - i) = temp;
	}
}

char* itoa(int num, char* str, int base)
{
	int i = 0;
	bool isNegative = false;
	if (num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	if (num < 0 && base == 10)
	{
		isNegative = true;
		num = -num;
	}

	while (num != 0)
	{
		int rem = num % base;
		str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
		num = num/base;
	}

	if (isNegative){
	  str[i++] = '-';
	}
	str[i] = '\0'; 
	reverse(str, i);

	return str;
}

int unlink(const char *pathname) {
    //printf("unlink %s\n", pathname);
    orig_unlink_f_type orig_unlink;
    orig_unlink = (orig_unlink_f_type)dlsym(RTLD_NEXT,"unlink");
    if(__preload_flag) {
        return orig_unlink(pathname);
    }
    if(pathname == NULL) {
        return orig_unlink(pathname);
    }
    //printf("unlink update %s\n", pathname);
    char buf[PATH_MAX] = {0};
    char *pret = realpath(pathname, buf);
    if(pret == NULL) {
        return orig_unlink(pathname);
    }
    /*if(strncmp(g_fix_path, buf, g_fix_size) == 0) {
        //printf("catch buf: %s\n", buf);
        return 0;
    }*/
    if(is_protect_dir(buf))
    {
        errno = 1;
        return -1;
    }
    return orig_unlink(pathname);
}

int unlinkat(int dirfd, const char *pathname, int flags) {

    orig_unlink_at_f_type orig_unlink_at;
    orig_unlink_at = (orig_unlink_at_f_type)dlsym(RTLD_NEXT,"unlinkat");
    if(__preload_flag) {
        return orig_unlink_at(dirfd, pathname, flags);
    }

    if(pathname == NULL) {
        return orig_unlink_at(dirfd, pathname, flags);
    }

    char link_path[PATH_MAX] = {0};
    char target[PATH_MAX] = {0};
	int number = dirfd;
    char str[256] = {0};
	itoa(number, str, 10);
	strcat(target,"/proc/self/fd/");
	strcat(target,str);
    readlink(target, link_path, PATH_MAX);
    /*if(strncmp(g_fix_path, link_path, g_fix_size) == 0) {
        //printf("%s\n", "-----------------------");
        return 0;
    }*/
    if(is_protect_dir(link_path))
    {
        errno = 1;
        return -1;
    }

#if 0
    struct stat finfo;
    memset(&finfo, 0, sizeof(finfo));
	fstat(dirfd, &finfo);
    DIR *pdir = opendir(g_fix_path);
    struct dirent *pdirent = readdir(pdir);
    ino_t inode_read = pdirent->d_ino;
    closedir(pdir);
    if(inode_read == finfo.st_ino) {
        printf("%s\n", "same inode ");
        return 0;
    } 
#endif
    /*for in dir delete*/
    char buf[PATH_MAX] = {0};
    char *pret = realpath(pathname, buf);
    if(pret == NULL) {
        return orig_unlink_at(dirfd, pathname, flags);
    }

    //printf("unlinkat REPATH update %s\n", buf);

    /*if(strncmp(g_fix_path, buf, g_fix_size) == 0) {
        //printf("%s\n", "not unlinkat return");
        return 0;
    }*/
    if(is_protect_dir(buf))
    {
        errno = 1;
        return -1;
    }
    return orig_unlink_at(dirfd, pathname, flags);
}

int rename(const char *oldpath, const char *newpath) {
    orig_rename_f_type orig_rename;
    orig_rename = (orig_rename_f_type)dlsym(RTLD_NEXT,"rename");
    if(__preload_flag) {
        return orig_rename(oldpath, newpath);
    }

    if(oldpath == NULL || newpath == NULL) {
        return orig_rename(oldpath, newpath);
    }

    //printf("rename old new %s %s\n", oldpath, newpath);

    char buf[PATH_MAX] = {0};
    char *pret = realpath(oldpath, buf);
    if(pret == NULL) {
        return orig_rename(oldpath, newpath);
    }
    /*if(strncmp(g_fix_path, buf, g_fix_size) == 0) {
        return 0;
    }*/
    if(is_protect_dir(buf))
    {
        errno = 1;
        return -errno;
    }
    memset(buf, 0, PATH_MAX);
    pret = NULL;
    pret = realpath(newpath, buf);
    if(pret == NULL) {
        return orig_rename(oldpath, newpath);
    }
    /*if(strncmp(g_fix_path, buf, g_fix_size) == 0) {
        return 0;
    }*/
    if(is_protect_dir(buf))
    {
        errno = 1;
        return -1;
    }
    return orig_rename(oldpath, newpath);
}

int renameat(int olddirfd, const char *oldpath,
		    int newdirfd, const char *newpath) {
    orig_rename_at_f_type orig_rename_at;
    orig_rename_at = (orig_rename_at_f_type)dlsym(RTLD_NEXT,"renameat");
    if(__preload_flag) {
        return orig_rename_at(olddirfd, oldpath, newdirfd, newpath);
    }

    if(oldpath == NULL || newpath == NULL) {
        return orig_rename_at(olddirfd, oldpath, newdirfd, newpath);
    }

    //printf("rename at old new %s %s\n", oldpath, newpath);

    char buf[PATH_MAX] = {0};
    char *pret = realpath(oldpath, buf);
    if(pret == NULL) {
        return orig_rename_at(olddirfd, oldpath, newdirfd, newpath);
    }
    /*if(strncmp(g_fix_path, buf, g_fix_size) == 0) {
        return 0;
    }*/
    if(is_protect_dir(buf))
    {
        errno = 1;
        return -1;
    }
    memset(buf, 0, PATH_MAX);
    pret = NULL;
    pret = realpath(newpath, buf);
    if(pret == NULL) {
        return orig_rename_at(olddirfd, oldpath, newdirfd, newpath);
    }
    /*if(strncmp(g_fix_path, buf, g_fix_size) == 0) {
        return 0;
    }*/
    if(is_protect_dir(buf))
    {
        errno = 1;
        return -1;
    }
    return orig_rename_at(olddirfd, oldpath, newdirfd, newpath);
}

ssize_t write(int fd, const void *buf, size_t count) {
    orig_write_f_type orig_write;
    orig_write = (orig_write_f_type)dlsym(RTLD_NEXT,"write");
    if(__preload_flag) {
        return orig_write(fd, buf, count);
    }
    char link_path[PATH_MAX] = {0};
    char target[PATH_MAX] = {0};
	int number = fd;
	char str[256] = {0};
	itoa(number, str, 10);
	strcat(target,"/proc/self/fd/");
	strcat(target,str);
    readlink(target, link_path, PATH_MAX);
    /*if(strncmp(g_fix_path, link_path, g_fix_size) == 0) {
        return -1;
    }*/
    if(is_protect_dir(link_path))
    {
        errno = 1;
        return -1;
    }
    return orig_write(fd, buf, count);
}

int kill(pid_t pid, int sig) {
    org_kill_f_type orig_kill;
    orig_kill = (org_kill_f_type)dlsym(RTLD_NEXT,"kill");
    if(__preload_flag) {
        return orig_kill(pid, sig);
    }
    char link_path[PATH_MAX] = {0};
    char target[PATH_MAX] = {0};
	int number = pid;
	char str[256] = {0};
	itoa(number, str, 10);
	strcat(target,"/proc/");
	strcat(target,str);
	strcat(target,"/exe");

    readlink(target, link_path, PATH_MAX);
    //if(strncmp(g_fix_path, link_path, g_fix_size) == 0) {
    if(is_protect_dir(link_path)){
        /*we must under the true god control, show respect to the father.*/
        if(getpid() == 1) {
            return orig_kill(pid, sig);
        }
        errno = 1;
        return -1;
    }
    return orig_kill(pid, sig);
}


int get_fake_flag() {
    return __preload_flag;
}

int set_fake_flag(int flag) {
    __preload_flag = flag;
    return __preload_flag;
}

void __private_get_evil_env__x_avoid(char *buffer) {
    memset(buffer, 0, 128);
    sprintf(buffer, "%s", g_evil_env);
}
