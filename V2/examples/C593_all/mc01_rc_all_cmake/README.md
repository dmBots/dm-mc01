# DM-MC01 C593 综合例程

本工程的目标芯片为 **STM32C593RGT6**，使用 CMake、Ninja 和 GNU Arm 工具链构建。
本文说明如何配置电脑上的工具，以及在 VS Code 中编译、下载和调试。
安装步骤以 Windows 为例；仓库中的 `.vscode` 不绑定维护者的安装盘符或探针编号。

## 1. 准备工具

| 工具 | 要求或用途 |
| --- | --- |
| VS Code | 编辑代码和运行构建、调试任务 |
| STM32CubeCLT | 提供 CMake、Ninja 和 GNU Arm 工具链；本工程已验证 1.22.0 / GCC 14.3.1 |
| CMake | 版本不低于 3.30 |
| Python 和 pyOCD | 通过 CMSIS-DAP / DAPLink 探针下载、调试；本工程已验证 pyOCD 0.45.1 |
| VS Code 扩展 | CMake Tools、C/C++、Cortex-Debug，已列在 `.vscode/extensions.json` 中 |

如果已有其他来源的 CMake、Ninja 或 GNU Arm 工具链，也应先确认版本及实际使用的程序路径。
不要把电脑上安装的编译器、Python 虚拟环境或 `build` 目录一起提交到仓库。

## 2. 修改工具路径：优先配置 PATH

`PATH` 是系统查找程序的目录列表。本例程通过程序名运行工具，不要求安装到某个固定盘符。

在 Windows 中搜索“编辑账户的环境变量”，编辑当前用户的 `Path`，加入以下目录。
下表的 `C:/ST/STM32CubeCLT_1.22.0` **只是示例，必须换成你自己的实际安装目录**。

| 加入 Path 的目录示例 | 目录内应有的程序 |
| --- | --- |
| `C:/ST/STM32CubeCLT_1.22.0/CMake/bin` | `cmake.exe` |
| `C:/ST/STM32CubeCLT_1.22.0/Ninja/bin` | `ninja.exe` |
| `C:/ST/STM32CubeCLT_1.22.0/GNU-tools-for-STM32/bin` | `arm-none-eabi-gcc.exe`、`arm-none-eabi-gdb.exe`、`arm-none-eabi-objdump.exe`、`arm-none-eabi-nm.exe` 等 |

Python 安装完成后，在未激活虚拟环境的 PowerShell 中安装 pyOCD：

```powershell
python -m pip install --user pyocd==0.45.1
python -c "import sysconfig; print(sysconfig.get_path('scripts', 'nt_user'))"
```

第二条命令输出用户级 Python `Scripts` 目录，把这个目录也加入用户 `Path`，其中应能找到 `pyocd.exe`。
如果运行 `python` 打开了应用商店，应先完成 Python 安装并检查其命令路径。

修改环境变量后，关闭所有 VS Code 窗口，再重新打开工程。在新终端逐条运行：

```powershell
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
pyocd --version
```

每条命令都应显示对应工具的版本。提示“找不到命令”时，先修正 PATH，不要急着修改工程源码。
若安装了多套工具，可用 `Get-Command cmake,ninja,arm-none-eabi-gcc,pyocd` 查看当前实际使用的路径。

## 3. 在 VS Code 中打开、编译

1. 选择“文件 → 打开文件夹”，打开包含本 README 和 `CMakeLists.txt` 的 `mc01_rc_all_cmake` 目录。
   不要只打开 `.vscode`，也不要把整个资料仓库根目录当作此工程的工作区。
2. 按扩展推荐安装 CMake Tools、C/C++ 和 Cortex-Debug。
3. 执行命令面板中的 `CMake: Select Configure Preset`，选择 `debug_GCC_STM32C593RGT6`。
4. 按 `Ctrl+Shift+B` 运行默认 Debug 构建。任务会先配置 CMake，再编译，不需要手工创建 `build`。

生成结果位于：

```text
build/debug_GCC_STM32C593RGT6/mc01_rc_all.elf
build/debug_GCC_STM32C593RGT6/mc01_rc_all.hex
build/debug_GCC_STM32C593RGT6/mc01_rc_all.bin
```

不使用 VS Code 时，也可在本工程目录运行：

```powershell
cmake --preset debug_GCC_STM32C593RGT6
cmake --build --preset debug_GCC_STM32C593RGT6 --parallel
```

构建 Release 则改用另一组预设：

```powershell
cmake --preset release_GCC_STM32C593RGT6
cmake --build --preset release_GCC_STM32C593RGT6 --parallel
```

Release 产物位于 `build/release_GCC_STM32C593RGT6/`。当前 `Ctrl+Shift+B` 和 F5 入口固定使用 Debug，
不会因为另一个目录里已有 Release 产物就自动改为下载 Release。

## 4. 下载和调试

下载会覆盖开发板上的原程序，操作前请确认目标板和供电，并按开发板说明连接调试接口。
首次使用只连接一个 CMSIS-DAP / DAPLink 探针，不需要填写其他人的探针编号。

- 只查看探针：运行任务 `pyOCD: list probes`。
- 下载并调试：按 F5，选择 `DAPLink + pyOCD: 下载并调试`。它会先构建 Debug，再下载，并停在 `main`。
- 只下载：运行任务 `pyOCD: flash STM32C593RGT6`。它会先构建 Debug，再下载 ELF 并复位。

芯片支持文件为 `tools/packs/STMicroelectronics.stm32c5xx_dfp.2.1.0.pack`，
配置文件为 `pyocd.yaml`，请保持它们与工程一起使用。

F5 配置保留了本例程已有的下载后启动、复位命令；“只下载”任务不执行这些额外的 GDB 命令。
若下载后没有运行，先使用 F5 检查启动过程，不要盲目修改选项字节或照搬其他芯片的寄存器命令。

### 同时连接多个探针时怎么改

先用 `pyOCD: list probes` 查看要使用的探针编号。最简单的做法是断开其他探针。
确实需要多探针同时连接时，分别设置：

1. `.vscode/launch.json`：在 `serverArgs` 数组中加入 `"--probe", "YOUR_PROBE_ID"`。
2. `.vscode/tasks.json`：在烧录任务的 `args` 数组中加入 `"--uid", "YOUR_PROBE_ID"`。

`YOUR_PROBE_ID` 要替换成你的真实探针编号。JSON 数组中的相邻元素需要用逗号分隔。
这是本机连接选择，不要把个人探针编号提交到公共仓库。
仅更换探针时，不需要修改 `stm32c593rgt6` 这个目标芯片名称。

## 5. 后续修改应从哪里开始

| 想修改的内容 | 应检查的位置 |
| --- | --- |
| 工具安装目录 | 优先修改电脑的用户 PATH；通常不用改 `.vscode` |
| 构建命令、默认构建任务 | `.vscode/tasks.json` |
| 下载调试入口、探针选择 | `.vscode/launch.json` |
| Debug / Release 预设和输出目录 | `CMakePresets.json`；改目录后还要同步检查调试 ELF 路径 |
| pyOCD 芯片名称、通信频率、芯片支持包 | `pyocd.yaml` 和 `.vscode/launch.json` |
| 各外设初始化、演示的调用顺序 | `main.c` |
| 目标温度、加热上限、测试时间等参数 | `board/board_config.h`，先阅读参数单位说明 |
| 某一项演示的实现、接线说明 | `app` 对应模块及头文件；底层硬件操作在 `board/bsp` |

换成不同型号的 MCU 不是只改一个 `targetId`：还涉及启动文件、链接配置、外设配置等。
本工程的构建和调试配置面向 STM32C593RGT6，不应直接用于其他芯片。

## 6. 常见问题

| 现象 | 先检查什么 |
| --- | --- |
| 找不到 CMake、Ninja、GCC 或 pyOCD | PATH 是否指向实际可执行文件所在目录；修改后是否重启了 VS Code |
| F5 提示找不到 GDB、objdump 或 nm | GNU Arm 的整个 `bin` 目录是否加入 PATH，而不只是单独配置 gcc |
| 打开源码出现大量头文件错误 | 是否打开了正确工程目录，是否安装 CMake Tools 并完成对应预设的配置 |
| CMake 提示缓存的源码目录与当前目录不同 | 不要复用从其他位置复制来的 `build`；先移走旧 `build`，再重新配置 |
| 找不到目标芯片或支持包 | `pyocd.yaml` 是否随工程存在，`tools/packs` 下的 `.pack` 文件是否完整 |
| 提示有多个探针或连接了错误的板子 | 先只连接一个探针，或按上一节分别配置调试、烧录任务的编号 |

`build/`、`.venv/` 和个人使用的 `CMakeUserPresets.json` 已在本工程 `.gitignore` 中排除。
公共配置应保留通用设置，不提交本机绝对路径、个人探针编号和构建缓存。

## 参考文档

- [STM32CubeCLT 官方工具页面](https://www.st.com/en/development-tools/stm32cubeclt.html)
- [VS Code 变量：workspaceFolder、env 等](https://code.visualstudio.com/docs/reference/variables-reference)
- [Cortex-Debug 调试配置字段](https://github.com/Marus/cortex-debug/blob/master/debug_attributes.md)
- [pyOCD 命令行说明](https://pyocd.io/docs/command_reference.html)
