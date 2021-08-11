# JNIffmpegStaticPlay

a ffmpeg player in static so file

# 编译FFmpeg:

    采用CygWin环境编译，ubuntu模拟器或Linux更好

### CygWin:

* 可以从[Cygwin官网](http://www.cygwin.com/)上直接下载[setup-x86_64.exe](http://www.cygwin.com/setup-x86_64.exe)运行安装Cygwin
* 一直下一步。注意：如果提示"Could not download mirror sites list"
  时，则需手动添加镜像地址。国内比较快的两个镜像地址：
  * 163的镜像：http://mirrors.163.com/cygwin/
  * 搜狐的镜像：http://mirrors.sohu.com/cygwin/
* 选择需要下载安装的组件包
  * 此处，对于安装Cygwin来说，就是安装各种各样的模块而已。最核心的，记住一定要安装Devel这个部分的模块，其中包含了各种开发所用到的工具或模块尤其是这几个模块：
    ` gcc g++ make cmake automake gdb nasm yasm wget `
* 安装apt-cyg

  ```
   git clone https://github.com/transcode-open/apt-cyg.git
   cd apt-cyg 
   install apt-cyg /bin #将apt-cyg安装到/bin目录下/
  ```

  或者 `install apt-cyg /bin apt-cyg -m http://mirrors.163.com/cygwin`

  查找apt-cyg安装位置： `where apt-cyg`

  给他权限： `chmod u+x apt-cyg`
* 配置环境变量：
  计算机高级环境变量找到"Path"那一行，新建一个环境变量，然后输入：D:\Cygwin\bin

### 编译FFMPEG

    1.下载ndk，最好小于17版本 :  
    `wget https://dl.google.com/android/repository/android-ndk-r17c-linux-x86_64.zip`

    2.解压 
    `unzip android-ndk-r17c-linux-x86_64.zip`

------------------------------

1. 配置NDK环境变量：

   `vim /etc/profile` 末尾添加：

   ```
       NDKROOT=/usr/local/jesen0823/android-ndk-r17c  
       export PATH=$NDKROOT:$PATH
   ```

   qw退出保存 使它生效： `source /etc/profile`

2. 下载FFMPEG

   ```shell
        wget https://ffmpeg.org/releases/ffmpeg-4.0.2.tar.bz2
        bunzip2 ffmpeg-4.0.2.tar.bz2
        cd ffmpeg-4.0.2
   ```

   新建编译脚本： `vim build_android.sh` **静态库的编译脚本：**

```shell script
#!/bin/bash

#export TMPDIR=/usr/local/jesen0823/ffmpeg/tempdir
NDK_ROOT=/usr/local/jesen0823/android-ndk-r17c
#toolchain 变量指向ndk中的交叉编译gcc所在的目录
TOOLCHAIN=$NDK_ROOT/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
#FLAGS与INCLUDES变量 可以从AS ndk工程的.externativeBuild/cmake/debug/armeabi-v7a/build.ninja中拷贝
FLAGS="-isystem $NDK_ROOT/sysroot/usr/include/arm-linux-androideabi -D__ANDROID_API__=21 -g -DANDROID -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -Wa,--noexecstack -Wformat -Werror=format-security -std=c++11  -O0 -fPIC"

INCLUDES="-isystem $NDK_ROOT/sources/cxx-stl/llvm-libc++/include -isystem $NDK_ROOT/sources/android/support/include -isystem $NDK_ROOT/sources/cxx-stl/llvm-libc++abi/include"

# 执行configure脚本，用于生成makefile
#--prefix :安装目录
#--enable-small：优化大小
#--isable-programs：不编译ffmpeg可执行程序,我们需要获得静态(动态)库
#--disable-avdevice：关闭avdevice模块，此模块在android中无用
#--disable-encoderrs：关闭所有的编码器，只需要播放decoder即可。
#--disable-muxers：没有推流无需muxer模块（复用器，封装器），不需要生成mp4这种文件，所以关闭
#--disable-filters：关闭视频滤镜
#--enable-cross-compile：开启交叉编译(ffmpeg比较**跨平台**，并不是所有库都有这么happy的选项)
#--cross-prefix：ndk的特定gcc编译器前缀
#--disable-shared enable-static： 不写也可以，默认就是这样的
#--sysroot
#--extra-cflags
#--arch --target-os
#

PREFIX=./android/armeabi-v7a

./configure \
        --prefix=$PREFIX \
        --enable-small \                                                                                                        #--disable-yasm \                                                                                                       --disable-programs \
        --disable-avdevice \
        --disable-encoders \                                                                                                    --disable-muxers \
        --disable-filters \
        --enable-cross-compile \
        --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
        --disable-shared \
        --enable-static \
        --sysroot=$NDK_ROOT/platforms/android-21/arch-arm \
        --extra-cflags="$FLAGS $INCLUDES" \
        --extra-cflags="-isysroot $NDK_ROOT/sysroot" \
        --arch=arm \
        --target-os=android


make clean
make -j4
make install
```

3. 保存之后执行

```shell script
  sh build_android.sh
````

### 报错处理

1. 

```shell script
sh build_android.sh ./configure: line 9: $'\r': command not found ): No
such file or directoryg: setlocale: LC_ALL: cannot change locale (C ':
not a valid identifierort: `LC_ALL ./configure: line 13: $'\r': command
not found ./configure: line 16: $'\r': command not found ./configure:
line 17: syntax error near unexpected token `$'{\r'' '/configure: line
17: `try_exec(){ Makefile:177: /tests/Makefile: No such file or
directory make: *** No rule to make target '/tests/Makefile'. Stop.
Makefile:177: /tests/Makefile: No such file or directory make: *** No
rule to make target '/tests/Makefile'. Stop.
```

这种报错是编译的脚本含有dos格式所致，需要转为Linux系统所支持的unix格式：

安装格式转换工具：

```shell
apt-cyg install dos2unix
dos2unix build_android.sh
```

赋予权限:

```shell
chmod 777 build_android.sh
chmod +x build_android.sh
```

2. 

```shell script
/usr/local/jesen0823/android-ndk-r17c/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc is unable to create an executable file.
C compiler test failed.

If you think configure made a mistake, make sure you are using the latest
version from Git.  If the latest version fails, report the problem to the
ffmpeg-user@ffmpeg.org mailing list or IRC #ffmpeg on irc.freenode.net.
Include the log file "ffbuild/config.log" produced by configure as this will help
solve the problem.
Makefile:159: /tests/Makefile: No such file or directory
make: *** 没有规则可制作目标“/tests/Makefile”。 停止。
Makefile:159: /tests/Makefile: No such file or directory
make: *** 没有规则可制作目标“/tests/Makefile”。 停止。
Makefile:159: /tests/Makefile: No such file or directory
make: *** 没有规则可制作目标“/tests/Makefile”。 停止。
build.sh:行55: l: 未找到命令
```

还是格式问题，在ffmpeg命令下执行，给所有文件转格式，给权限：

```shell script
dos2unix *
dos2unix */*
```

### 附件

**动态库编译脚本：**

```shell script
#!/bin/bash
NDK_ROOT=/usr/local/jesen0823/android-ndk-r17c
#toolchain 变量指向ndk中的交叉编译gcc所在的目录
TOOLCHAIN=$NDK_ROOT/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64
#FLAGS与INCLUDES变量 可以从AS ndk工程的.externativeBuild/cmake/debug/armeabi-v7a/build.ninja中拷贝
FLAGS="-isystem $NDK_ROOT/sysroot/usr/include/aarch64-linux-android -D__ANDROID_API__=21 -g -DANDROID -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -march=armv8-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -Wa,--noexecstack -Wformat -Werror=format-security -std=c++11  -O0 -fPIC"

INCLUDES="-isystem $NDK_ROOT/sources/cxx-stl/llvm-libc++/include -isystem $NDK_ROOT/sources/android/support/include -isystem $NDK_ROOT/sources/cxx-stl/llvm-libc++abi/include"

# 执行configure脚本，用于生成makefile
#--prefix :安装目录
#--enable-small：优化大小
#--isable-programs：不编译ffmpeg可执行程序,我们需要获得静态(动态)库
#--disable-avdevice：关闭avdevice模块，此模块在android中无用
#--disable-encoderrs：关闭所有的编码器，只需要播放decoder即可。
#--disable-muxers：没有推流无需muxer模块（复用器，封装器），不需要生成mp4这种文件，所以关闭
#--disable-filters：关闭视频滤镜
#--enable-cross-compile：开启交叉编译(ffmpeg比较**跨平台**，并不是所有库都有这么happy的选项)
#--cross-prefix：ndk的特定gcc编译器前缀
#--disable-shared enable-static： 不写也可以，默认就是这样的
#--sysroot
#--extra-cflags
#--arch --target-os
#
#


PREFIX=./android/arm64-v8a

./configure \
--prefix=$PREFIX \
--enable-small \
--disable-programs \
--disable-avdevice \
--disable-encoders \
--disable-muxers \
--disable-filters \
--enable-cross-compile \
--cross-prefix=$TOOLCHAIN/bin/aarch64-linux-androideabi- \
--enable-shared \
--disable-static \hjjjkkj
--sysroot=$NDK_ROOT/platforms/android-21/arch-arm64 \
--extra-cflags="$FLAGS $INCLUDES" \
--extra-cflags="-isysroot $NDK_ROOT/sysroot" \
--arch=aarch64 \
--target-os=android


make clean
make -j4
make install

```

