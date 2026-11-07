# hardware — 硬件资料

存放小车的全部硬件设计资料，与固件源码分开管理。

- `schematic/`：系统原理图（PDF、图片或 EDA 源文件），按版本命名。
- `pcb/`：PCB 布局走线、丝印、Gerber、贴片坐标等生产文件。
- `datasheets/`：主控、电机驱动、传感器、显示屏等器件手册。
- `pinmap/`：引脚分配表与接线表，是固件 `config/` 中引脚宏的唯一依据。

约定：`pinmap/` 中的引脚表一旦变更，必须同步修改 `firmware/config/`，并在 `docs/notes/` 留变更记录。
