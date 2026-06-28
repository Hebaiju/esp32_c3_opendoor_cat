# ============================================================
#  用户可配置区 - 以下参数可根据实际情况自行修改
# ============================================================

# PlatformIO 可执行文件路径
PLATFORMIO_EXE = "C:/Users/HEBAIJU/.platformio/penv/Scripts/platformio.exe"

# 串口端口（留空则自动检测，例如 "COM15" 或 "/dev/ttyUSB0"）
UPLOAD_PORT = "COM15"
MONITOR_PORT = "COM15"

# 上传波特率
UPLOAD_SPEED = 921600

# 串口监视器波特率
MONITOR_SPEED = 115200

# 编译环境（对应 platformio.ini 中的 [env:xxx]）
PIO_ENV = "esp32c3"

# ============================================================
#  以下为脚本内部逻辑，一般无需修改
# ============================================================

"""
ESP32-C3 PlatformIO 智能编译和上传工具
功能：
1. 自动编译、上传、监控
2. 构建和上传SPIFFS文件系统
3. 实时捕获编译输出
4. 智能识别错误和警告
5. 提供友好的问题报告
6. 统计编译信息（大小、时间等）
"""

import os
import sys
import subprocess
import re
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Tuple

# 项目根目录
PROJECT_DIR = Path(__file__).parent

# 颜色代码（Windows需要colorama，这里用简单版本）
class Color:
    RED = "\033[91m" if os.name != "nt" else ""
    GREEN = "\033[92m" if os.name != "nt" else ""
    YELLOW = "\033[93m" if os.name != "nt" else ""
    BLUE = "\033[94m" if os.name != "nt" else ""
    RESET = "\033[0m" if os.name != "nt" else ""


class BuildMonitor:
    """编译监控器：捕获和分析编译输出"""

    def __init__(self):
        self.errors = []
        self.warnings = []
        self.info_lines = []
        self.build_time = 0
        self.flash_size = 0
        self.ram_size = 0

    def parse_line(self, line: str):
        """解析每一行输出"""
        # 保存到完整日志
        self.info_lines.append(line)

        # 检测错误
        if "error:" in line.lower() or "fatal error:" in line.lower():
            self.errors.append(line.strip())
            print(f"{Color.RED}❌ {line.strip()}{Color.RESET}")
        # 检测警告
        elif "warning:" in line.lower():
            self.warnings.append(line.strip())
            print(f"{Color.YELLOW}⚠️  {line.strip()}{Color.RESET}")
        # 检测成功信息
        elif "success" in line.lower():
            print(f"{Color.GREEN}✅ {line.strip()}{Color.RESET}")
        # 检测Flash/RAM使用
        elif "RAM:" in line or "Flash:" in line:
            print(f"{Color.BLUE}📊 {line.strip()}{Color.RESET}")
            self._parse_memory_usage(line)
        # 其他信息
        else:
            print(line.rstrip())

    def _parse_memory_usage(self, line: str):
        """解析内存使用情况"""
        # 示例: "RAM:   [          ]   3.4% (used 13884 bytes from 409600 bytes)"
        ram_match = re.search(r"RAM:\s+.*?(\d+\.\d+)%.*?(\d+)\s+bytes", line)
        flash_match = re.search(r"Flash:\s+.*?(\d+\.\d+)%.*?(\d+)\s+bytes", line)

        if ram_match:
            self.ram_size = int(ram_match.group(2))
        if flash_match:
            self.flash_size = int(flash_match.group(2))

    def report(self):
        """生成编译报告"""
        print(f"\n{'='*60}")
        print("📋 编译报告")
        print(f"{'='*60}")

        print(f"\n⏱️  编译时间: {self.build_time:.2f} 秒")
        print(f"💾 Flash使用: {self.flash_size:,} bytes")
        print(f"🧠 RAM使用: {self.ram_size:,} bytes")

        if self.warnings:
            print(f"\n⚠️  警告 ({len(self.warnings)} 个):")
            for i, warning in enumerate(self.warnings[:5], 1):  # 只显示前5个
                print(f"  {i}. {warning}")
            if len(self.warnings) > 5:
                print(f"  ... 还有 {len(self.warnings) - 5} 个警告")

        if self.errors:
            print(f"\n❌ 错误 ({len(self.errors)} 个):")
            for i, error in enumerate(self.errors, 1):
                print(f"  {i}. {error}")
            return False
        else:
            print(f"\n✅ 编译成功，无错误！")
            return True

    def save_log(self, filename: str = "build.log"):
        """保存完整编译日志"""
        log_path = PROJECT_DIR / filename
        with open(log_path, "w", encoding="utf-8") as f:
            f.write("\n".join(self.info_lines))
        print(f"\n📄 完整日志已保存到: {log_path}")


def run_command_with_monitor(cmd: List[str], description: str) -> Tuple[bool, BuildMonitor]:
    """运行命令并监控输出"""
    print(f"\n{'='*60}")
    print(f"📋 {description}")
    print(f"{'='*60}\n")

    monitor = BuildMonitor()
    start_time = datetime.now()

    try:
        # 使用subprocess.Popen实时捕获输出
        process = subprocess.Popen(
            cmd,
            cwd=PROJECT_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            encoding="utf-8",
            errors="ignore"
        )

        # 实时读取输出
        for line in process.stdout:
            monitor.parse_line(line)

        # 等待进程结束
        process.wait()
        end_time = datetime.now()
        monitor.build_time = (end_time - start_time).total_seconds()

        # 生成报告
        success = monitor.report()
        monitor.save_log()

        return success, monitor

    except FileNotFoundError:
        print(f"{Color.RED}❌ 错误: 找不到PlatformIO可执行文件{Color.RESET}")
        print(f"   路径: {PLATFORMIO_EXE}")
        return False, monitor
    except Exception as e:
        print(f"{Color.RED}❌ 意外错误: {e}{Color.RESET}")
        return False, monitor


def _base_run_args():
    """构建 pio run 的基础参数列表"""
    args = [PLATFORMIO_EXE, "run", "-e", PIO_ENV]
    if UPLOAD_PORT:
        args += ["--upload-port", UPLOAD_PORT]
    return args


def clean():
    """清理构建文件"""
    success, _ = run_command_with_monitor(
        _base_run_args() + ["--target", "clean"],
        "清理项目"
    )
    return success


def build():
    """编译项目"""
    success, _ = run_command_with_monitor(
        _base_run_args(),
        "编译项目"
    )
    return success


def upload():
    """编译并上传固件"""
    success, monitor = run_command_with_monitor(
        _base_run_args() + ["--target", "upload"],
        "上传固件"
    )
    return success


def buildfs():
    """构建SPIFFS文件系统镜像"""
    success, _ = run_command_with_monitor(
        _base_run_args() + ["--target", "buildfs"],
        "构建文件系统"
    )
    return success


def uploadfs():
    """上传SPIFFS文件系统镜像"""
    success, _ = run_command_with_monitor(
        _base_run_args() + ["--target", "uploadfs"],
        "上传文件系统"
    )
    return success


def monitor():
    """打开串口监视器"""
    print(f"\n{'='*60}")
    print("📺 打开串口监视器 (Ctrl+C 退出)")
    print(f"{'='*60}\n")

    cmd = [PLATFORMIO_EXE, "device", "monitor", "-b", str(MONITOR_SPEED)]
    if MONITOR_PORT:
        cmd += ["-p", MONITOR_PORT]

    try:
        subprocess.run(cmd, cwd=PROJECT_DIR)
    except KeyboardInterrupt:
        print("\n👋 已退出串口监视器")
    except Exception as e:
        print(f"{Color.RED}❌ 错误: {e}{Color.RESET}")

    return True


def main():
    """主函数"""
    print(f"\n{'='*60}")
    print("🚀 ESP32-C3 PlatformIO 智能编译工具")
    print(f"{'='*60}\n")

    # 检查PlatformIO是否存在
    if not Path(PLATFORMIO_EXE).exists():
        print(f"{Color.RED}❌ 错误: 找不到PlatformIO: {PLATFORMIO_EXE}{Color.RESET}")
        print("请检查PlatformIO是否已安装")
        sys.exit(1)

    # 解析命令行参数
    if len(sys.argv) > 1:
        action = sys.argv[1].lower()
    else:
        action = "upload"  # 默认动作

    # 执行动作
    if action == "clean":
        success = clean()
    elif action == "build":
        success = build()
    elif action == "upload":
        success = upload()
    elif action == "buildfs":
        success = buildfs()
    elif action == "uploadfs":
        success = uploadfs()
    elif action == "fs":
        # 构建并上传文件系统
        success = buildfs() and uploadfs()
    elif action == "monitor":
        success = monitor()
    elif action == "all":
        # 清理 -> 编译 -> 上传固件 -> 构建并上传文件系统
        success = clean() and build() and upload() and buildfs() and uploadfs()
    elif action == "default":
        # 默认：编译+上传固件+上传文件系统
        success = build() and upload() and buildfs() and uploadfs()
    else:
        print(f"{Color.RED}❌ 未知动作: {action}{Color.RESET}")
        print(f"\n可用动作:")
        print(f"  {Color.GREEN}clean{Color.RESET}   - 清理项目")
        print(f"  {Color.GREEN}build{Color.RESET}   - 编译项目")
        print(f"  {Color.GREEN}upload{Color.RESET}  - 编译并上传固件")
        print(f"  {Color.GREEN}buildfs{Color.RESET}  - 构建文件系统镜像")
        print(f"  {Color.GREEN}uploadfs{Color.RESET} - 上传文件系统镜像")
        print(f"  {Color.GREEN}fs{Color.RESET}      - 构建并上传文件系统")
        print(f"  {Color.GREEN}monitor{Color.RESET} - 打开串口监视器")
        print(f"  {Color.GREEN}all{Color.RESET}     - 清理+编译+上传固件+上传文件系统")
        sys.exit(1)

    # 退出
    if success:
        print(f"\n{Color.GREEN}🎉 所有操作已完成！{Color.RESET}")
        sys.exit(0)
    else:
        print(f"\n{Color.RED}⚠️  操作失败，请检查错误信息{Color.RESET}")
        sys.exit(1)


if __name__ == "__main__":
    main()
