#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SVG注册工具打包工具
用于将SVG注册工具打包成可执行文件
"""

import os
import sys
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
from pathlib import Path
import threading
import subprocess
import shutil
import tempfile
from PIL import Image
import io

class PackageTool:
    def __init__(self, root):
        self.root = root
        self.root.title("SVG注册工具打包器")
        self.root.geometry("800x700")
        self.root.resizable(True, True)
        
        # 设置窗口居中显示
        self.center_window(self.root)
        
        # 设置图标
        self.set_icon()
        
        # 变量
        self.source_path = Path(__file__).parent
        # 默认打包路径设置为UtilityExtend插件根目录（相对路径）
        self.target_path = Path(__file__).parent.parent.parent
        self.package_name = "SVG_Icon_Manager"
        self.icon_path = None
        self.is_packaging = False
        
        self.setup_ui()
        
    def set_icon(self):
        """设置窗口图标"""
        try:
            icon_path = self.source_path / "Resources" / "icon.ico"
            if icon_path.exists():
                self.root.iconbitmap(icon_path)
        except:
            pass
    
    def center_window(self, window):
        """将窗口居中显示"""
        # 更新窗口信息以确保获取正确的尺寸
        window.update_idletasks()
        
        # 获取屏幕尺寸
        screen_width = window.winfo_screenwidth()
        screen_height = window.winfo_screenheight()
        
        # 获取窗口尺寸
        window_width = window.winfo_width()
        window_height = window.winfo_height()
        
        # 计算居中位置
        x = (screen_width - window_width) // 2
        y = (screen_height - window_height) // 2
        
        # 设置窗口位置
        window.geometry(f"+{x}+{y}")
    
    def setup_ui(self):
        """设置用户界面"""
        # 主框架
        main_frame = ttk.Frame(self.root, padding="20")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 配置网格权重
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        
        # 标题
        title_label = ttk.Label(main_frame, text="SVG注册工具打包器", 
                               font=("Microsoft YaHei UI", 18, "bold"))
        title_label.grid(row=0, column=0, columnspan=2, pady=(0, 30))
        
        # 源路径选择
        ttk.Label(main_frame, text="源路径:", font=("Microsoft YaHei UI", 10)).grid(
            row=1, column=0, sticky=tk.W, pady=5)
        self.source_entry = ttk.Entry(main_frame, width=50)
        self.source_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=(10, 0), pady=5)
        self.source_entry.insert(0, str(self.source_path))
        ttk.Button(main_frame, text="浏览", command=self.browse_source).grid(
            row=1, column=2, padx=(10, 0), pady=5)
        
        # 目标路径选择
        ttk.Label(main_frame, text="目标路径:", font=("Microsoft YaHei UI", 10)).grid(
            row=2, column=0, sticky=tk.W, pady=5)
        self.target_entry = ttk.Entry(main_frame, width=50)
        self.target_entry.grid(row=2, column=1, sticky=(tk.W, tk.E), padx=(10, 0), pady=5)
        self.target_entry.insert(0, str(self.target_path))
        ttk.Button(main_frame, text="浏览", command=self.browse_target).grid(
            row=2, column=2, padx=(10, 0), pady=5)
        
        # 打包名称
        ttk.Label(main_frame, text="打包名称:", font=("Microsoft YaHei UI", 10)).grid(
            row=3, column=0, sticky=tk.W, pady=5)
        self.name_entry = ttk.Entry(main_frame, width=50)
        self.name_entry.grid(row=3, column=1, sticky=(tk.W, tk.E), padx=(10, 0), pady=5)
        self.name_entry.insert(0, self.package_name)
        
        # 图标选择
        ttk.Label(main_frame, text="程序图标:", font=("Microsoft YaHei UI", 10)).grid(
            row=4, column=0, sticky=tk.W, pady=5)
        self.icon_entry = ttk.Entry(main_frame, width=50)
        self.icon_entry.grid(row=4, column=1, sticky=(tk.W, tk.E), padx=(10, 0), pady=5)
        self.icon_entry.insert(0, "使用默认图标")
        ttk.Button(main_frame, text="选择图片", command=self.browse_icon).grid(
            row=4, column=2, padx=(10, 0), pady=5)
        ttk.Button(main_frame, text="清除", command=self.clear_icon).grid(
            row=4, column=3, padx=(5, 0), pady=5)
        
        # 图标预览
        self.icon_preview = ttk.Label(main_frame, text="")
        self.icon_preview.grid(row=5, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        # 分隔线
        separator = ttk.Separator(main_frame, orient='horizontal')
        separator.grid(row=6, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=20)
        
        # 进度条
        ttk.Label(main_frame, text="打包进度:", font=("Microsoft YaHei UI", 10)).grid(
            row=7, column=0, sticky=tk.W, pady=5)
        self.progress = ttk.Progressbar(main_frame, mode='determinate', length=400)
        self.progress.grid(row=7, column=1, sticky=(tk.W, tk.E), padx=(10, 0), pady=5)
        
        # 打包按钮
        self.package_button = ttk.Button(main_frame, text="开始打包", 
                                        command=self.start_packaging, style='Accent.TButton')
        self.package_button.grid(row=8, column=0, columnspan=2, pady=20)
        
        # 日志视口
        ttk.Label(main_frame, text="打包日志:", font=("Microsoft YaHei UI", 10)).grid(
            row=9, column=0, sticky=tk.W, pady=(20, 5))
        
        # 日志文本框
        log_frame = ttk.Frame(main_frame)
        log_frame.grid(row=10, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=15, width=80)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 日志控制按钮
        log_buttons_frame = ttk.Frame(main_frame)
        log_buttons_frame.grid(row=11, column=0, columnspan=2, pady=10)
        
        ttk.Button(log_buttons_frame, text="清空日志", command=self.clear_log).pack(side=tk.LEFT, padx=5)
        ttk.Button(log_buttons_frame, text="打开目标文件夹", command=self.open_target_folder).pack(side=tk.LEFT, padx=5)
        
        # 配置网格权重
        main_frame.rowconfigure(10, weight=1)
        
        # 初始日志
        self.log("打包工具已启动")
        self.log(f"源路径: {self.source_path}")
        self.log(f"目标路径: {self.target_path}")
        
    def browse_source(self):
        """浏览源路径"""
        path = filedialog.askdirectory(initialdir=self.source_path)
        if path:
            self.source_path = Path(path)
            self.source_entry.delete(0, tk.END)
            self.source_entry.insert(0, str(self.source_path))
            self.log(f"源路径已更改为: {self.source_path}")
    
    def browse_target(self):
        """浏览目标路径"""
        path = filedialog.askdirectory(initialdir=self.target_path)
        if path:
            self.target_path = Path(path)
            self.target_entry.delete(0, tk.END)
            self.target_entry.insert(0, str(self.target_path))
            self.log(f"目标路径已更改为: {self.target_path}")
    
    def browse_icon(self):
        """浏览图标文件"""
        file_types = [
            ("图片文件", "*.png *.jpg *.jpeg *.bmp *.gif *.tiff"),
            ("PNG文件", "*.png"),
            ("JPG文件", "*.jpg *.jpeg"),
            ("所有文件", "*.*")
        ]
        file_path = filedialog.askopenfilename(
            title="选择图标图片",
            filetypes=file_types,
            initialdir=self.source_path
        )
        if file_path:
            self.icon_path = Path(file_path)
            self.icon_entry.delete(0, tk.END)
            self.icon_entry.insert(0, str(self.icon_path))
            self.preview_icon()
            self.log(f"已选择图标: {self.icon_path}")
    
    def clear_icon(self):
        """清除图标选择"""
        self.icon_path = None
        self.icon_entry.delete(0, tk.END)
        self.icon_entry.insert(0, "使用默认图标")
        self.icon_preview.config(text="")
        self.log("已清除图标选择")
    
    def preview_icon(self):
        """预览图标"""
        if not self.icon_path or not self.icon_path.exists():
            return
        
        try:
            # 尝试显示图标预览
            self.icon_preview.config(text=f"图标: {self.icon_path.name}")
        except Exception as e:
            self.icon_preview.config(text=f"图标预览失败: {e}")
    
    def convert_to_ico(self, image_path, output_path, size=(256, 256)):
        """将图片转换为ICO格式"""
        try:
            self.log(f"开始转换图标: {image_path} -> {output_path}")
            
            with Image.open(image_path) as img:
                self.log(f"原始图片信息: 模式={img.mode}, 尺寸={img.size}")
                
                # 转换为RGBA模式
                if img.mode != 'RGBA':
                    self.log(f"转换图片模式从 {img.mode} 到 RGBA")
                    img = img.convert('RGBA')
                
                # 调整大小
                self.log(f"调整图片尺寸到 {size}")
                img = img.resize(size, Image.Resampling.LANCZOS)
                
                # 保存为ICO，包含多种尺寸
                ico_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
                self.log(f"保存ICO文件，包含尺寸: {ico_sizes}")
                img.save(output_path, format='ICO', sizes=ico_sizes)
                
                # 验证文件是否创建成功
                if output_path.exists():
                    file_size = output_path.stat().st_size
                    self.log(f"ICO文件创建成功: {output_path}, 大小: {file_size} 字节")
                    return True
                else:
                    self.log("ICO文件创建失败：文件不存在")
                    return False
                    
        except Exception as e:
            self.log(f"图标转换失败: {e}")
            import traceback
            self.log(f"详细错误: {traceback.format_exc()}")
            return False
    
    def start_packaging(self):
        """开始打包"""
        if self.is_packaging:
            return
        
        # 获取参数
        source = Path(self.source_entry.get())
        target = Path(self.target_entry.get())
        name = self.name_entry.get().strip()
        
        if not source.exists():
            messagebox.showerror("错误", "源路径不存在！")
            return
        
        if not target.exists():
            try:
                target.mkdir(parents=True)
            except Exception as e:
                messagebox.showerror("错误", f"无法创建目标路径: {e}")
                return
        
        if not name:
            messagebox.showerror("错误", "请输入打包名称！")
            return
        
        # 开始打包
        self.is_packaging = True
        self.package_button.config(state='disabled')
        self.progress['value'] = 0
        
        # 在新线程中执行打包
        thread = threading.Thread(target=self.package_worker, args=(source, target, name))
        thread.daemon = True
        thread.start()
    
    def package_worker(self, source, target, name):
        """打包工作线程"""
        try:
            self.log("开始打包...")
            self.progress['value'] = 10
            
            # 检查依赖
            self.log("检查依赖...")
            if not self.check_dependencies():
                self.log("依赖检查失败，尝试安装...")
                self.install_dependencies()
            
            self.progress['value'] = 20
            
            # 准备图标
            icon_file = None
            if self.icon_path and self.icon_path.exists():
                self.log("转换图标...")
                icon_file = self.prepare_icon()
                if icon_file:
                    self.log(f"图标已转换: {icon_file}")
                self.progress['value'] = 30
            
            # 准备spec文件
            self.log("准备打包配置...")
            spec_file = self.prepare_spec_file(source, name, icon_file)
            if not spec_file:
                self.log("Spec文件创建失败，打包终止")
                return
            self.progress['value'] = 40
            
            # 执行打包
            self.log("执行PyInstaller打包...")
            success = self.execute_pyinstaller(spec_file)
            
            if success:
                self.progress['value'] = 80
                
                # 检查打包结果
                self.log("检查打包结果...")
                self.check_package_result(target, name)
                
                # 清理临时文件
                self.cleanup_temp_files()
                
                self.progress['value'] = 100
                self.log("打包完成！")
                messagebox.showinfo("成功", f"打包完成！\n目标位置: {target / name}")
            else:
                self.log("打包失败！")
                messagebox.showerror("错误", "打包失败，请查看日志")
                
        except Exception as e:
            self.log(f"打包过程中发生错误: {e}")
            messagebox.showerror("错误", f"打包失败: {e}")
        finally:
            self.is_packaging = False
            self.package_button.config(state='normal')
    
    def check_dependencies(self):
        """检查依赖"""
        try:
            import PyInstaller
            return True
        except ImportError:
            return False
    
    def install_dependencies(self):
        """安装依赖"""
        try:
            self.log("安装PyInstaller...")
            # 使用subprocess.run来获取更好的错误信息
            result = subprocess.run(
                [sys.executable, "-m", "pip", "install", "pyinstaller"], 
                capture_output=True, 
                text=True,
                timeout=300  # 5分钟超时
            )
            
            if result.returncode == 0:
                self.log("PyInstaller安装成功")
                return True
            else:
                self.log(f"PyInstaller安装失败: {result.stderr}")
                return False
                
        except subprocess.TimeoutExpired:
            self.log("PyInstaller安装超时")
            return False
        except Exception as e:
            self.log(f"依赖安装失败: {e}")
            return False
    
    def prepare_icon(self):
        """准备图标文件"""
        try:
            # 在源目录下创建临时图标文件，避免路径问题
            temp_dir = Path(__file__).parent / "temp_icons"
            temp_dir.mkdir(exist_ok=True)
            
            # 转换图标
            icon_file = temp_dir / "package_icon.ico"
            if self.convert_to_ico(self.icon_path, icon_file):
                self.log(f"图标转换成功: {icon_file}")
                return icon_file
            else:
                self.log("图标转换失败")
                return None
        except Exception as e:
            self.log(f"图标准备失败: {e}")
            import traceback
            self.log(f"详细错误: {traceback.format_exc()}")
            return None
    
    def prepare_spec_file(self, source, name, icon_file):
        """准备PyInstaller spec文件"""
        try:
            # 检查必需文件是否存在，只包含存在的文件
            datas = []
            required_files = [
                ("Resources/icon.ico", "Resources/icon.ico"),
                ("Resources/icon.svg", "Resources/icon.svg"),
                ("README.md", "README.md"),
                ("使用说明.md", "使用说明.md")
            ]
            
            for file_name, target_name in required_files:
                file_path = source / file_name
                if file_path.exists():
                    datas.append((str(file_path), '.'))
                    self.log(f"包含文件: {file_name}")
                else:
                    self.log(f"跳过不存在的文件: {file_name}")
            
            # 如果没有找到任何文件，至少包含主程序
            if not datas:
                datas.append((str(source), '.'))
                self.log("未找到其他文件，包含整个源目录")
            
            datas_str = ',\n        '.join([f"(r'{data[0]}', '{data[1]}')" for data in datas])
            
            # 处理图标参数
            icon_param = ""
            if icon_file and icon_file.exists():
                # 使用绝对路径，确保PyInstaller能找到图标文件
                icon_abs_path = icon_file.resolve()
                icon_param = f"icon=r'{icon_abs_path}',"
                self.log(f"设置程序图标: {icon_abs_path}")
            else:
                self.log("未设置程序图标，将使用默认图标")
            
            spec_content = f'''# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

a = Analysis(
    [r'{source / "SVG_Icon_Manager.py"}'],
    pathex=[r'{source}'],
    binaries=[],
    datas=[
        {datas_str}
    ],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={{}},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name=r'{name}',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    {icon_param}
    distpath=r'{source / "dist"}',
    workpath=r'{source / "build"}',
)
'''
            
            # 保存spec文件
            spec_file = source / f"{name}.spec"
            with open(spec_file, 'w', encoding='utf-8') as f:
                f.write(spec_content)
            
            self.log(f"Spec文件已创建: {spec_file}")
            return spec_file
            
        except Exception as e:
            self.log(f"Spec文件创建失败: {e}")
            return None
    
    def execute_pyinstaller(self, spec_file):
        """执行PyInstaller打包"""
        try:
            # 获取目标路径
            target = Path(self.target_entry.get())
            name = self.name_entry.get().strip()
            
            # 构建命令，指定输出路径
            cmd = [
                sys.executable, "-m", "PyInstaller", 
                str(spec_file), 
                "--clean",
                "--distpath", str(target),
                "--workpath", str(Path(spec_file).parent / "build")
            ]
            self.log(f"执行命令: {' '.join(cmd)}")
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1,
                cwd=str(spec_file.parent)  # 在spec文件所在目录执行
            )
            
            # 实时读取输出
            for line in process.stdout:
                if line.strip():  # 只记录非空行
                    self.log(line.strip())
                    self.root.update_idletasks()
            
            process.wait()
            
            if process.returncode == 0:
                self.log("PyInstaller执行成功")
                return True
            else:
                self.log(f"PyInstaller执行失败，返回码: {process.returncode}")
                return False
            
        except Exception as e:
            self.log(f"PyInstaller执行失败: {e}")
            return False
    
    def check_package_result(self, target, name):
        """检查打包结果"""
        try:
            target_dir = target / name
            self.log(f"检查目标目录: {target_dir}")
            
            if target_dir.exists():
                self.log(f"打包结果已成功创建在: {target_dir}")
                
                # 列出目录内容
                try:
                    contents = list(target_dir.iterdir())
                    self.log(f"目录包含 {len(contents)} 个项目:")
                    for item in contents:
                        self.log(f"  - {item.name}")
                except Exception as e:
                    self.log(f"无法列出目录内容: {e}")
            else:
                self.log(f"警告: 目标目录不存在: {target_dir}")
                
        except Exception as e:
            self.log(f"检查打包结果失败: {e}")
            import traceback
            self.log(f"详细错误信息: {traceback.format_exc()}")
    
    def cleanup_temp_files(self):
        """清理临时文件"""
        try:
            temp_dir = Path(__file__).parent / "temp_icons"
            if temp_dir.exists():
                import shutil
                shutil.rmtree(temp_dir)
                self.log("临时图标文件已清理")
        except Exception as e:
            self.log(f"清理临时文件失败: {e}")
    
    def log(self, message):
        """添加日志"""
        timestamp = self.get_timestamp()
        log_message = f"[{timestamp}] {message}\n"
        
        # 在主线程中更新UI
        self.root.after(0, self._update_log, log_message)
        
        # 自动保存日志到文件
        self.auto_save_log(message, timestamp)
    
    def auto_save_log(self, message, timestamp):
        """自动保存日志到文件"""
        try:
            # 创建Logs文件夹（如果不存在）
            logs_dir = Path(__file__).parent / "Logs"
            logs_dir.mkdir(exist_ok=True)
            
            # 生成日志文件名（使用当前日期）
            from datetime import datetime
            current_date = datetime.now().strftime("%Y-%m-%d")
            log_filename = f"打包日志_{current_date}.txt"
            log_file_path = logs_dir / log_filename
            
            # 写入日志内容
            with open(log_file_path, 'a', encoding='utf-8') as f:
                f.write(f"[{timestamp}] {message}\n")
                
        except Exception as e:
            # 如果自动保存失败，不显示错误，避免影响主程序运行
            pass
    
    def _update_log(self, message):
        """更新日志（在主线程中调用）"""
        self.log_text.insert(tk.END, message)
        self.log_text.see(tk.END)
        self.root.update_idletasks()
    
    def get_timestamp(self):
        """获取时间戳"""
        from datetime import datetime
        return datetime.now().strftime("%H:%M:%S")
    
    def clear_log(self):
        """清空日志"""
        self.log_text.delete(1.0, tk.END)
        self.log("日志已清空")
    
    def open_target_folder(self):
        """打开目标文件夹"""
        try:
            target = Path(self.target_entry.get())
            if target.exists():
                os.startfile(str(target))
            else:
                messagebox.showwarning("警告", "目标路径不存在")
        except Exception as e:
            self.log(f"打开目标文件夹失败: {e}")

def main():
    """主函数"""
    root = tk.Tk()
    app = PackageTool(root)
    root.mainloop()

if __name__ == "__main__":
    main()
