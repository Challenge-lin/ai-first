# Keil 工程说明（firmware/project）

## 1. 文件与格式

| 文件 | 说明 |
| --- | --- |
| `51car.uvproj` | Keil uVision4 工程格式（`SchemaVersion 1.1`），本目录唯一的工程入口 |
| `51car.uvopt` | uVision 自动生成的用户级配置（窗口布局、断点），已加入 `.gitignore`，不用提交 |

工程在 **uVision 5.14 + C51 V9.54** 上验证过可以打开并进入编译流程。uVision5 打开 `.uvproj` 时若提示“转换旧版本工程”，选择“是/保存”即可，会另存出 `51car.uvprojx`；不转换也能直接编译。

目标名：`51car`。

## 2. 打开工程

1. 双击 `firmware\project\51car.uvproj`，或 uVision 菜单 `Project → Open Project`。
2. 左侧 Project 窗口应看到：`main`、`app`、`drivers\*`（8 个）、`common`。
3. `main` 分组下已挂 `..\main\main.c`（占位文件，只有空 `while(1)`，用于验证工程可编译）。
4. 按 `F7` 或 `Project → Build Target` 编译。

## 3. 器件选择与差异

工程当前器件是 **Atmel AT89C52**，作为 STC89C52RC 的最接近等价项：

| 项目 | AT89C52（工程当前） | STC89C52RC（实际芯片） | 影响 |
| --- | --- | --- | --- |
| 内核 | 8052 | 8052 | 一致 |
| Flash | 8KB | 8KB | 一致 |
| RAM | 256B | 512B（256 内部 + 256 扩展） | 编译器按 256B 约束，偏保守 |
| 定时器 | 3 个（T0/T1/T2） | 3 个 | 一致 |
| 串口 | 1 个 | 1 个 | 一致 |
| 特殊功能 | 无 ISP / EEPROM / WDT | 有 ISP、EEPROM、看门狗 | 本方案不依赖，见 `config_build.h` 的 `CFG_USE_WATCHDOG=0` |

结论：用 AT89C52 编译不会生成 STC 专有指令，代码可直接烧到 STC89C52RC；代价是 RAM 只按 256B 保证，若后续变量超过 256B，需要把大数组显式放到 `xdata`（STC 的 256B 扩展 RAM），或精简缓冲。

**需要手动确认**：如果你的 Keil 安装了 STC 器件包，可在 `Project → Options for Target → Device` 里改选 `STC89C52RC`，其余设置不用动。没有 STC 器件包就保持 AT89C52。

时钟已在工程的 `Cpu` 字段写成 `CLOCK(11059200)`，只影响软件仿真时序，真机频率由 11.0592MHz 晶振决定。

## 4. 编译选项位置

| 设置 | 当前值 | 在 uVision 里的位置 |
| --- | --- | --- |
| Memory Model | Small | `Options for Target → Target` 页 |
| Code Rom Size | Large: 64K | 同上 |
| 优化级别 | 8 | `Options for Target → C51` 页 |
| Include Paths | 见下 | `Options for Target → C51 → Include Paths` |
| 输出目录 | `.\..\build\` | `Options for Target → Output` 页 |
| 输出文件名 | `51car` | 同上 |
| 生成 HEX | 已勾选 | `Options for Target → Output → Create HEX File` |
| Listing | `.\..\build\` | `Options for Target → Listing` 页 |

Include Paths 已配置（相对 `firmware\project\`，用 `;` 分隔）：

```text
..\config;..\include;..\common;..\main;..\app;
..\drivers\motor;..\drivers\tracking;..\drivers\ultrasonic;
..\drivers\servo;..\drivers\remote;..\drivers\display;
..\drivers\keys;..\drivers\buzzer_led
```

## 5. 输出位置

- HEX：`firmware\build\51car.hex`
- 编译中间文件（`.obj`、`.lst`、`.m51`）：同在 `firmware\build\`
- 构建日志：`firmware\build\51car.build_log.htm`

烧录不在 uVision 里做：打开 STC-ISP，选 STC89C52RC、11.0592MHz、加载上面的 HEX，冷启动下载。uVision 的 Flash Download 未配置，也不需要配置。

## 6. 工程分组与加文件

| 分组 | 对应目录 |
| --- | --- |
| `main` | `firmware\main\` |
| `app` | `firmware\app\` |
| `drivers\motor` | `firmware\drivers\motor\` |
| `drivers\tracking` | `firmware\drivers\tracking\` |
| `drivers\ultrasonic` | `firmware\drivers\ultrasonic\` |
| `drivers\servo` | `firmware\drivers\servo\` |
| `drivers\remote` | `firmware\drivers\remote\` |
| `drivers\display` | `firmware\drivers\display\` |
| `drivers\keys` | `firmware\drivers\keys\` |
| `drivers\buzzer_led` | `firmware\drivers\buzzer_led\` |
| `common` | `firmware\common\` |

加新源文件：

1. 把 `.c` 放进对应目录（小写英文 + 下划线）。
2. 在 Project 窗口右键对应分组 → `Add Existing Files to Group ...` → 选中文件。
3. 头文件统一放 `firmware\include\`，不进工程分组，靠 Include Paths 引用。

除 `main\main.c` 外，当前没有任何业务代码，分组为空是正常状态。

## 7. .gitignore 已排除

根目录 `.gitignore` 中与 Keil 相关的规则：

| 规则 | 作用 |
| --- | --- |
| `51-smart-car/firmware/build/` | 整个编译输出目录 |
| `*.hex` | 生成的 HEX |
| `*.lst` | 汇编列表 |
| `*.m51` | 链接映射 |
| `*.omf` | 目标文件 |
| `*.uvguix.*` | uVision5 的用户界面布局文件 |
| `*.uvoptx.bak`、`*.bak` | 备份文件 |
| `51-smart-car/firmware/project/*.uvopt` | uVision4 用户级配置 |
| `51-smart-car/firmware/project/*.uvoptx` | uVision5 用户级配置 |

需要提交的只有 `51car.uvproj`（以及将来转换出的 `.uvprojx`）。

## 8. 本机已知问题：C51 工具链路径

当前这台机器的 Keil 装在 `D:\Keil_v5`，但 `D:\Keil_v5\TOOLS.INI` 中写的是：

```ini
[C51]
PATH="C:\Keil_v5\C51\"
```

因此 uVision 编译时会报：

```text
--- Error: failed to execute 'C:\Keil_v5\C51\BIN\C51.EXE'
```

处理办法（任选其一）：

1. 把 `D:\Keil_v5\TOOLS.INI` 的 `[C51] PATH` 改为 `PATH="D:\Keil_v5\C51\"`；
2. 或者修复/重装 Keil，让安装路径与 TOOLS.INI 一致。

这是本机 Keil 安装配置问题，与工程文件无关。已用 `D:\Keil_v5\C51\BIN\C51.EXE` 直接编译 `main.c`（包含全部 `config` 与 `include` 头文件），结果为 **0 warning / 0 error**。

## 9. 下一步

1. 修好上面的 C51 路径后按 F7，应生成 `firmware\build\51car.hex`。
2. 按 `docs/design/photoelectric_plan.md` 的开发顺序，从 `firmware/common/` 的 1ms 节拍开始写第一个模块。
3. 每新增一个模块，同步在对应分组挂 `.c`，并更新 `tests/module/` 的验证记录。
