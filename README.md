# 基于 libghttp 的七牛文件上传

## 背景

有些情况下，嵌入式设备文件上传需要的网络库不可以太大，诸如 curl 之类的库可能就太大了，好在有一种 libghttp 的库，可以用来发送 http 请求。

## 编译

首先需要安装 libghttp 库到 arm 的系统中。

下载源码：

```
$ git clone https://github.com/sknown/libghttp.git
$ ./configure
$ make
$ sudo make install
```

上面的命令，把 libghttp 库默认安装到`/usr/local/lib`目录下面，头文件在`/usr/local/include`目录下面。

然后下载本项目源代码，使用下面方法编译：

```
$ make
```

## FAQ

1. 运行的时候报错：
>./upload: error while loading shared libraries: libghttp.so.1: cannot open shared object file: No such file or directory

解决方案，如果共享库文件安装到了`/usr/local/lib`(很多开源的共享库都会安装到该目录下)或其它"非`/lib`或`/usr/lib`目录下, 那么在执行`ldconfig`命令前, 还要把新共享库目录加入到共享库配置文件/etc/ld.so.conf中, 如下:

```
# cat /etc/ld.so.conf
include ld.so.conf.d/*.conf
# echo "/usr/local/lib" >> /etc/ld.so.conf
# ldconfig
```