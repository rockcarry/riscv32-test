risc32-test 是 ffvm risc32 虚拟机的测试程序


测试程序基本都是用 c 语言编写，需要使用 riscv32-toolchain 编译
toolchain 地址：https://github.com/rockcarry/riscv32-toolchain

编译生成的镜像文件可以在 ffvm 上直接运行


======= 编译方法 =======

所有构建脚本和 Makefile 统一使用以下环境变量控制编译选项：

  FFVM_FLOAT=1    (默认) 启用单精度浮点 (rv32imacf)
  FFVM_FLOAT=0           禁用浮点 (rv32imac)
  FFVM_FFTASK=1  (默认) 启用 fftask 多任务
  FFVM_FFTASK=0          禁用 fftask
  FFVM_FATFS=1   (默认) 启用 FAT 文件系统
  FFVM_FATFS=0           禁用 FAT 文件系统

示例 - 默认全部开启构建：
  ./build-liblvgl.sh
  ./build-liblwip.sh
  ./fftask/build.sh
  make -C lvgltest clean && make -C lvgltest

示例 - 纯整数无浮点构建：
  FFVM_FLOAT=0 ./build-liblvgl.sh
  FFVM_FLOAT=0 ./build-liblwip.sh
  FFVM_FLOAT=0 ./fftask/build.sh
  FFVM_FLOAT=0 make -C lvgltest clean && FFVM_FLOAT=0 make -C lvgltest

配置保存在 config.mk 中，所有子目录 Makefile 通过 include 引用。


======= Toolchain 分支选择 =======

riscv32-toolchain 有两个分支，对应不同指令集：

  rv32imac   - 整数指令集 (不含浮点)
  rv32imacf  - 含单精度浮点


rockcarry
2020-12-24

