<p align="center" style="display: flex; align-items: center; justify-content: center;">
  <img src="assets/logo-readme.png" alt="Aneiang.Pa" width="400" style="vertical-align: middle; border-radius: 8px;">
</p>

中文 | [English](README.md)

BoYa 是一款功能丰富的高级开源波形查看与分析工具，专为数字逻辑设计者和验证工程师打造。它同时支持多种格式文件，并提供了无缝的波形与RTL源代码关联的调试与追踪能力，详细且全面的波形标定与查看能力，优雅简洁的代码、波形界面，支持跨平台且免安装，都有相应的Windows版和Linux版二进制包，开箱即用。我们会在未来继续丰富、完善、扩展、演进这一新工具，让免费的波形调试不再痛苦。

<p align="center" style="display: flex; align-items: center; justify-content: center;">
  <img src="assets/example.png" alt="Aneiang.Pa" width="800" style="vertical-align: middle; border-radius: 8px;">
</p>

## 亮点
- 支持 VCD 和 FST 波形格式
- 支持波形与 SystemVerilog、Verilog、Chisel 源代码工程关联
- 支持 SystemVerilog/Verilog 与 Chisel Scala 代码对应
- 支持双击信号波形对应到RTL代码
- 支持信号的驱动源、负载追踪
- 支持信号全局搜索、波形值及其表达式值搜索
- 支持 windows、linux 平台


## 功能特性
### 文件与数据加载
- 支持多种波形格式：VCD、FST（也可支持通过第三方波形转换工具将其他波形文件转换为上述两种文件，如fsdb2vcd）
- 支持加载多个RTL源文件（.v、.sv）和文件列表
- 支持保存与加载工作区历史（包括信号列表和分组等历史信息）
### 信号管理与操作
- 单个/批量添加信号到波形
- 源代码区域单选、多选信号通过右键、快捷键、拖拽等方式加载到波形
- 添加、删除、重命名信号分组
- 信号同组、跨组移动；分组移动
- 移除波形中的信号
- 信号高亮与背景高亮
- 复制信号完整路径或当前值
- 将重要信号“置顶”在列表顶部
- 支持多比特信号展开、收缩查看
- 可切换数值显示格式（二进制、八进制、十进制、十六进制等）
- 支持多种逻辑表达式运算
- 支持信号位拼接
### 波形查看与分析
- 波形区域放大、缩小
- 支持多波形区域展示
- 跳转到自定义时刻
- Mark设置、跳转
- 边沿/跳变检测：支持按任意沿、上升沿、下降沿、特定值、信号变化进行跳转
- 统计选定区域内信号的上升沿/下降沿个数
- 信号搜索
### 源码集成与调试
- RTL源文件中可操作信号激活高亮展示
- 支持Verilog/SV与Scala代码对应
- 追踪信号的驱动源和负载
- 双击信号跳转到驱动源
- 拖拽信号跳转到负载
- 双击源码区域信号到驱动源
- 模块调用与定义跳转
- 鼠标悬浮显示信号当前时刻值
- 全局搜索
- 源代码文本搜索
- 输入行号跳转到对应源代码行
- 波形搜索的前进、后退
### 全局设置
- 三种主题模式切换
- 全局字体设置
- 全局快捷键配置
- 布局快速恢复

## 快速开始
- 可以从下方地址直接下载我们的二进制可执行文件，即刻体验，开箱即用。

### windows版本下载地址
- https://github.com/OS-CHIP/BoYa/releases/download/V1.0.2/BoYa.zip
### linux版本下载地址
- https://github.com/OS-CHIP/BoYa/releases/download/V1.0.2/BoYa-x86_64.AppImage

### Example 快速使用

1. **打开 BoYa 工具**
   - Windows: 运行 BoYa.exe
   - Linux: 运行 BoYa-x86_64.AppImage

2. **加载设计文件**
   - `File` → `Import Design` → 选择 `example/rtl/` 下所有 .v 文件

3. **打开波形文件**  
   - `File` → `Open File` → 选择 `example/sim/wave.fst` 或 `example/sim/wave.vcd`

4. **查看波形**
   - 通过源代码拖拽信号 或 点击 `Get Signals` 选择信号 添加到波形
   - 使用波形工具查看信号或调试
   
5. **详细文档与教程**
   - 如需查看完整的工具使用指南、高级功能教程和常见问题解答，请访问我们的官方Wiki文档站：https://github.com/OS-CHIP/BoYa/wiki


## 编译构建
- 如果您想自己从源代码级开始编译构建，可以参考以下文字：
  
### 构建要求（从源代码编译）
- **Qt:** 6.x 或更高版本（推荐 6.5+）
- **CMake:** 3.16 或更高版本
- **C++ 编译器:** 支持 C++17（例如 Windows 上的 MSVC 2019/2022，Linux 上的 GCC）

### 构建指南
``` bash
git clone https://github.com/OS-CHIP/BoYa.git
cd BoYa
```
#### 使用 Qt Creator（推荐用于开发）
1.  打开 Qt Creator。
2.  选择  `File` → `Open File or Project...` 然后导航到 `bo-ya/CMakeLists.txt` 文件。
3.  Qt Creator 将提示您配置项目。使用默认设置（选择工具包）。
4.  点击 `Build` (锤子) 图标或按 `Ctrl+B` 来构建项目。
5.  点击 `运行` (绿色箭头) 图标或按 `Ctrl+R` 来启动 BoYa。

## 使用方法
命令行选项
``` bash
BoYa [options] [source-files...]

Options:
  -h, --help                    Print usage information
  -f, --filelist <path>         Filelist input file path (can be specified multiple times)
  -w, --waveform <path>         Waveform input file path
  --source-files <files>        Direct source files (.v, .sv extensions)
```


## 反馈 & 贡献
我们欢迎任何反馈和贡献！您可以在 [Github 仓库](https://github.com/OS-CHIP/BoYa)上提交 Issue 和 Pull Request。

或者也可以通过邮箱：OSCHIP@126.com 来联络我们。

我们期待并重视您的反馈，这是我们前进的动力之一。

