# 基于 libghttp 的七牛文件上传

## 背景

此项目主要用来解决嵌入式设备下内存和存储空间有限的情况下，使用表单或分片的方式将文件上传到七牛云存储。

## 编译

### 安装 cjson 库

```
# apt install -y libcjson-dev
```

### 安装 libghttp
首先需要安装 libghttp 库到 arm 的系统中。

下载源码：

```
$ git clone https://github.com/sknown/libghttp.git
$ ./configure
$ make
$ sudo make install
```

上面的命令，把 libghttp 库默认安装到`/usr/local/lib`目录下面，头文件在`/usr/local/include`目录下面。

### 配置头文件
由于项目依赖了 libghttp 库中的 base64 编码，所以需要将当前目录下的头文件拷贝一份都 `/usr/local/include/ghttp` 下面。
例如：
```shell
# ls -lh /usr/local/include/ghttp/
total 44K
-rwxr-xr-x 1 root root 5.5K Apr 13 10:24 ghttp.h
-rwxr-xr-x 1 root root 3.5K Apr 13 10:24 ghttp_constants.h
-rwxr-xr-x 1 root root 1.1K Apr 13 10:24 http_base64.h
-rwxr-xr-x 1 root root 1.1K Apr 13 10:24 http_date.h
-rwxr-xr-x 1 root root 1.2K Apr 13 10:24 http_global.h
-rwxr-xr-x 1 root root 2.2K Apr 13 10:24 http_hdrs.h
-rwxr-xr-x 1 root root 2.2K Apr 13 10:24 http_req.h
-rwxr-xr-x 1 root root 2.5K Apr 13 10:24 http_resp.h
-rwxr-xr-x 1 root root 2.9K Apr 13 10:24 http_trans.h
-rwxr-xr-x 1 root root 1.5K Apr 13 10:24 http_uri.h
```
### 编译本项目
然后下载本项目源代码，使用下面方法编译：

```
$ make all
```

## 使用说明

1. 使用样例在 [examples](examples/) 目录下面。
2. 建议使用源码的方式将功能嵌入到自己的项目中。
3. 图片类小文件建议采用表单（form）上传效率较高，表单上传时 post body 的最大支持字节可以通过 `ghttp-qiniu.h` 文件中的变量 `QN_MULTIDATA_FORM_SIZE` 调整，默认为 512KB；请注意设置得比需要支持上传的最大文件大一点，因为表单上传时还会有一些额外的字节（例如分隔符，自定义参数等）。
4. 视频类大文件建议采用分片（chunk）上传效率较高；分片上传默认支持断点续传功能；分片的默认大小为 1MB，通过 `ghttp-qiniu.h` 文件中的变量 `QN_CHUNK_SIZE` 定义；
5. 断点续传是利用分片上传的机制实现的，即将已上传成功的分片的 etag 记录在本地文件中。在上传中断后重新上传时，会从上次上传成功的分片的下一个分片继续上传。默认情况下可以不指定 put_extra 变量中的 recorder_key，因为默认会用 base64(local_path+bucket_name+file_key)作为 recorder_key。注意当本地文件的大小或最后修改时间发生了变化，或者 UploadId 已经失效的情况下，会自动忽略本地的 recorder_key 文件，重新申请新的 UploadId 进行重新上传。

## 技术支持

可以通过以下方式联系我：

![技术支持](images/wechat.png)

## 参考文档

[表单上传](https://developer.qiniu.com/kodo/1312/upload)
[分片上传(v2)](https://developer.qiniu.com/kodo/6365/initialize-multipartupload)

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