# 文字编辑
**strings**  
returns each string of printable characters in files. Its main uses are to determine the contents of and to extract text from binary files (i.e., non-text files).

**xargs**  
将参数列表转换成小块分段传递给其他命令,以避免参数列表过长的问题  
Commands like `grep` and `awk` can accept the standard input as a parameter, or argument by using a pipe. 
However, others like `cp` and `echo` disregard the standard input stream and rely solely on the arguments found after the command. 
Additionally, under the Linux kernel before version 2.6.23, arbitrarily long lists of parameters could not be passed to a command, 
so `xargs` breaks the list of arguments into sublists small enough to be acceptable.

下面的命令:  
	
	rm `find /path -type f`
如果path目录下文件过多就会因为"参数列表过长"而报错无法执行.但改用xargs以后,问题即获解决.

	find /path -type f -print0 | xargs -0 rm
本例中xargs将find产生的长串文件列表拆散成多个子串,然后对每个子串调用rm.这样要比如下使用find命令效率高的多

**umask**  
	
	[eric@human ~]$ umask 
	0002
	[eric@human ~]$ umask -S
	u=rwx,g=rwx,o=rx
	[eric@human ~]$ touch file
	[eric@human ~]$ ll file
	-rw-rw-r--. 1 eric eric 0 May 19 12:31 file
	[eric@human ~]$ mkdir dir
	[eric@human ~]$ ll
	drwxrwxr-x. 2 eric eric    4096 May 19 12:39 dir

奇怪的是,怎么umask会有4组数字啊?第一组是特殊权限用的,我们看后面3组即可.

在默认权限的属性上,目录与文件是不一样的.由于我们不希望文件具有可执行的权力,默认情况中,文件是没有可执行(x)权限的.因此:  

- 若用户建立为”文件”则默认“没有可执行(x)项目”，即只有rw这两个项目,也就是最大为666分,默认属性如下:
`-rw-rw-rw-`
- 若用户建立为”目录”,则由于x与是否可以进入此目录有关,因此默认为所有权限均开放,即为777分,默认属性如下:
`drwxrwxrwx`

umask指定的是**该默认值需要减掉的权限**.因为r,w,x分别是4,2,1.  
所以,当要去掉能写的权限,就是输入2,而如果要去掉能读的权限,也就是4,那么要去掉读与写的权限,也就是6,而要去掉执行与写入的权限,也就是3.  
So, when creating a new file, the permission is: `the default - umask = -rw-rw-rw- - 0002 = -rw-rw-r--`

# 程序
**pgrep**  
`pgrep` looks through the currently running processes and **lists the process IDs** which matches the selection criteria to stdout. All the criteria have to match.  
`-n`: Select only the newest (most recently started) of the matching processes.
`shell> strace -c -p $(pgrep -n php-cgi)`

# 网络
**putty 中文乱码**  
Window->Appearance->Font: 选择Courier New  
Window->Translation->Remote character set: UTF-8  
之后, 在Session->Saved Sessions 中起一个名字, 然后Save, 下次使用的时候, 直接load, 就可以了

**PSCP**, the PuTTY Secure Copy client, and it is a command line application

The format for pscp is straight forward:

	pscp [options] source destination

To copy a Windows file to Linux system, at the DOS prompt, type

	pscp c:/music.mp3  ubuntu@10.0.0.3:/home/ubuntu/Music

The reverse works as well (copy Linux file to Windows)

	pscp ubuntu@10.0.0.3:/home/ubuntu/Music/music.mp3 c:/

**scp**  
secure copy, 用于在Linux下进行远程拷贝文件的命令,和它类似的命令有cp,不过cp只是在本机进行拷贝不能跨服务器,而且scp传输是加密的.可能会稍微影响一下速度。

1. 获取远程服务器上的文件

		scp -P 2222 root@www.vpser.net:/root/lnmp0.4.tar.gz /home/lnmp0.4.tar.gz
上端口大写P 为参数，2222 表示更改SSH端口后的端口，如果没有更改SSH端口可以不用添加该参数。 root@www.vpser.net 表示使用root用户登录远程服务器www.vpser.net，:/root/lnmp0.4.tar.gz 表示远程服务器上的文件，最后面的/home/lnmp0.4.tar.gz表示保存在本地上的路径和文件名。

2. 获取远程服务器上的目录

		scp -P 2222 -r root@www.vpser.net:/root/lnmp0.4/ /home/lnmp0.4/
上端口大写P 为参数，2222 表示更改SSH端口后的端口，如果没有更改SSH端口可以不用添加该参数。-r 参数表示递归复制（即复制该目录下面的文件和目录）；root@www.vpser.net 表示使用root用户登录远程服务器www.vpser.net，:/root/lnmp0.4/ 表示远程服务器上的目录，最后面的/home/lnmp0.4/表示保存在本地上的路径。

3. 将本地文件上传到服务器上

		scp -P 2222 /home/lnmp0.4.tar.gz root@www.vpser.net:/root/lnmp0.4.tar.gz
上端口大写P 为参数，2222 表示更改SSH端口后的端口，如果没有更改SSH端口可以不用添加该参数。 /home/lnmp0.4.tar.gz表示本地上准备上传文件的路径和文件名。root@www.vpser.net 表示使用root用户登录远程服务器www.vpser.net，:/root/lnmp0.4.tar.gz 表示保存在远程服务器上目录和文件名。

4. 将本地目录上传到服务器上

		scp -P 2222 -r /home/lnmp0.4/ root@www.vpser.net:/root/lnmp0.4/
上 端口大写P 为参数，2222 表示更改SSH端口后的端口，如果没有更改SSH端口可以不用添加该参数。-r 参数表示递归复制（即复制该目录下面的文件和目录）；/home/lnmp0.4/表示准备要上传的目录，root@www.vpser.net 表示使用root用户登录远程服务器www.vpser.net，:/root/lnmp0.4/ 表示保存在远程服务器上的目录位置。

# File System
`du`: 查看目录大小  
查看某个目录的大小 `/home/master/documents`  
查看目录下所有目录的大小并按大小降序排列:`sudo du -sm /etc/* | sort -nr | less`

`df`: 查看磁盘使用情况  
于du不同的是,du是面向文件的命令,只计算被文件占用的空间.不计算文件系统metadata 占用的空间.df则是基于文件系统总体来计算,通过文件系统中未分配空间来确定系统中已经分配空间的大小

# 杂项
**man**  
`man number item`: find item in section number, eg: `man 3 fopen`  
`man -k word`: 关键字查找, 查找包含word(命令本身或者解释中) 的command  
`man -f word`: 根据关键字在联机帮助中搜索完全匹配的条目, Equivalent to whatis  
把man page 转成文本文件,如: `man ls | col -b > ~/Desktop/man_ls.txt`  
`man -t ls > man_ls.ps && ps2pdf man_ls.ps && rm man_ls.ps`: print