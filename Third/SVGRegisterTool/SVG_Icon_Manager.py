#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SVG图标注册管理工具
用于管理UtilityExtend插件中的SVG图标注册状态
"""

import os
import re
import sys
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from pathlib import Path
import json
import shutil
import subprocess
import threading
import time

class SVGIconManager:
    def __init__(self):
        """初始化SVG图标管理器"""
        self.root = tk.Tk()
        self.root.title("SVG图标注册管理器")
        self.root.geometry("1200x800")
        
        # 设置窗口居中显示
        self.center_window(self.root)
        
        # 配置文件路径
        self.config_file = Path(__file__).parent / "config.json"
        
        # 默认配置值
        self.project_name = "NEXBox"  # 项目名称 - 可配置
        self.plugin_name = "UtilityExtend"  # 插件名称 - 硬编码，不可配置
        self.project_path = None
        self.engine_path = None
        
        # 获取插件路径
        self.plugin_path = self._get_plugin_path()
        if not self.plugin_path:
            messagebox.showerror("错误", "无法找到UtilityExtend插件路径")
            self.root.quit()
            return
            
        # 设置源文件路径
        self.source_path = self.plugin_path / "Source" / "UtilityExtend"
        
        # 初始化其他属性
        self.svg_files = []  # 所有SVG图标文件
        self.missing_registrations = []
        self.orphaned_registrations = []
        self.friendly_names = {}
        self.registered_icons = []  # 添加缺失的属性
        self.button_configs = {'persistent': [], 'project': []}  # 按钮配置
        
        # 文件监控相关属性
        self.file_monitoring_enabled = False
        self.last_scan_time = 0
        self.file_check_interval = 2000  # 2秒检查一次
        self.monitoring_timer = None
        
        # 设置用户界面
        self.setup_ui()
        
        # 加载配置（移到setup_ui之后，因为add_log需要log_text）
        self.load_config()
        
        # 自动检测项目路径（如果没有设置）
        if not self.project_path:
            self.project_path = self._find_project_path()
        
        # 初始扫描
        self.refresh_scan()
        
        # 绑定键盘快捷键
        self.bind_shortcuts()
        
        # 创建工具提示
        self.tooltip = None
        self.tooltip_text = ""
        
        # 绑定窗口关闭事件
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
    
    def _get_plugin_path(self):
        """获取UtilityExtend插件根目录路径，支持打包后的相对路径"""
        # 如果是打包后的exe，使用当前工作目录
        if getattr(sys, 'frozen', False):
            # 打包后的exe，使用当前工作目录
            base_path = Path.cwd()
            # 尝试向上查找UtilityExtend插件根目录
            for parent in [base_path] + list(base_path.parents):
                # 检查是否是UtilityExtend插件根目录
                if (parent / "Resources").exists() and (parent / "Source" / "UtilityExtend").exists() and (parent / "Third" / "SVGRegisterTool").exists():
                    return parent
            # 如果找不到，使用当前目录
            return base_path
        else:
            # 开发环境，使用脚本相对路径
            # SVGRegisterTool位于 Third/SVGRegisterTool/，所以向上3级是UtilityExtend插件根目录
            return Path(__file__).parent.parent.parent
    
    def _find_project_path(self):
        """自动查找项目路径"""
        # 从插件路径开始向上查找，寻找包含.uproject文件的目录
        current_path = self.plugin_path
        for parent in [current_path] + list(current_path.parents):
            # 检查常见的项目文件
            if (parent / f"{self.project_name}.uproject").exists():
                self.add_log(f"找到项目路径: {parent}")
                return parent
            # 查找其他可能的项目文件
            uproject_files = list(parent.glob("*.uproject"))
            if uproject_files:
                self.add_log(f"找到项目路径: {parent}")
                return parent
            # 检查是否有.sln文件
            sln_files = list(parent.glob("*.sln"))
            if sln_files:
                self.add_log(f"找到项目路径: {parent}")
                return parent
        
        self.add_log("警告: 无法自动找到项目路径")
        return None
    
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
    
    def center_dialog(self, dialog):
        """将对话框相对于主窗口居中显示"""
        # 更新对话框信息以确保获取正确的尺寸
        dialog.update_idletasks()
        
        # 获取主窗口位置和尺寸
        root_x = self.root.winfo_rootx()
        root_y = self.root.winfo_rooty()
        root_width = self.root.winfo_width()
        root_height = self.root.winfo_height()
        
        # 获取对话框尺寸
        dialog_width = dialog.winfo_width()
        dialog_height = dialog.winfo_height()
        
        # 计算居中位置（相对于主窗口）
        x = root_x + (root_width - dialog_width) // 2
        y = root_y + (root_height - dialog_height) // 2
        
        # 确保对话框不会超出屏幕边界
        screen_width = dialog.winfo_screenwidth()
        screen_height = dialog.winfo_screenheight()
        
        if x < 0:
            x = 0
        if y < 0:
            y = 0
        if x + dialog_width > screen_width:
            x = screen_width - dialog_width
        if y + dialog_height > screen_height:
            y = screen_height - dialog_height
        
        # 设置对话框位置
        dialog.geometry(f"+{x}+{y}")
    
    def _find_engine_path(self):
        """自动查找引擎路径"""
        # 方法1：从项目路径向上查找
        if self.project_path:
            for parent in [self.project_path] + list(self.project_path.parents):
                if (parent / "Engine").exists() and (parent / "Engine" / "Binaries").exists():
                    self.add_log(f"找到引擎路径: {parent}")
                    return parent
        
        # 方法2：从插件路径向上查找
        for parent in [self.plugin_path] + list(self.plugin_path.parents):
            if (parent / "Engine").exists() and (parent / "Engine" / "Binaries").exists():
                self.add_log(f"找到引擎路径: {parent}")
                return parent
        
        # 方法3：检查常见的安装路径
        common_paths = [
            Path("C:/Program Files/Epic Games/UE_5.3"),
            Path("C:/Program Files/Epic Games/UE_5.2"),
            Path("C:/Program Files/Epic Games/UE_5.1"),
            Path("C:/Program Files/Epic Games/UE_5.0"),
            Path("C:/Program Files/Epic Games/UE_4.27"),
            Path("C:/Program Files/Epic Games/UE_4.26"),
            Path("C:/Program Files/Epic Games/UE_4.25"),
            Path("D:/Program Files/Epic Games/UE_5.3"),
            Path("D:/Program Files/Epic Games/UE_5.2"),
            Path("D:/Program Files/Epic Games/UE_5.1"),
            Path("D:/Program Files/Epic Games/UE_5.0"),
            Path("D:/Program Files/Epic Games/UE_4.27"),
            Path("D:/Program Files/Epic Games/UE_4.26"),
            Path("D:/Program Files/Epic Games/UE_4.25"),
        ]
        
        for path in common_paths:
            if path.exists() and (path / "Engine").exists() and (path / "Engine" / "Binaries").exists():
                self.add_log(f"找到引擎路径: {path}")
                return path
        
        self.add_log("警告: 无法自动找到引擎路径")
        return None
    
    def load_config(self):
        """加载配置"""
        try:
            if self.config_file.exists():
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                    
                # 加载项目名称
                if 'project_name' in config and config['project_name']:
                    self.project_name = config['project_name']
                    self.add_log(f"从配置加载项目名称: {self.project_name}")
                
                # 插件名称保持硬编码，不从配置加载
                # self.plugin_name = "UtilityExtend"  # 硬编码，不可配置
                
                # 加载项目路径
                if 'project_path' in config and config['project_path']:
                    project_path = Path(config['project_path'])
                    if project_path.exists():
                        self.project_path = project_path
                        self.add_log(f"从配置加载项目路径: {self.project_path}")
                
                # 加载引擎路径
                if 'engine_path' in config and config['engine_path']:
                    engine_path = Path(config['engine_path'])
                    if engine_path.exists():
                        self.engine_path = engine_path
                        self.add_log(f"从配置加载引擎路径: {self.engine_path}")
                        
                self.add_log("配置加载成功")
            else:
                self.add_log("配置文件不存在，使用默认配置")
        except Exception as e:
            self.add_log(f"加载配置时发生错误: {e}")
    
    def save_config(self):
        """保存配置"""
        try:
            # 确保配置目录存在
            self.config_file.parent.mkdir(parents=True, exist_ok=True)
            
            config = {
                'project_name': self.project_name,
                # 'plugin_name': self.plugin_name,  # 插件名称硬编码，不保存到配置
                'project_path': str(self.project_path) if self.project_path else "",
                'engine_path': str(self.engine_path) if self.engine_path else ""
            }
            
            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump(config, f, ensure_ascii=False, indent=2)
                
            self.add_log(f"配置已保存到: {self.config_file}")
        except Exception as e:
            self.add_log(f"保存配置时发生错误: {e}")
        
    def setup_ui(self):
        """设置用户界面"""
        # 配置根窗口网格权重
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(2, weight=1)  # Changed from 1 to 2 to make room for top button
        
        # 顶部菜单栏
        self.setup_menu_bar()
        
        # 顶部编译按钮区域
        top_button_frame = ttk.Frame(self.root)
        top_button_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), padx=5, pady=(5, 0))
        top_button_frame.columnconfigure(0, weight=0)  # 改为0，不扩展第一列
        
        # 编译按钮 - 放在顶部左侧
        compile_button = ttk.Button(top_button_frame, text="开始编译", command=self.recompile_project, style="Accent.TButton")
        compile_button.grid(row=0, column=0, pady=5, padx=(10, 0), sticky=tk.W)  # 添加sticky=tk.W使其靠左对齐
        
        # 打开项目SLN按钮 - 放在编译按钮右侧
        open_sln_button = ttk.Button(top_button_frame, text="手动编译", command=self.open_project_sln)
        open_sln_button.grid(row=0, column=1, pady=5, padx=(10, 0), sticky=tk.W)
        
        # 主框架 - 左右布局
        main_frame = ttk.Frame(self.root)
        main_frame.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)  # Changed from row=1 to row=2
        main_frame.columnconfigure(1, weight=1)  # 右侧面板可扩展
        main_frame.rowconfigure(0, weight=1)
        
        # 左侧：SVG文件列表（较小）
        left_frame = ttk.LabelFrame(main_frame, text="SVG文件列表", padding="5")
        left_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 5))
        left_frame.columnconfigure(0, weight=1)
        left_frame.rowconfigure(0, weight=1)
        
        # 设置左侧面板的初始宽度比例（较小）
        main_frame.columnconfigure(0, weight=1, minsize=300)  # 左侧最小宽度300px
        
        # SVG文件列表
        self.svg_tree = ttk.Treeview(left_frame, columns=("name", "path"), show="headings", height=15)
        self.svg_tree.heading("name", text="文件名")
        self.svg_tree.heading("path", text="路径")
        self.svg_tree.column("name", width=120, minwidth=100, stretch=True)
        self.svg_tree.column("path", width=150, minwidth=120, stretch=True)
        self.svg_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 左侧滚动条
        left_scrollbar = ttk.Scrollbar(left_frame, orient=tk.VERTICAL, command=self.svg_tree.yview)
        left_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.svg_tree.configure(yscrollcommand=left_scrollbar.set)
        
        # 左侧水平滚动条
        left_h_scrollbar = ttk.Scrollbar(left_frame, orient=tk.HORIZONTAL, command=self.svg_tree.xview)
        left_h_scrollbar.grid(row=1, column=0, sticky=(tk.W, tk.E))
        self.svg_tree.configure(xscrollcommand=left_h_scrollbar.set)
        
        # 右侧：注册状态和Log视口
        right_frame = ttk.Frame(main_frame)
        right_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        right_frame.columnconfigure(0, weight=1)
        right_frame.rowconfigure(0, weight=3)  # 状态区域占3份
        right_frame.rowconfigure(1, weight=2)  # Log区域占2份
        
        # 右侧上部：注册状态
        status_frame = ttk.LabelFrame(right_frame, text="注册状态", padding="5")
        status_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 5))
        status_frame.columnconfigure(0, weight=1)
        status_frame.rowconfigure(0, weight=1)
        
        # 状态列表
        self.status_tree = ttk.Treeview(status_frame, columns=("status", "details", "friendly_name", "actions"), show="headings", height=12)
        self.status_tree.heading("status", text="状态")
        self.status_tree.heading("details", text="详情")
        self.status_tree.heading("friendly_name", text="友好名")
        self.status_tree.heading("actions", text="操作")
        self.status_tree.column("status", width=80, minwidth=70, stretch=False)
        self.status_tree.column("details", width=120, minwidth=100, stretch=True)
        self.status_tree.column("friendly_name", width=120, minwidth=100, stretch=True)
        self.status_tree.column("actions", width=120, minwidth=100, stretch=False)  # 增加宽度以容纳按钮样式
        self.status_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 创建自定义样式用于按钮效果
        self.style = ttk.Style()
        self.style.configure("ActionButton.TButton", 
                           padding=(8, 4), 
                           relief="raised", 
                           borderwidth=2,
                           font=("TkDefaultFont", 9, "bold"))
        
        # 配置Treeview的样式，使操作列更突出
        self.style.configure("Treeview", 
                           background="white",
                           fieldbackground="white",
                           foreground="black")
        self.style.configure("Treeview.Heading", 
                           background="lightgray",
                           foreground="black",
                           font=("TkDefaultFont", 9, "bold"))
        
        # 状态滚动条
        status_scrollbar = ttk.Scrollbar(status_frame, orient=tk.VERTICAL, command=self.status_tree.yview)
        status_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.status_tree.configure(yscrollcommand=status_scrollbar.set)
        
        # 状态水平滚动条
        status_h_scrollbar = ttk.Scrollbar(status_frame, orient=tk.HORIZONTAL, command=self.status_tree.xview)
        status_h_scrollbar.grid(row=1, column=0, sticky=(tk.W, tk.E))
        self.status_tree.configure(xscrollcommand=status_h_scrollbar.set)
        
        # 右侧下部：Log视口
        log_frame = ttk.LabelFrame(right_frame, text="编译日志", padding="5")
        log_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        
        # Log文本框 - 增加高度以更好地显示编译日志
        self.log_text = tk.Text(log_frame, height=12, wrap=tk.WORD, state=tk.DISABLED, font=("Consolas", 9))
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Log滚动条
        log_scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.log_text.yview)
        log_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.log_text.configure(yscrollcommand=log_scrollbar.set)
        
        # Log水平滚动条
        log_h_scrollbar = ttk.Scrollbar(log_frame, orient=tk.HORIZONTAL, command=self.log_text.xview)
        log_h_scrollbar.grid(row=1, column=0, sticky=(tk.W, tk.E))
        self.log_text.configure(xscrollcommand=log_h_scrollbar.set)
        
        # 绑定事件
        self.svg_tree.bind('<<TreeviewSelect>>', self.on_svg_select)
        self.status_tree.bind('<<TreeviewSelect>>', self.on_status_select)
        self.status_tree.bind('<Button-1>', self.on_status_tree_click)
        
        # 绑定悬停事件显示详细信息（已移除on_tree_hover方法）
        
        # 绑定鼠标移动事件
        self.svg_tree.bind('<Motion>', self.on_tree_motion)
        self.status_tree.bind('<Motion>', self.on_tree_motion)
        
        # 绑定离开事件
        self.svg_tree.bind('<Leave>', self.on_tree_leave)
        self.status_tree.bind('<Leave>', self.on_tree_leave)
        
        # 绑定右键菜单
        self.svg_tree.bind('<Button-3>', self.show_context_menu)
        self.status_tree.bind('<Button-3>', self.show_context_menu)
        
        # 绑定窗口大小改变事件
        self.root.bind('<Configure>', self.on_window_resize)
        
        # 绑定键盘快捷键
        self.bind_shortcuts()
        
        # 创建状态栏
        self.setup_status_bar()
        
        # 创建工具提示
        self.tooltip = None
        self.tooltip_text = ""
        
    def setup_menu_bar(self):
        """设置顶部菜单栏"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # 文件菜单
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="重新扫描", command=self.refresh_scan)
        file_menu.add_command(label="重新注册图集", command=self.reregister_icons)
        file_menu.add_command(label="导入SVG到插件", command=lambda: self.import_svg('plugin'))
        file_menu.add_separator()
        file_menu.add_command(label="按钮配置管理", command=self.show_button_config_manager)
        file_menu.add_separator()
        file_menu.add_command(label="自动监控文件变化", command=self.toggle_file_monitoring)
        file_menu.add_separator()
        file_menu.add_command(label="查看备份", command=self.show_backups)
        file_menu.add_separator()
        file_menu.add_command(label="退出编辑器", command=self.root.quit)
        
        # 配置菜单
        config_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="配置", menu=config_menu)
        config_menu.add_command(label="工具设置", command=self.show_tool_settings)
        
        # 编译菜单
        compile_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="编译", menu=compile_menu)
        compile_menu.add_command(label="重新编译", command=self.recompile_project)
        compile_menu.add_command(label="打开项目sln进行手动编译", command=self.open_project_sln)
        
        # 高级菜单
        advanced_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="高级", menu=advanced_menu)
        advanced_menu.add_command(label="生成报告", command=self.generate_report)
        advanced_menu.add_command(label="打开代码文件", command=self.open_code_files)
        advanced_menu.add_separator()
        advanced_menu.add_command(label="查看当前已注册图标", command=self.show_current_registrations)
        advanced_menu.add_command(label="查看日志", command=self.view_compilation_logs)
        advanced_menu.add_command(label="清空日志", command=self.clear_log)
        
    def setup_status_bar(self):
        """设置状态栏"""
        # 状态栏框架 - 修正行号为3，因为主框架在第2行
        status_bar_frame = ttk.Frame(self.root)
        status_bar_frame.grid(row=3, column=0, sticky=(tk.W, tk.E), padx=5, pady=2)
        status_bar_frame.columnconfigure(1, weight=1)  # 进度条列可扩展
        
        # 左侧状态信息
        self.status_label = ttk.Label(status_bar_frame, text="就绪", relief=tk.SUNKEN, anchor=tk.W)
        self.status_label.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=(0, 5))
        
        # 进度条（默认隐藏，编译时显示）
        self.progress_bar = ttk.Progressbar(status_bar_frame, mode='indeterminate', length=200)
        self.progress_bar.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 5))
        self.progress_bar.grid_remove()  # 默认隐藏
        
        # 右侧文件计数信息
        self.count_label = ttk.Label(status_bar_frame, text="", relief=tk.SUNKEN, anchor=tk.E)
        self.count_label.grid(row=0, column=2, sticky=(tk.E), padx=(5, 0))
        
        # 更新状态栏显示
        self.update_status_bar()
        
    def update_status_bar(self):
        """更新状态栏信息"""
        if hasattr(self, 'svg_files') and hasattr(self, 'status_tree'):
            total_svg = len(self.svg_files)
            total_registered = len([item for item in self.status_tree.get_children() if "已注册" in self.status_tree.item(item)['values'][0]])
            total_missing = len(self.missing_registrations) if hasattr(self, 'svg_files') and hasattr(self, 'missing_registrations') else 0
            total_orphaned = len(self.orphaned_registrations) if hasattr(self, 'svg_files') and hasattr(self, 'orphaned_registrations') else 0
            
            self.count_label.config(text=f"SVG: {total_svg} | 已注册: {total_registered} | 未注册: {total_missing} | 孤立: {total_orphaned}")
    
    def set_status(self, message):
        """设置状态栏状态信息"""
        if hasattr(self, 'status_label'):
            self.status_label.config(text=message)
    
    def show_progress(self):
        """显示进度条"""
        if hasattr(self, 'progress_bar'):
            self.progress_bar.grid()
            self.progress_bar.start()
    
    def hide_progress(self):
        """隐藏进度条"""
        if hasattr(self, 'progress_bar'):
            self.progress_bar.stop()
            self.progress_bar.grid_remove()
    
    def show_context_menu(self, event):
        """显示右键上下文菜单"""
        tree = event.widget
        item = tree.identify_row(event.y)
        
        if item:
            # 选择项目
            tree.selection_set(item)
            
            # 创建上下文菜单
            context_menu = tk.Menu(self.root, tearoff=0)
            
            if tree == self.svg_tree:
                # SVG文件列表的上下文菜单
                context_menu.add_command(label="查看详情", command=lambda: self.on_svg_select(None))
                context_menu.add_separator()
                context_menu.add_command(label="复制文件名", command=lambda: self.copy_to_clipboard(tree.item(item)['values'][0]))
                context_menu.add_command(label="复制文件路径", command=lambda: self.copy_to_clipboard(tree.item(item)['values'][1]))
            elif tree == self.status_tree:
                # 状态列表的上下文菜单
                values = tree.item(item)['values']
                if "未注册" in values[0]:
                    context_menu.add_command(label="生成注册代码", command=lambda: self.show_registration_code(values[1]))
                elif "文件缺失" in values[0]:
                    context_menu.add_command(label="生成删除代码", command=lambda: self.show_removal_code(values[1]))
                elif "已注册" in values[0] or "文件缺失" in values[0]:
                    # 为已注册和文件缺失的图标添加修改友好名选项
                    context_menu.add_command(label="修改友好名", command=lambda: self.rename_friendly_name_for_item(values[1]))
                context_menu.add_separator()
                context_menu.add_command(label="复制状态信息", command=lambda: self.copy_to_clipboard(f"{values[0]} - {values[1]}"))
            
            # 显示菜单
            context_menu.tk_popup(event.x_root, event.y_root)
    
    def copy_to_clipboard(self, text):
        """复制文本到剪贴板"""
        self.root.clipboard_clear()
        self.root.clipboard_append(text)
        self.set_status(f"已复制到剪贴板: {text[:30]}...")
    
    def show_registration_code(self, svg_name):
        """显示注册代码"""
        code = self.generate_registration_code(svg_name)
        self.add_log(f"SVG文件 {svg_name} 的注册代码:")
        self.add_log(code)
    
    def show_removal_code(self, icon_name):
        """显示删除代码"""
        code = self.generate_removal_code(icon_name)
        self.add_log(f"图标 {icon_name} 的删除代码:")
        self.add_log(code)
        
    def scan_files(self):
        """扫描SVG文件"""
        self.set_status("正在扫描SVG文件...")
        self.svg_files.clear()
        self.svg_tree.delete(*self.svg_tree.get_children())
        
        # 扫描插件内置图标（Resources目录）
        # 注意：现在只扫描插件内置图标，不再支持项目配置图标
        self.scan_plugin_icons()
        
        self.add_log(f"扫描完成，发现 {len(self.svg_files)} 个SVG文件")
        
        # 更新最后扫描时间
        self.last_scan_time = time.time()
        
        # 更新状态栏
        self.update_status_bar()
        self.set_status("扫描完成")
    
    def scan_plugin_icons(self):
        """扫描插件内置图标（Resources目录）"""
        resources_path = self.plugin_path / "Resources"
        if not resources_path.exists():
            self.add_log(f"警告: Resources目录不存在: {resources_path}")
            return
            
        # 在Resources目录中查找SVG文件
        for svg_file in resources_path.glob("*.svg"):
            icon_info = {
                'name': svg_file.stem,
                'path': str(svg_file),
                'full_path': svg_file,
                'type': 'plugin'
            }
            self.svg_files.append(icon_info)
            self.svg_tree.insert("", "end", values=(f"[插件] {svg_file.stem}", str(svg_file)))
        
        self.add_log(f"扫描插件图标完成，发现 {len([f for f in self.svg_files if f['type'] == 'plugin'])} 个")
    
        # 读取按钮配置
        self.load_button_configs()
    
    def load_button_configs(self):
        """读取持久化配置和项目配置中的按钮配置"""
        self.button_configs = {
            'persistent': [],
            'project': []
        }
        
        # 读取持久化配置
        persistent_config_path = self.plugin_path / "Config" / "DefaultUtilityExtendPersistent.ini"
        if persistent_config_path.exists():
            self.parse_config_file(persistent_config_path, 'persistent')
        
        # 读取项目配置
        if self.project_path:
            project_config_path = self.project_path / "Config" / "DefaultUtilityExtend.ini"
            if project_config_path.exists():
                self.parse_config_file(project_config_path, 'project')
        
        total_configs = len(self.button_configs['persistent']) + len(self.button_configs['project'])
        self.add_log(f"读取按钮配置完成: 持久化配置 {len(self.button_configs['persistent'])} 个, 项目配置 {len(self.button_configs['project'])} 个")
        
    def parse_config_file(self, config_path, config_type):
        """解析配置文件"""
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            # 根据配置类型解析不同的配置键
            if config_type == 'persistent':
                config_key = '+PersistentButtonConfigs='
                section_name = '/Script/UtilityExtend.UtilityExtendPersistentSettings'
            else:
                config_key = '+ToolbarButtonConfigs='
                section_name = '/Script/UtilityExtend.UtilityExtendSettings'
            
            # 查找配置部分
            if section_name in content:
                lines = content.split('\n')
                for line in lines:
                    line = line.strip()
                    if line.startswith(config_key):
                        config_value = line[len(config_key):]
                        button_config = self.parse_button_config_string(config_value, config_type)
                        if button_config:
                            self.button_configs[config_type].append(button_config)
                            
        except Exception as e:
            self.add_log(f"解析配置文件 {config_path} 时出错: {e}")
            
    def parse_button_config_string(self, config_string, config_type):
        """解析按钮配置字符串"""
        try:
            # 移除外层括号
            config_string = config_string.strip()
            if config_string.startswith('(') and config_string.endswith(')'):
                config_string = config_string[1:-1]
            
            # 解析键值对
            config_dict = {}
            current_pos = 0
            
            while current_pos < len(config_string):
                # 查找下一个键
                eq_pos = config_string.find('=', current_pos)
                if eq_pos == -1:
                    break
                    
                key = config_string[current_pos:eq_pos].strip()
                
                # 查找值
                current_pos = eq_pos + 1
                if current_pos >= len(config_string):
                    break
                    
                # 处理值
                if config_string[current_pos] == '"':
                    # 字符串值
                    current_pos += 1
                    end_quote = config_string.find('"', current_pos)
                    if end_quote == -1:
                        break
                    value = config_string[current_pos:end_quote]
                    current_pos = end_quote + 1
                elif config_string[current_pos] == '(':
                    # 嵌套括号值（如DropdownItems）
                    bracket_count = 1
                    start_pos = current_pos
                    current_pos += 1
                    while current_pos < len(config_string) and bracket_count > 0:
                        if config_string[current_pos] == '(':
                            bracket_count += 1
                        elif config_string[current_pos] == ')':
                            bracket_count -= 1
                        current_pos += 1
                    value = config_string[start_pos:current_pos]
                else:
                    # 简单值
                    comma_pos = config_string.find(',', current_pos)
                    if comma_pos == -1:
                        value = config_string[current_pos:].strip()
                        current_pos = len(config_string)
                    else:
                        value = config_string[current_pos:comma_pos].strip()
                        current_pos = comma_pos
                
                config_dict[key] = value
                
                # 跳过逗号
                if current_pos < len(config_string) and config_string[current_pos] == ',':
                    current_pos += 1
            
            # 创建按钮配置对象
            if 'ButtonName' in config_dict:
                button_config = {
                    'name': config_dict.get('ButtonName', ''),
                    'type': config_dict.get('ButtonType', 'SingleButton'),
                    'icon_name': config_dict.get('ButtonIconName', ''),
                    'bound_class': config_dict.get('BoundClass', 'None'),
                    'dropdown_items': config_dict.get('DropdownItems', ''),
                    'config_type': config_type,
                    'original_string': config_string
                }
                return button_config
                
        except Exception as e:
            self.add_log(f"解析按钮配置字符串时出错: {e}")
            
        return None

    def show_button_config_manager(self):
        """显示按钮配置管理器"""
        self.add_log("打开按钮配置管理器...")
        
        # 创建按钮配置管理窗口
        config_window = tk.Toplevel(self.root)
        config_window.title("按钮配置管理器")
        config_window.geometry("1200x800")
        config_window.transient(self.root)
        
        # 居中显示对话框
        self.center_dialog(config_window)
        
        # 主框架
        main_frame = ttk.Frame(config_window, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 创建Notebook用于分页显示
        notebook = ttk.Notebook(main_frame)
        notebook.pack(fill=tk.BOTH, expand=True, pady=(0, 20))
        
        # 持久化配置页面
        persistent_frame = ttk.Frame(notebook)
        notebook.add(persistent_frame, text="持久化配置")
        self.create_config_page(persistent_frame, 'persistent')
        
        # 项目配置页面
        project_frame = ttk.Frame(notebook)
        notebook.add(project_frame, text="项目配置")
        self.create_config_page(project_frame, 'project')
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X)
        
        # 刷新按钮
        refresh_button = ttk.Button(button_frame, text="刷新配置", 
                                  command=lambda: self.refresh_button_configs(config_window))
        refresh_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 关闭按钮
        close_button = ttk.Button(button_frame, text="关闭", command=config_window.destroy)
        close_button.pack(side=tk.RIGHT)
        
    def create_config_page(self, parent_frame, config_type):
        """创建配置页面"""
        # 配置信息框架
        info_frame = ttk.LabelFrame(parent_frame, text=f"{config_type.title()}配置信息", padding="10")
        info_frame.pack(fill=tk.X, pady=(0, 10))
        
        config_count = len(self.button_configs.get(config_type, []))
        config_file = "DefaultUtilityExtendPersistent.ini" if config_type == 'persistent' else "DefaultUtilityExtend.ini"
        
        ttk.Label(info_frame, text=f"配置文件: {config_file}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"按钮数量: {config_count}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"配置类型: {'插件持久化配置' if config_type == 'persistent' else '项目配置'}").pack(anchor=tk.W)
        
        # 按钮列表框架
        list_frame = ttk.LabelFrame(parent_frame, text="按钮列表", padding="10")
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        # 创建按钮列表树形视图
        tree = ttk.Treeview(list_frame, columns=("name", "type", "icon", "class", "items"), show="headings", height=15)
        tree.heading("name", text="按钮名称")
        tree.heading("type", text="类型")
        tree.heading("icon", text="图标名称")
        tree.heading("class", text="绑定类")
        tree.heading("items", text="下拉项")
        
        tree.column("name", width=200, minwidth=150)
        tree.column("type", width=100, minwidth=80)
        tree.column("icon", width=150, minwidth=120)
        tree.column("class", width=200, minwidth=150)
        tree.column("items", width=250, minwidth=200)
        
        # 添加滚动条
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scrollbar.set)
        
        tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # 填充按钮配置数据
        self.populate_config_tree(tree, config_type)
        
        # 绑定双击事件编辑配置
        tree.bind("<Double-1>", lambda e: self.edit_button_config(tree, config_type))
        
        # 绑定右键菜单
        tree.bind("<Button-3>", lambda e: self.show_config_context_menu(e, tree, config_type))
        
        # 保存树形视图引用
        if config_type == 'persistent':
            self.persistent_tree = tree
        else:
            self.project_tree = tree
            
    def populate_config_tree(self, tree, config_type):
        """填充配置树形视图"""
        # 清空现有项目
        for item in tree.get_children():
            tree.delete(item)
            
        # 添加按钮配置
        configs = self.button_configs.get(config_type, [])
        for config in configs:
            # 处理下拉项显示
            dropdown_display = ""
            if config['type'] == 'DropdownButton' and config['dropdown_items']:
                # 简化显示，只显示项目数量
                try:
                    # 简单计算括号对数量来估算项目数
                    item_count = config['dropdown_items'].count('(ItemName=')
                    dropdown_display = f"{item_count} 个下拉项"
                except:
                    dropdown_display = "有下拉项"
            
            tree.insert("", tk.END, values=(
                config['name'],
                config['type'],
                config['icon_name'],  # 配置文件中已经保存友好名称，直接显示
                config['bound_class'] if config['bound_class'] != 'None' else '',
                dropdown_display
            ))
            
    def edit_button_config(self, tree, config_type):
        """编辑按钮配置"""
        selection = tree.selection()
        if not selection:
            return
            
        item = selection[0]
        values = tree.item(item)['values']
        button_name = values[0]
        
        # 查找对应的配置
        configs = self.button_configs.get(config_type, [])
        config_index = None
        for i, config in enumerate(configs):
            if config['name'] == button_name:
                config_index = i
                break
                
        if config_index is None:
            messagebox.showerror("错误", "未找到对应的按钮配置")
            return
            
        # 创建配置编辑对话框
        self.show_config_edit_dialog(config_type, config_index, tree)
    
    def edit_button_icon(self, tree, config_type):
        """编辑按钮图标（保留原有接口）"""
        self.edit_button_config(tree, config_type)
    
    def show_config_edit_dialog(self, config_type, config_index, tree):
        """显示完整的配置编辑对话框"""
        config = self.button_configs[config_type][config_index]
        
        # 创建对话框
        dialog = tk.Toplevel(self.root)
        dialog.title(f"编辑按钮配置 - {config['name']}")
        dialog.geometry("800x700")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # 居中显示对话框
        self.center_dialog(dialog)
        
        # 主框架
        main_frame = ttk.Frame(dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 配置信息框架
        info_frame = ttk.LabelFrame(main_frame, text="配置信息", padding="10")
        info_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(info_frame, text=f"配置类型: {'持久化配置' if config_type == 'persistent' else '项目配置'}").pack(anchor=tk.W)
        ttk.Label(info_frame, text="注意: 绑定脚本不可修改", font=("", 8), foreground="red").pack(anchor=tk.W)
        
        # 创建Notebook用于分页
        notebook = ttk.Notebook(main_frame)
        notebook.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # 基本设置页
        basic_frame = ttk.Frame(notebook, padding="10")
        notebook.add(basic_frame, text="基本设置")
        
        # 按钮名称
        name_frame = ttk.Frame(basic_frame)
        name_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(name_frame, text="按钮名称:").pack(side=tk.LEFT)
        name_var = tk.StringVar(value=config['name'])
        name_entry = ttk.Entry(name_frame, textvariable=name_var, width=40)
        name_entry.pack(side=tk.LEFT, padx=(10, 0), fill=tk.X, expand=True)
        
        # 按钮类型
        type_frame = ttk.Frame(basic_frame)
        type_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(type_frame, text="按钮类型:").pack(side=tk.LEFT)
        type_var = tk.StringVar(value=config['type'])
        type_combo = ttk.Combobox(type_frame, textvariable=type_var, 
                                 values=['SingleButton', 'DropdownButton'])
        type_combo.pack(side=tk.LEFT, padx=(10, 0))
        type_combo.state(['readonly'])
        
        # 图标选择
        icon_frame = ttk.Frame(basic_frame)
        icon_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(icon_frame, text="图标:").pack(side=tk.LEFT)
        # 配置文件中已经保存友好名称，直接使用
        icon_var = tk.StringVar(value=config['icon_name'])
        available_icons = self.get_available_icon_names()
        icon_combo = ttk.Combobox(icon_frame, textvariable=icon_var, values=available_icons, width=30)
        icon_combo.pack(side=tk.LEFT, padx=(10, 0))
        
        # 绑定脚本（只读）
        bound_frame = ttk.Frame(basic_frame)
        bound_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(bound_frame, text="绑定脚本:").pack(side=tk.LEFT)
        bound_var = tk.StringVar(value=config['bound_class'] if config['bound_class'] != 'None' else '')
        bound_entry = ttk.Entry(bound_frame, textvariable=bound_var, width=60, state='readonly')
        bound_entry.pack(side=tk.LEFT, padx=(10, 0), fill=tk.X, expand=True)
        
        # 下拉项设置页（仅对DropdownButton有效）
        dropdown_frame = ttk.Frame(notebook, padding="10")
        notebook.add(dropdown_frame, text="下拉项设置")
        
        # 下拉项说明
        ttk.Label(dropdown_frame, text="下拉项配置:", font=("", 10, "bold")).pack(anchor=tk.W, pady=(0, 5))
        ttk.Label(dropdown_frame, text="格式: ((ItemName=\"项目名\",BoundClass=\"脚本路径\"),...)", 
                 font=("", 8), foreground="gray").pack(anchor=tk.W, pady=(0, 10))
        
        # 下拉项文本框
        dropdown_text = tk.Text(dropdown_frame, height=15, wrap=tk.WORD)
        dropdown_text.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # 添加滚动条
        dropdown_scroll = ttk.Scrollbar(dropdown_frame, orient=tk.VERTICAL, command=dropdown_text.yview)
        dropdown_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        dropdown_text.configure(yscrollcommand=dropdown_scroll.set)
        
        # 填充下拉项内容
        dropdown_text.insert('1.0', config['dropdown_items'] if config['dropdown_items'] else '')
        
        # 启用/禁用下拉项编辑
        def on_type_change(*args):
            if type_var.get() == 'DropdownButton':
                dropdown_text.config(state='normal')
                notebook.tab(1, state='normal')
            else:
                dropdown_text.config(state='disabled')
                notebook.tab(1, state='disabled')
        
        type_var.trace('w', on_type_change)
        on_type_change()  # 初始状态
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X)
        
        def save_config():
            """保存配置修改"""
            try:
                # 验证输入
                new_name = name_var.get().strip()
                if not new_name:
                    messagebox.showerror("错误", "按钮名称不能为空")
                    return
                
                new_type = type_var.get()
                new_icon_friendly = icon_var.get().strip()
                if not new_icon_friendly:
                    messagebox.showerror("错误", "必须选择图标")
                    return
                
                # 获取下拉项内容
                new_dropdown = ""
                if new_type == 'DropdownButton':
                    new_dropdown = dropdown_text.get('1.0', tk.END).strip()
                
                # 更新配置
                updated_config = {
                    'name': new_name,
                    'type': new_type,
                    'icon_name': new_icon_friendly,  # 直接保存友好名称到配置文件
                    'bound_class': config['bound_class'],  # 绑定脚本不可修改
                    'dropdown_items': new_dropdown
                }
                
                # 保存到文件
                if self.update_button_config(config_type, config_index, updated_config):
                    messagebox.showinfo("成功", "配置已保存")
                    dialog.destroy()
                    # 刷新树形视图
                    self.populate_config_tree(tree, config_type)
                else:
                    messagebox.showerror("错误", "保存配置失败")
                    
            except Exception as e:
                messagebox.showerror("错误", f"保存配置时发生错误: {str(e)}")
        
        def cancel_edit():
            """取消编辑"""
            dialog.destroy()
        
        # 按钮
        ttk.Button(button_frame, text="保存", command=save_config).pack(side=tk.RIGHT, padx=(10, 0))
        ttk.Button(button_frame, text="取消", command=cancel_edit).pack(side=tk.RIGHT)
        
    def show_icon_selection_dialog(self, config_type, config_index, current_icon, tree):
        """显示图标选择对话框"""
        config = self.button_configs[config_type][config_index]
        
        # 创建对话框
        dialog = tk.Toplevel(self.root)
        dialog.title(f"修改按钮图标 - {config['name']}")
        dialog.geometry("600x500")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # 居中显示对话框
        self.center_dialog(dialog)
        
        # 主框架
        main_frame = ttk.Frame(dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 按钮信息
        info_frame = ttk.LabelFrame(main_frame, text="按钮信息", padding="10")
        info_frame.pack(fill=tk.X, pady=(0, 20))
        
        ttk.Label(info_frame, text=f"按钮名称: {config['name']}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"按钮类型: {config['type']}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"配置类型: {'持久化配置' if config_type == 'persistent' else '项目配置'}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"当前图标: {current_icon}").pack(anchor=tk.W)
        
        # 图标选择框架
        icon_frame = ttk.LabelFrame(main_frame, text="选择新图标", padding="10")
        icon_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 20))
        
        # 获取可用图标列表
        available_icons = self.get_available_icon_names()
        
        # 创建图标列表
        icon_listbox = tk.Listbox(icon_frame, selectmode=tk.SINGLE, height=15)
        icon_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # 添加滚动条
        icon_scrollbar = ttk.Scrollbar(icon_frame, orient=tk.VERTICAL, command=icon_listbox.yview)
        icon_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        icon_listbox.configure(yscrollcommand=icon_scrollbar.set)
        
        # 填充图标列表
        for icon_name in available_icons:
            icon_listbox.insert(tk.END, icon_name)
            
        # 选中当前图标
        try:
            current_index = available_icons.index(current_icon)
            icon_listbox.selection_set(current_index)
            icon_listbox.see(current_index)
        except ValueError:
            pass
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X)
        
        def apply_icon_change():
            selection = icon_listbox.curselection()
            if not selection:
                messagebox.showwarning("警告", "请选择一个图标")
                return
                
            new_icon = available_icons[selection[0]]
            
            if new_icon == current_icon:
                messagebox.showinfo("信息", "图标没有变化")
                dialog.destroy()
                return
                
            # 更新配置
            self.update_button_icon(config_type, config_index, new_icon)
            
            # 刷新树形视图
            self.populate_config_tree(tree, config_type)
            
            messagebox.showinfo("完成", f"图标已更新为: {new_icon}")
            dialog.destroy()
            
        def cancel_change():
            dialog.destroy()
            
        # 应用按钮
        apply_button = ttk.Button(button_frame, text="应用更改", command=apply_icon_change)
        apply_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 取消按钮
        cancel_button = ttk.Button(button_frame, text="取消", command=cancel_change)
        cancel_button.pack(side=tk.LEFT)
        
        # 预览按钮（可选）
        preview_button = ttk.Button(button_frame, text="预览图标", 
                                  command=lambda: self.preview_icon(icon_listbox, available_icons))
        preview_button.pack(side=tk.RIGHT)
        
    def get_available_icon_names(self):
        """获取已注册的图标友好名称列表（与UE编辑器中显示一致）"""
        friendly_names = set()
        
        # 从图标注册表文件获取友好名称
        if self.source_path:
            registry_file = self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
            if registry_file.exists():
                try:
                    content = registry_file.read_text(encoding='utf-8')
                    # 查找 FToolbarIconInfo 的模式来获取友好名称
                    # 格式: FToolbarIconInfo(TEXT("技术名称"), TEXT("友好名称"), TEXT("描述"))
                    pattern = r'FToolbarIconInfo\(\s*TEXT\(\s*"[^"]+"\s*\)\s*,\s*TEXT\(\s*"([^"]+)"\s*\)\s*,\s*TEXT\(\s*"[^"]*"\s*\)\s*\)'
                    matches = re.findall(pattern, content)
                    friendly_names.update(matches)
                    self.add_log(f"从注册表文件中找到 {len(matches)} 个图标友好名称")
                        
                except Exception as e:
                    self.add_log(f"读取图标注册表文件失败: {e}")
        
        # 如果没有找到任何图标，添加默认的友好名称
        if not friendly_names:
            friendly_names = {"默认图标", "工具箱", "NEXBox图标"}
            self.add_log("未找到注册的图标，使用默认友好名称")
        
        result = sorted(list(friendly_names))
        self.add_log(f"可用图标友好名称: {result}")
        return result
    

    def preview_icon(self, listbox, available_icons):
        """预览选中的图标"""
        selection = listbox.curselection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个图标")
            return
            
        icon_name = available_icons[selection[0]]
        
        # 查找对应的SVG文件
        svg_path = None
        for svg_info in self.svg_files:
            if svg_info['name'] == icon_name:
                svg_path = svg_info['path']
                break
                
        if svg_path and os.path.exists(svg_path):
            try:
                # 使用系统默认程序打开SVG文件
                os.startfile(svg_path)
                self.add_log(f"预览图标: {icon_name}")
            except Exception as e:
                messagebox.showerror("错误", f"无法预览图标: {str(e)}")
        else:
            messagebox.showinfo("信息", f"图标 '{icon_name}' 是内置图标，无法预览文件")
            
    def update_button_config(self, config_type, config_index, updated_config):
        """更新完整的按钮配置"""
        try:
            # 更新内存中的配置
            old_config = self.button_configs[config_type][config_index].copy()
            self.button_configs[config_type][config_index].update(updated_config)
            
            # 备份现有配置文件
            self.backup_config_files("修改按钮配置")
            
            # 保存到配置文件
            self.save_button_config_to_file(config_type, config_index)
            
            # 记录修改的项目
            changes = []
            for key, new_value in updated_config.items():
                old_value = old_config.get(key, '')
                if str(old_value) != str(new_value):
                    changes.append(f"{key}: '{old_value}' -> '{new_value}'")
            
            if changes:
                self.add_log(f"已更新{config_type}配置: {', '.join(changes)}")
            
            return True
            
        except Exception as e:
            error_msg = f"更新按钮配置时发生错误: {str(e)}"
            self.add_log(f"错误: {error_msg}")
            return False
    
    def update_button_icon(self, config_type, config_index, new_icon):
        """更新按钮图标配置（保留兼容性）"""
        return self.update_button_config(config_type, config_index, {'icon_name': new_icon})
            
    def save_button_config_to_file(self, config_type, config_index):
        """保存按钮配置到文件"""
        # 获取配置文件路径
        if config_type == 'persistent':
            config_path = self.plugin_path / "Config" / "DefaultUtilityExtendPersistent.ini"
        else:
            if not self.project_path:
                raise Exception("项目路径未设置")
            config_path = self.project_path / "Config" / "DefaultUtilityExtend.ini"
            
        # 重新生成整个配置文件
        self.regenerate_config_file(config_path, config_type)
        
    def regenerate_config_file(self, config_path, config_type):
        """重新生成配置文件"""
        try:
            # 读取现有文件内容
            if config_path.exists():
                with open(config_path, 'r', encoding='utf-8') as f:
                    content = f.read()
            else:
                content = ""
                
            # 确定配置节和键
            if config_type == 'persistent':
                section_name = "[/Script/UtilityExtend.UtilityExtendPersistentSettings]"
                config_key = "+PersistentButtonConfigs="
            else:
                section_name = "[/Script/UtilityExtend.UtilityExtendSettings]"
                config_key = "+ToolbarButtonConfigs="
                
            # 分析现有内容
            lines = content.split('\n')
            new_lines = []
            in_target_section = False
            
            # 保留非目标配置的内容
            for line in lines:
                line_stripped = line.strip()
                if line_stripped.startswith('['):
                    in_target_section = (line_stripped == section_name)
                    new_lines.append(line)
                elif in_target_section and line_stripped.startswith(config_key):
                    # 跳过现有的按钮配置行
                    continue
                else:
                    new_lines.append(line)
                    
            # 如果没有找到目标节，添加它
            if section_name not in content:
                if new_lines and new_lines[-1].strip():
                    new_lines.append("")
                new_lines.append(section_name)
                
            # 添加更新后的按钮配置
            configs = self.button_configs.get(config_type, [])
            for config in configs:
                config_line = self.generate_config_line(config, config_key)
                new_lines.append(config_line)
                
            # 确保文件以空行结尾
            if new_lines and new_lines[-1].strip():
                new_lines.append("")
                
            # 写回文件
            with open(config_path, 'w', encoding='utf-8') as f:
                f.write('\n'.join(new_lines))
                
            self.add_log(f"配置文件已更新: {config_path}")
            
        except Exception as e:
            raise Exception(f"重新生成配置文件失败: {str(e)}")
            
    def generate_config_line(self, config, config_key):
        """生成配置行字符串"""
        # 重新构建配置字符串
        config_parts = [
            f'ButtonName="{config["name"]}"',
            f'ButtonType={config["type"]}',
            f'BoundClass="{config["bound_class"]}"',
            f'ButtonIconName="{config["icon_name"]}"'
        ]
        
        # 添加下拉项（如果有）
        if config['type'] == 'DropdownButton' and config['dropdown_items']:
            config_parts.append(f'DropdownItems={config["dropdown_items"]}')
            
        config_string = f"({','.join(config_parts)})"
        return f"{config_key}{config_string}"
        
    def backup_config_files(self, description):
        """备份配置文件"""
        try:
            backup_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "ConfigBackups"
            backup_dir.mkdir(parents=True, exist_ok=True)
            
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            backup_name = f"config_backup_{timestamp}_{description.replace(' ', '_')}"
            
            files_to_backup = []
            
            # 备份持久化配置文件
            persistent_config = self.plugin_path / "Config" / "DefaultUtilityExtendPersistent.ini"
            if persistent_config.exists():
                backup_path = backup_dir / f"{backup_name}_persistent.ini"
                shutil.copy2(persistent_config, backup_path)
                files_to_backup.append(str(backup_path))
                
            # 备份项目配置文件
            if self.project_path:
                project_config = self.project_path / "Config" / "DefaultUtilityExtend.ini"
                if project_config.exists():
                    backup_path = backup_dir / f"{backup_name}_project.ini"
                    shutil.copy2(project_config, backup_path)
                    files_to_backup.append(str(backup_path))
                    
            self.add_log(f"配置文件已备份: {backup_name}, 共备份 {len(files_to_backup)} 个文件")
            
        except Exception as e:
            self.add_log(f"备份配置文件时发生错误: {str(e)}")
            
    def show_config_context_menu(self, event, tree, config_type):
        """显示配置项的右键上下文菜单"""
        item = tree.identify_row(event.y)
        if item:
            tree.selection_set(item)
            
            # 创建上下文菜单
            context_menu = tk.Menu(self.root, tearoff=0)
            context_menu.add_command(label="编辑配置", command=lambda: self.edit_button_config(tree, config_type))
            context_menu.add_command(label="修改图标", command=lambda: self.edit_button_icon(tree, config_type))
            context_menu.add_separator()
            context_menu.add_command(label="复制按钮名称", command=lambda: self.copy_button_info(tree, 0))
            context_menu.add_command(label="复制图标名称", command=lambda: self.copy_button_info(tree, 2))
            
            # 显示菜单
            context_menu.tk_popup(event.x_root, event.y_root)
            
    def copy_button_info(self, tree, column_index):
        """复制按钮信息到剪贴板"""
        selection = tree.selection()
        if selection:
            values = tree.item(selection[0])['values']
            if column_index < len(values):
                text = values[column_index]
                self.copy_to_clipboard(text)
                
    def refresh_button_configs(self, config_window):
        """刷新按钮配置"""
        self.load_button_configs()
        
        # 刷新所有配置页面
        if hasattr(self, 'persistent_tree'):
            self.populate_config_tree(self.persistent_tree, 'persistent')
        if hasattr(self, 'project_tree'):
            self.populate_config_tree(self.project_tree, 'project')
            
        self.add_log("按钮配置已刷新")
        messagebox.showinfo("完成", "按钮配置已刷新")
        
    def analyze_registrations(self):
        """分析注册状态"""
        self.status_tree.delete(*self.status_tree.get_children())
        self.missing_registrations.clear()
        self.orphaned_registrations.clear()
        
        # 读取代码中的注册信息
        self.read_code_registrations()
        
        # 分析每个SVG文件
        for svg_info in self.svg_files:
            svg_name = svg_info['name']
            icon_name = f"UtilityExtend.{svg_name}"
            icon_type = svg_info.get('type', 'unknown')
            
            if icon_name in self.registered_icons:
                # 图标已注册
                friendly_name = self.friendly_names.get(icon_name, svg_name)
                type_label = "[持久]" if icon_type == 'persistent' else "[项目]"
                self.status_tree.insert("", "end", values=(f"✅ {type_label} 已注册", f"{svg_name}", friendly_name, "🔧 修改友好名"))
            else:
                # 图标未注册
                type_label = "[持久]" if icon_type == 'persistent' else "[项目]"
                self.status_tree.insert("", "end", values=(f"❌ {type_label} 未注册", f"{svg_name}", "", ""))
                self.missing_registrations.append(svg_info)
                
        # 分析孤立的注册（代码中有但文件不存在）
        for icon_name in self.registered_icons:
            if icon_name.startswith("UtilityExtend."):
                base_name = icon_name.replace("UtilityExtend.", "")
                svg_file = self.plugin_path / "Resources" / f"{base_name}.svg"
                if not svg_file.exists():
                    friendly_name = self.friendly_names.get(icon_name, base_name)
                    self.status_tree.insert("", "end", values=("⚠️ 文件缺失", f"{base_name}", friendly_name, "🔧 修改友好名"))
                    self.orphaned_registrations.append(icon_name)
                    
        # 添加分析结果日志
        total_svg = len(self.svg_files)
        total_registered = len([item for item in self.status_tree.get_children() if "已注册" in self.status_tree.item(item)['values'][0]])
        total_missing = len(self.missing_registrations)
        total_orphaned = len(self.orphaned_registrations)
        
        self.add_log(f"注册状态分析完成:")
        self.add_log(f"  - SVG文件总数: {total_svg}")
        self.add_log(f"  - 已注册图标: {total_registered}")
        self.add_log(f"  - 未注册图标: {total_missing}")
        self.add_log(f"  - 孤立注册: {total_orphaned}")
        
        if total_missing > 0:
            self.add_log(f"需要注册的图标: {', '.join([item['name'] for item in self.missing_registrations])}")
        if total_orphaned > 0:
            self.add_log(f"需要清理的孤立注册: {', '.join([item.replace('UtilityExtend.', '') for item in self.orphaned_registrations])}")
        
        # 更新状态栏
        self.update_status_bar()
        
    def read_code_registrations(self):
        """读取代码中的图标注册信息"""
        self.registered_icons.clear()
        self.friendly_names.clear()
        
        # 读取样式文件中的注册
        style_file = self.source_path / "Private" / "UtilityExtendStyle.cpp"
        if style_file.exists():
            with open(style_file, 'r', encoding='utf-8') as f:
                content = f.read()
                # 查找所有 Style->Set 调用
                pattern = r'Style->Set\("([^"]+)"'
                matches = re.findall(pattern, content)
                self.registered_icons.extend(matches)
                
        # 读取图标注册文件中的信息
        registry_file = self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
        if registry_file.exists():
            with open(registry_file, 'r', encoding='utf-8') as f:
                content = f.read()
                # 查找所有 FToolbarIconInfo 调用，包括友好名
                pattern = r'FToolbarIconInfo\(TEXT\("([^"]+)"\), TEXT\("([^"]+)"'
                matches = re.findall(pattern, content)
                for icon_name, friendly_name in matches:
                    if icon_name not in self.registered_icons:
                        self.registered_icons.append(icon_name)
                    # 存储友好名映射
                    self.friendly_names[icon_name] = friendly_name
                
        # 去重
        self.registered_icons = list(set(self.registered_icons))
        self.add_log(f"代码中找到 {len(self.registered_icons)} 个图标注册")
        self.add_log(f"找到 {len(self.friendly_names)} 个友好名映射")
        self.add_log(f"代码中找到 {len(self.registered_icons)} 个图标注册")
        self.add_log(f"找到 {len(self.friendly_names)} 个友好名映射")
        
    def on_svg_select(self, event):
        """SVG文件选择事件"""
        selection = self.svg_tree.selection()
        if selection:
            item = self.svg_tree.item(selection[0])
            svg_name = item['values'][0]
            svg_path = item['values'][1]
            
            # 检查注册状态
            icon_name = f"UtilityExtend.{svg_name}"
            if icon_name in self.registered_icons:
                status = "✅ 已注册"
                details = f"图标名称: {icon_name}\n文件路径: {svg_path}\n状态: 正常"
            else:
                status = "❌ 未注册"
                details = f"图标名称: {icon_name}\n文件路径: {svg_path}\n状态: 需要注册"
                
            # 在日志区域显示信息
            self.add_log(f"选择SVG文件: {svg_name}")
            self.add_log(details)
            
    def on_status_tree_click(self, event):
        """状态树点击事件处理"""
        region = self.status_tree.identify("region", event.x, event.y)
        if region == "cell":
            column = self.status_tree.identify_column(event.x)
            if column == "#4":  # 操作列
                item = self.status_tree.identify_row(event.y)
                if item:
                    values = self.status_tree.item(item)['values']
                    if len(values) >= 4 and "修改友好名" in values[3]:
                        # 点击了"修改友好名"按钮
                        svg_name = values[1]
                        self.rename_friendly_name_for_item(svg_name)
                        return
        
        # 如果不是点击操作列，则正常处理选择事件
        self.on_status_select(event)
    
    def on_status_select(self, event):
        """状态选择事件"""
        selection = self.status_tree.selection()
        if selection:
            item = self.status_tree.item(selection[0])
            status = item['values'][0]
            details = item['values'][1]
            
            if "未注册" in status:
                # 显示需要添加的代码
                code = self.generate_registration_code(details)
                self.add_log(f"选择状态项目: {details}")
                self.add_log(f"需要添加的代码:\n{code}")
            elif "文件缺失" in status:
                # 显示需要删除的代码
                code = self.generate_removal_code(details)
                self.add_log(f"选择状态项目: {details}")
                self.add_log(f"需要删除的代码:\n{code}")
            else:
                # 显示正常状态
                icon_name = f"UtilityExtend.{details}"
                current_friendly = self.friendly_names.get(icon_name, details)
                info = f"图标技术名称: {icon_name}\n友好显示名称: {current_friendly}\n状态: 正常\n\n说明：\n• 友好名称用于项目设置中的显示\n• 点击'修改友好名'按钮可以修改显示名称\n• 修改后需要重新编译项目\n• 图标功能不受影响"
                self.add_log(f"选择状态项目: {details}")
                self.add_log(info)
                
    def generate_registration_code(self, svg_name):
        """生成注册代码"""
        # 查找图标类型
        icon_type = 'unknown'
        for svg_info in self.svg_files:
            if svg_info['name'] == svg_name:
                icon_type = svg_info.get('type', 'unknown')
                break
        
        if icon_type == 'plugin':
            # 插件图标需要手动添加到代码中
            code = f"""// 插件图标 - 需要在 UtilityExtendStyle.cpp 的基础图标区域添加:
Style->Set("UtilityExtend.{svg_name}", new IMAGE_BRUSH_SVG(TEXT("{svg_name}"), FVector2D(20.0f, 20.0f)));

// 在 UtilityExtendIconRegistry.cpp 的 CreateIconSet 函数中添加:
IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.{svg_name}"), TEXT("{svg_name}"), TEXT("插件图标")));"""

        else:
            code = f"""// 未知图标类型
// 无法确定图标 "{svg_name}" 的类型，请检查文件位置"""
        
        return code
        
    def generate_removal_code(self, icon_name):
        """生成删除代码"""
        code = f"""// 需要从以下位置删除相关代码:
// 1. UtilityExtendStyle.cpp 中的 Style->Set 调用
// 2. UtilityExtendIconRegistry.cpp 中的 FToolbarIconInfo 调用

// 搜索并删除包含 "{icon_name}" 的代码行"""
        return code
        
    def refresh_scan(self):
        """重新扫描文件"""
        self.add_log("开始重新扫描文件...")
        self.scan_files()
        self.analyze_registrations()
        self.add_log("文件扫描完成")
        
    def reregister_icons(self):
        """重新注册图集"""
        if not self.missing_registrations and not self.orphaned_registrations:
            self.add_log("没有需要处理的图标")
            messagebox.showinfo("信息", "没有需要处理的图标")
            return
            
        self.set_status("正在重新注册图集...")
        self.show_progress()
        self.add_log("开始重新注册图集...")
        self.add_log(f"需要注册的图标: {len(self.missing_registrations)}")
        self.add_log(f"需要清理的孤立注册: {len(self.orphaned_registrations)}")
        
        try:
            # 备份原文件
            self.backup_files("重新注册图集")
            
            # 处理未注册的图标
            self.add_missing_registrations()
            
            # 处理孤立的注册
            self.remove_orphaned_registrations()
            
            self.add_log("图标注册更新完成！请重新编译项目。")
            self.set_status("图标注册完成")
            messagebox.showinfo("完成", "图标注册更新完成！\n请重新编译项目。")
            
            # 自动刷新状态
            self.refresh_scan()
        except Exception as e:
            self.set_status("注册失败")
            self.add_log(f"错误: {str(e)}")
        finally:
            self.hide_progress()
        
    def backup_files(self, description="自动备份"):
        """备份原文件，添加时间戳和描述"""
        backup_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Backups"
        backup_dir.mkdir(parents=True, exist_ok=True)
        
        # 创建时间戳
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        backup_name = f"backup_{timestamp}_{description.replace(' ', '_')}"
        
        self.add_log(f"备份目录: {backup_dir}")
        self.add_log(f"备份名称: {backup_name}")
        
        files_to_backup = [
            self.source_path / "Private" / "UtilityExtendStyle.cpp",
            self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
        ]
        
        # 创建备份信息
        backup_info = {
            "timestamp": timestamp,
            "description": description,
            "backup_name": backup_name,
            "files": []
        }
        
        for file_path in files_to_backup:
            if file_path.exists():
                backup_path = backup_dir / f"{backup_name}_{file_path.name}"
                shutil.copy2(file_path, backup_path)
                backup_info["files"].append({
                    "original": str(file_path),
                    "backup": str(backup_path)
                })
                self.add_log(f"已备份: {backup_path}")
                self.add_log(f"已备份: {backup_path}")
            else:
                self.add_log(f"警告: 文件不存在，跳过备份: {file_path}")
        
        # 保存备份信息到JSON文件
        backup_info_file = backup_dir / f"{backup_name}_info.json"
        try:
            with open(backup_info_file, 'w', encoding='utf-8') as f:
                json.dump(backup_info, f, ensure_ascii=False, indent=2)
            self.add_log(f"备份信息已保存: {backup_info_file}")
        except Exception as e:
            self.add_log(f"警告: 无法保存备份信息: {e}")
        
        return backup_name
        
    def add_missing_registrations(self):
        """添加缺失的注册"""
        if not self.missing_registrations:
            return
            
        self.add_log("开始添加缺失的图标注册...")
        
        # 备份现有文件
        self.backup_files("添加缺失图标注册")
        
        # 更新样式文件
        self.update_style_file()
        
        # 更新注册文件
        self.update_registry_file()
        
        self.add_log("缺失的图标注册添加完成")
        
        # 显示成功消息
        messagebox.showinfo("完成", f"图标注册更新完成！\n\n已添加 {len(self.missing_registrations)} 个新图标注册。\n\n注意：\n• 修改已完成，请重新编译项目\n• 现有图标注册已保留\n• 新图标注册已添加到用户自定义区域")
    
    def update_style_file(self):
        """更新样式文件"""
        style_file = self.source_path / "Private" / "UtilityExtendStyle.cpp"
        if not style_file.exists():
            self.add_log(f"错误: 样式文件不存在: {style_file}")
            return
            
        self.add_log(f"更新样式文件: {style_file}")
        
        with open(style_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 分离插件图标和项目图标
        plugin_icons = []
        for svg_info in self.missing_registrations:
            icon_type = svg_info.get('type', 'unknown')
            if icon_type == 'plugin':
                plugin_icons.append(svg_info)
        
        # 只有插件图标需要添加到样式文件
        if plugin_icons:
            # 在用户自定义区域添加代码
            user_custom_section = "// 用户自定义区域"
            if user_custom_section in content:
                # 查找用户自定义区域的位置
                section_start = content.find(user_custom_section)
                section_end = content.find("// ============================================================================", section_start)
                
                if section_start != -1 and section_end != -1:
                    # 获取现有的用户自定义区域内容
                    existing_section = content[section_start:section_end]
                    
                    # 检查每个新图标是否已经存在
                    new_icons_to_add = []
                    for svg_info in plugin_icons:
                        svg_name = svg_info['name']
                        icon_pattern = f'Style->Set("UtilityExtend.{svg_name}", new IMAGE_BRUSH_SVG(TEXT("{svg_name}"), FVector2D(20.0f, 20.0f)));'
                        
                        # 如果图标不存在，则添加到新图标列表
                        if icon_pattern not in existing_section:
                            new_icons_to_add.append(svg_info)
                        else:
                            self.add_log(f"持久化图标 {svg_name} 已存在于样式文件中，跳过")
                    
                    if new_icons_to_add:
                        # 构建新的用户自定义区域内容
                        new_section = user_custom_section + "\n"
                        
                        # 保留现有的图标注册
                        existing_lines = existing_section.split('\n')[1:]  # 跳过第一行的"// 用户自定义区域"
                        for line in existing_lines:
                            if line.strip() and not line.strip().startswith("//"):
                                new_section += line + "\n"
                        
                        # 添加新的持久化图标注册
                        for svg_info in new_icons_to_add:
                            svg_name = svg_info['name']
                            new_section += f'    Style->Set("UtilityExtend.{svg_name}", new IMAGE_BRUSH_SVG(TEXT("{svg_name}"), FVector2D(20.0f, 20.0f)));\n'
                        
                        new_section += "    // ============================================================================"
                        
                        # 替换用户自定义区域
                        new_content = content[:section_start] + new_section + content[section_end + len("// ============================================================================"):]
                        
                        # 写回文件
                        with open(style_file, 'w', encoding='utf-8') as f:
                            f.write(new_content)
                            
                        self.add_log(f"样式文件更新完成，添加了 {len(new_icons_to_add)} 个持久化图标注册")
                        self.add_log(f"已更新样式文件: {style_file}")
                    else:
                        self.add_log("所有持久化图标都已存在于样式文件中，无需更新")
                else:
                    self.add_log("警告: 无法正确定位用户自定义区域")
            else:
                self.add_log("警告: 样式文件中未找到用户自定义区域")
        else:
            self.add_log("没有持久化图标需要添加到样式文件")
    
    def update_registry_file(self):
        """更新注册文件"""
        registry_file = self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
        if not registry_file.exists():
            self.add_log(f"错误: 注册文件不存在: {registry_file}")
            return
            
        self.add_log(f"更新注册文件: {registry_file}")
        
        with open(registry_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 处理插件图标 - 添加到CreateIconSet函数的基础图标区域
        plugin_icons = []
        for svg_info in self.missing_registrations:
            icon_type = svg_info.get('type', 'unknown')
            if icon_type == 'plugin':
                plugin_icons.append(svg_info)
        
        if plugin_icons:
            self.add_plugin_icons_to_icon_set(content, plugin_icons, registry_file)
    
    def add_plugin_icons_to_icon_set(self, content, plugin_icons, registry_file):
        """添加插件图标到图标集"""
        # 查找CreateIconSet函数中的基础图标用户自定义区域
        base_custom_section = "// 基础图标用户自定义区域"
        if base_custom_section in content:
            section_start = content.find(base_custom_section)
            section_end = content.find("// ============================================================================", section_start)
            
            if section_start != -1 and section_end != -1:
                existing_section = content[section_start:section_end]
                
                # 检查每个新图标是否已经存在
                new_icons_to_add = []
                for svg_info in plugin_icons:
                    svg_name = svg_info['name']
                    icon_pattern = f'IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.{svg_name}"), TEXT("{svg_name}"), TEXT("插件图标")));'
                    
                    if icon_pattern not in existing_section:
                        new_icons_to_add.append(svg_info)
                    else:
                        self.add_log(f"插件图标 {svg_name} 已存在于基础图标集中，跳过")
                
                if new_icons_to_add:
                    # 构建新的基础图标自定义区域内容
                    new_section = base_custom_section + "\n"
                    
                    # 保留现有的图标注册
                    existing_lines = existing_section.split('\n')[1:]
                    for line in existing_lines:
                        if line.strip() and not line.strip().startswith("//"):
                            new_section += line + "\n"
                    
                    # 添加新的持久化图标注册
                    for svg_info in new_icons_to_add:
                        svg_name = svg_info['name']
                        new_section += f'    IconInfos.Add(FToolbarIconInfo(TEXT("UtilityExtend.{svg_name}"), TEXT("{svg_name}"), TEXT("持久化图标")));\n'
                    
                    new_section += "    // ============================================================================"
                    
                    # 替换基础图标自定义区域
                    new_content = content[:section_start] + new_section + content[section_end + len("// ============================================================================"):]
                    
                    # 写回文件
                    with open(registry_file, 'w', encoding='utf-8') as f:
                        f.write(new_content)
                        
                    self.add_log(f"基础图标集更新完成，添加了 {len(new_icons_to_add)} 个持久化图标注册")
                else:
                    self.add_log("所有持久化图标都已存在于基础图标集中，无需更新")
            else:
                self.add_log("警告: 无法正确定位基础图标用户自定义区域")
        else:
            self.add_log("警告: 注册文件中未找到基础图标用户自定义区域")
    
    def remove_orphaned_registrations(self):
        """删除孤立的注册"""
        if not self.orphaned_registrations:
            return
            
        self.add_log("开始删除孤立的图标注册...")
        
        # 删除样式文件中的孤立注册
        self.remove_orphaned_from_style()
        
        # 删除注册文件中的孤立注册
        self.remove_orphaned_from_registry()
        
        self.add_log(f"孤立注册删除完成，共删除 {len(self.orphaned_registrations)} 个")
        self.add_log(f"已删除孤立注册: {self.orphaned_registrations}")
        
    def remove_orphaned_from_style(self):
        """从样式文件中删除孤立的注册"""
        style_file = self.source_path / "Private" / "UtilityExtendStyle.cpp"
        if not style_file.exists():
            self.add_log(f"错误: 样式文件不存在: {style_file}")
            return
            
        self.add_log(f"从样式文件中删除孤立注册: {style_file}")
        
        with open(style_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # 删除每个孤立的注册
        deleted_count = 0
        for icon_name in self.orphaned_registrations:
            base_name = icon_name.replace("UtilityExtend.", "")
            # 查找并删除包含该图标的Style->Set行
            # 使用更精确的模式匹配
            pattern = rf'^\s*Style->Set\("{icon_name}", new IMAGE_BRUSH_SVG\(TEXT\("[^"]*"\), FVector2D\([^)]*\)\)\);\s*$'
            if re.search(pattern, content, flags=re.MULTILINE):
                content = re.sub(pattern, '', content, flags=re.MULTILINE)
                deleted_count += 1
                self.add_log(f"从样式文件中删除了图标: {icon_name}")
            else:
                # 如果上面的模式不匹配，尝试更宽松的模式
                pattern2 = rf'^\s*Style->Set\("{icon_name}"[^;]*;\s*$'
                if re.search(pattern2, content, flags=re.MULTILINE):
                    content = re.sub(pattern2, '', content, flags=re.MULTILINE)
                    deleted_count += 1
                    self.add_log(f"从样式文件中删除了图标（宽松匹配）: {icon_name}")
                else:
                    self.add_log(f"警告: 无法找到样式注册行: {icon_name}")
            
        # 写回文件
        with open(style_file, 'w', encoding='utf-8') as f:
            f.write(content)
            
        self.add_log(f"样式文件孤立注册删除完成，删除了 {deleted_count} 个")
        self.add_log(f"已从样式文件中删除孤立注册")
    
    def remove_orphaned_from_registry(self):
        """从注册文件中删除孤立的注册"""
        registry_file = self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
        if not registry_file.exists():
            self.add_log(f"错误: 注册文件不存在: {registry_file}")
            return
            
        self.add_log(f"从注册文件中删除孤立注册: {registry_file}")
        
        with open(registry_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # 删除每个孤立的注册
        deleted_count = 0
        for icon_name in self.orphaned_registrations:
            base_name = icon_name.replace("UtilityExtend.", "")
            # 查找并删除包含该图标的FToolbarIconInfo行
            # 修复正则表达式：第二个参数是友好名称，可能是任意字符串，不一定是文件名
            pattern = rf'^\s*IconInfos\.Add\(FToolbarIconInfo\(TEXT\("{icon_name}"\), TEXT\("[^"]*"\), TEXT\("[^"]*"\)\)\);\s*$'
            if re.search(pattern, content, flags=re.MULTILINE):
                content = re.sub(pattern, '', content, flags=re.MULTILINE)
                deleted_count += 1
                self.add_log(f"从注册文件中删除了图标: {icon_name}")
            else:
                # 如果上面的模式不匹配，尝试更宽松的模式
                pattern2 = rf'^\s*IconInfos\.Add\(FToolbarIconInfo\(TEXT\("{icon_name}"\)[^)]*\)\);\s*$'
                if re.search(pattern2, content, flags=re.MULTILINE):
                    content = re.sub(pattern2, '', content, flags=re.MULTILINE)
                    deleted_count += 1
                    self.add_log(f"从注册文件中删除了图标（宽松匹配）: {icon_name}")
                else:
                    self.add_log(f"警告: 无法找到图标注册行: {icon_name}")
            
        # 写回文件
        with open(registry_file, 'w', encoding='utf-8') as f:
            f.write(content)
            
        self.add_log(f"注册文件孤立注册删除完成，删除了 {deleted_count} 个")
        self.add_log(f"已从注册文件中删除孤立注册")
    
    def import_svg(self, import_type='plugin'):
        """导入SVG文件到插件Resources目录"""
        self.add_log("开始导入SVG文件到插件目录...")
        target_dir = self.plugin_path / "Resources"
        dir_name = "插件目录（Resources）"
        
        # 选择SVG文件
        file_path = filedialog.askopenfilename(
            title=f"选择SVG文件导入到{dir_name}",
            filetypes=[("SVG文件", "*.svg"), ("所有文件", "*.*")],
            initialdir=os.path.expanduser("~")
        )
        
        if not file_path:
            self.add_log("用户取消了SVG文件导入")
            return
            
        try:
            # 获取文件名
            file_name = os.path.basename(file_path)
            # 确保目标目录存在
            target_dir.mkdir(parents=True, exist_ok=True)
            target_path = target_dir / file_name
            
            self.add_log(f"选择的文件: {file_path}")
            self.add_log(f"目标路径: {target_path}")
            
            # 检查文件是否已存在
            if target_path.exists():
                result = messagebox.askyesno(
                    "文件已存在",
                    f"文件 {file_name} 已存在于{dir_name}中，是否覆盖？"
                )
                if not result:
                    self.add_log("用户选择不覆盖现有文件")
                    return
            
            # 复制文件
            shutil.copy2(file_path, target_path)
            
            self.add_log(f"SVG文件导入成功: {file_name} -> {dir_name}")
            
            # 刷新扫描
            self.refresh_scan()
            
        except Exception as e:
            error_msg = f"导入SVG文件时发生错误: {str(e)}"
            self.add_log(f"错误: {error_msg}")
            messagebox.showerror("错误", error_msg)
        
    def generate_report(self):
        """生成报告"""
        self.add_log("开始生成报告...")
        
        try:
            # 创建报告目录
            report_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Reports"
            report_dir.mkdir(parents=True, exist_ok=True)
            
            # 生成报告文件名
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            report_file = report_dir / f"SVG_Report_{timestamp}.txt"
            
            # 生成报告内容
            report_content = []
            report_content.append("SVG图标注册状态报告")
            report_content.append("=" * 50)
            report_content.append(f"生成时间: {time.strftime('%Y-%m-%d %H:%M:%S')}")
            report_content.append(f"项目路径: {self.project_path}")
            report_content.append(f"引擎路径: {self.engine_path}")
            report_content.append("")
            
            # SVG文件统计
            report_content.append("SVG文件统计:")
            report_content.append(f"  总数: {len(self.svg_files)}")
            for svg_info in self.svg_files:
                report_content.append(f"    - {svg_info['name']}: {svg_info['path']}")
            report_content.append("")
            
            # 注册状态统计
            report_content.append("注册状态统计:")
            total_registered = len([item for item in self.status_tree.get_children() if "已注册" in self.status_tree.item(item)['values'][0]])
            report_content.append(f"  已注册: {total_registered}")
            report_content.append(f"  未注册: {len(self.missing_registrations)}")
            report_content.append(f"  孤立注册: {len(self.orphaned_registrations)}")
            report_content.append("")
            
            # 详细状态
            report_content.append("详细状态:")
            for item in self.status_tree.get_children():
                values = self.status_tree.item(item)['values']
                report_content.append(f"  {values[0]} {values[1]} (友好名: {values[2]})")
            
            # 写入文件
            with open(report_file, 'w', encoding='utf-8') as f:
                f.write('\n'.join(report_content))
            
            self.add_log(f"报告生成成功: {report_file}")
            
            # 询问是否打开报告文件
            if messagebox.askyesno("完成", f"报告已生成到:\n{report_file}\n\n是否打开报告文件？"):
                os.startfile(report_file)
                
        except Exception as e:
            error_msg = f"生成报告时发生错误: {str(e)}"
            self.add_log(f"错误: {error_msg}")
            messagebox.showerror("错误", error_msg)
        
    def rename_friendly_name_for_item(self, svg_name):
        """为指定SVG名称重命名友好名"""
        icon_name = f"UtilityExtend.{svg_name}"
        
        # 检查图标是否已注册
        if icon_name not in self.registered_icons:
            messagebox.showwarning("警告", "只能重命名已注册图标的友好名")
            return
            
        self._show_rename_dialog(icon_name, svg_name)
    
    def rename_friendly_name(self):
        """重命名友好名（从菜单调用）"""
        # 获取当前选中的状态项
        selection = self.status_tree.selection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个已注册的图标")
            return
            
        item = self.status_tree.item(selection[0])
        status = item['values'][0]
        details = item['values'][1]
        
        # 只允许重命名已注册的图标
        if "已注册" not in status:
            messagebox.showwarning("警告", "只能重命名已注册图标的友好名")
            return
            
        icon_name = f"UtilityExtend.{details}"
        self._show_rename_dialog(icon_name, details)
    
    def _show_rename_dialog(self, icon_name, svg_name):
        """显示重命名对话框"""
        current_friendly = self.friendly_names.get(icon_name, svg_name)
        
        # 创建重命名对话框
        rename_dialog = tk.Toplevel(self.root)
        rename_dialog.title("重命名友好名")
        rename_dialog.geometry("500x400")
        rename_dialog.transient(self.root)
        rename_dialog.grab_set()
        
        # 居中显示对话框
        self.center_dialog(rename_dialog)
        
        # 对话框内容
        main_frame = ttk.Frame(rename_dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 说明信息
        info_text = """友好名称说明：
• 友好名称是图标在项目设置中的显示名称
• 修改友好名称不会影响SVG文件名或图标的技术名称
• 修改后需要重新编译项目才能生效
• 友好名称仅用于UI显示，不影响图标功能"""
        
        info_label = ttk.Label(main_frame, text=info_text, font=("Arial", 9), foreground="blue")
        info_label.pack(anchor=tk.W, pady=(0, 20))
        
        # 分隔线
        separator = ttk.Separator(main_frame, orient='horizontal')
        separator.pack(fill=tk.X, pady=(0, 20))
        
        # 图标名称（只读）
        ttk.Label(main_frame, text=f"图标技术名称: {icon_name}", font=("Arial", 10)).pack(anchor=tk.W, pady=(0, 10))
        
        # 当前友好名
        ttk.Label(main_frame, text="当前友好名称:").pack(anchor=tk.W)
        current_label = ttk.Label(main_frame, text=current_friendly, font=("Arial", 10, "bold"))
        current_label.pack(anchor=tk.W, pady=(0, 15))
        
        # 新友好名输入
        ttk.Label(main_frame, text="新友好名称:").pack(anchor=tk.W)
        new_name_var = tk.StringVar(value=current_friendly)
        new_name_entry = ttk.Entry(main_frame, textvariable=new_name_var, width=30)
        new_name_entry.pack(anchor=tk.W, pady=(0, 20))
        new_name_entry.focus()
        new_name_entry.select_range(0, tk.END)
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(0, 10))
        
        def apply_rename():
            new_name = new_name_var.get().strip()
            if not new_name:
                messagebox.showwarning("警告", "友好名称不能为空")
                return
                
            if new_name == current_friendly:
                messagebox.showinfo("信息", "友好名称没有变化")
                rename_dialog.destroy()
                return
                
            try:
                # 备份文件
                self.backup_files("更新友好名称")
                
                # 更新友好名
                self.update_friendly_name(icon_name, current_friendly, new_name)
                
                messagebox.showinfo("完成", f"友好名称已更新为: {new_name}\n\n注意：\n• 修改已完成，请重新编译项目\n• 图标技术名称保持不变\n• 新友好名称将在项目设置中显示")
                
                # 关闭对话框并刷新状态
                rename_dialog.destroy()
                self.refresh_scan()
                
            except Exception as e:
                messagebox.showerror("错误", f"更新友好名称时发生错误：\n{str(e)}")
        
        def cancel_rename():
            rename_dialog.destroy()
        
        # 应用按钮
        apply_button = ttk.Button(button_frame, text="应用更改", command=apply_rename)
        apply_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 取消按钮
        cancel_button = ttk.Button(button_frame, text="取消", command=cancel_rename)
        cancel_button.pack(side=tk.LEFT)
        
        # 绑定回车键
        rename_dialog.bind('<Return>', lambda e: apply_rename())
        rename_dialog.bind('<Escape>', lambda e: cancel_rename())
        
    def update_friendly_name(self, icon_name, old_friendly, new_friendly):
        """更新友好名"""
        try:
            self.add_log(f"开始更新友好名: {icon_name}")
            self.add_log(f"旧友好名: {old_friendly} -> 新友好名: {new_friendly}")
            
            # 更新注册文件中的友好名（这是唯一需要修改的地方）
            self.update_registry_friendly_name(icon_name, old_friendly, new_friendly)
            
            # 更新内存中的友好名映射
            self.friendly_names[icon_name] = new_friendly
            
            self.add_log(f"友好名更新成功: {icon_name} -> {old_friendly} -> {new_friendly}")
            self.add_log(f"已更新友好名: {icon_name} -> {old_friendly} -> {new_friendly}")
            
        except Exception as e:
            error_msg = f"更新友好名时发生错误：{str(e)}"
            self.add_log(f"错误: {error_msg}")
            self.add_log(error_msg)
            raise
        
    def update_registry_friendly_name(self, icon_name, old_friendly, new_friendly):
        """更新注册文件中的友好名"""
        registry_file = self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
        if not registry_file.exists():
            error_msg = f"注册文件不存在: {registry_file}"
            self.add_log(f"错误: {error_msg}")
            raise FileNotFoundError(error_msg)
            
        self.add_log(f"更新注册文件中的友好名: {registry_file}")
        
        with open(registry_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # 使用更安全的方法来更新友好名称
        # 首先尝试精确匹配
        updated = False
        
        # 方法1：精确的正则表达式匹配
        pattern = rf'IconInfos\.Add\(FToolbarIconInfo\(TEXT\("{re.escape(icon_name)}"\), TEXT\("{re.escape(old_friendly)}"\), TEXT\("[^"]*"\)\)\);'
        if re.search(pattern, content):
            replacement = f'IconInfos.Add(FToolbarIconInfo(TEXT("{icon_name}"), TEXT("{new_friendly}"), TEXT("用户自定义图标")));'
            content = re.sub(pattern, replacement, content)
            updated = True
            self.add_log(f"使用正则表达式更新友好名: {old_friendly} -> {new_friendly}")
            self.add_log(f"使用正则表达式更新友好名: {old_friendly} -> {new_friendly}")
        
        # 方法2：如果正则表达式失败，使用逐行查找和替换
        if not updated:
            lines = content.split('\n')
            for i, line in enumerate(lines):
                # 检查这一行是否包含我们要修改的图标信息
                if (f'FToolbarIconInfo(TEXT("{icon_name}")' in line and 
                    f'TEXT("{old_friendly}")' in line and
                    'IconInfos.Add(' in line):
                    
                    # 安全地替换友好名称，保持其他部分不变
                    new_line = line.replace(f'TEXT("{old_friendly}")', f'TEXT("{new_friendly}")')
                    lines[i] = new_line
                    updated = True
                    self.add_log(f"使用逐行查找更新第{i+1}行的友好名: {old_friendly} -> {new_friendly}")
                    self.add_log(f"使用逐行查找更新第{i+1}行的友好名: {old_friendly} -> {new_friendly}")
                    break
            
            if updated:
                content = '\n'.join(lines)
        
        # 如果两种方法都失败了，抛出异常
        if not updated:
            error_msg = f"无法在注册文件中找到图标 {icon_name} 的友好名称 {old_friendly}"
            self.add_log(f"错误: {error_msg}")
            raise ValueError(error_msg)
        
        # 写回文件
        with open(registry_file, 'w', encoding='utf-8') as f:
            f.write(content)
            
        self.add_log(f"注册文件友好名更新成功: {registry_file}")
        self.add_log(f"已成功更新注册文件: {registry_file}")
        
    def open_code_files(self):
        """打开代码文件查看"""
        self.add_log("打开代码文件选择对话框...")
        
        # 创建文件选择对话框
        file_dialog = tk.Toplevel(self.root)
        file_dialog.title("选择要查看的代码文件")
        file_dialog.geometry("500x400")
        file_dialog.transient(self.root)
        file_dialog.grab_set()
        
        # 居中显示对话框
        self.center_dialog(file_dialog)
        
        # 对话框内容
        main_frame = ttk.Frame(file_dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 标题已移除
        
        # 文件列表框架
        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 20))
        
        # 文件列表
        file_listbox = tk.Listbox(list_frame, selectmode=tk.SINGLE)
        file_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # 滚动条
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=file_listbox.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        file_listbox.configure(yscrollcommand=scrollbar.set)
        
        # 添加可查看的文件
        code_files = [
            ("UtilityExtendStyle.cpp", "样式定义文件"),
            ("UtilityExtendIconRegistry.cpp", "图标注册文件"),
            ("UtilityExtend.Build.cs", "构建配置文件"),
            ("UtilityExtend.h", "头文件")
        ]
        
        for file_name, description in code_files:
            file_listbox.insert(tk.END, f"{file_name} - {description}")
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X)
        
        def open_selected_file():
            selection = file_listbox.curselection()
            if not selection:
                messagebox.showwarning("警告", "请先选择一个文件")
                return
                
            selected_index = selection[0]
            file_name = code_files[selected_index][0]
            
            # 构建文件路径
            if file_name.endswith('.cpp') or file_name.endswith('.h'):
                file_path = self.source_path / "Private" / file_name
                if not file_path.exists():
                    file_path = self.source_path / "Public" / file_name
            else:
                file_path = self.source_path / file_name
                
            if not file_path.exists():
                error_msg = f"文件不存在: {file_path}"
                self.add_log(f"错误: {error_msg}")
                messagebox.showerror("错误", error_msg)
                return
                
            try:
                # 使用Windows默认程序打开文件
                self.add_log(f"打开代码文件: {file_path}")
                os.startfile(str(file_path))
                file_dialog.destroy()
            except Exception as e:
                error_msg = f"打开文件时发生错误: {str(e)}"
                self.add_log(f"错误: {error_msg}")
                messagebox.showerror("错误", error_msg)
    
    def get_current_time(self):
        """获取当前时间"""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def show_tool_settings(self):
        """显示工具设置对话框"""
        self.add_log("打开工具设置对话框...")
        
        settings_dialog = tk.Toplevel(self.root)
        settings_dialog.title("工具设置")
        settings_dialog.geometry("700x600")
        settings_dialog.transient(self.root)
        settings_dialog.grab_set()
        
        # 居中显示对话框
        self.center_dialog(settings_dialog)
        
        # 主框架
        main_frame = ttk.Frame(settings_dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 标题已移除
        
        # 创建滚动框架
        canvas = tk.Canvas(main_frame)
        scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # 项目名称设置
        project_name_frame = ttk.LabelFrame(scrollable_frame, text="项目名称", padding="10")
        project_name_frame.pack(fill=tk.X, pady=(0, 15))
        
        project_name_var = tk.StringVar(value=self.project_name)
        project_name_entry = ttk.Entry(project_name_frame, textvariable=project_name_var, width=50)
        project_name_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 10))
        
        project_name_help = ttk.Label(project_name_frame, text="用于生成项目文件和解决方案文件的名称", 
                                    font=("Arial", 9), foreground="gray")
        project_name_help.pack(side=tk.RIGHT, padx=(10, 0))
        

        
        # 项目路径设置
        project_frame = ttk.LabelFrame(scrollable_frame, text="项目路径", padding="10")
        project_frame.pack(fill=tk.X, pady=(0, 15))
        
        project_path_var = tk.StringVar(value=str(self.project_path) if self.project_path else "")
        project_entry = ttk.Entry(project_frame, textvariable=project_path_var, width=50)
        project_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 10))
        
        def browse_project():
            path = filedialog.askdirectory(title="选择项目目录")
            if path:
                project_path_var.set(path)
                self.add_log(f"选择项目路径: {path}")
        
        browse_project_button = ttk.Button(project_frame, text="浏览", command=browse_project)
        browse_project_button.pack(side=tk.RIGHT)
        
        # 引擎路径设置
        engine_frame = ttk.LabelFrame(scrollable_frame, text="虚幻引擎路径", padding="10")
        engine_frame.pack(fill=tk.X, pady=(0, 15))
        
        engine_path_var = tk.StringVar(value=str(self.engine_path) if self.engine_path else "")
        engine_entry = ttk.Entry(engine_frame, textvariable=engine_path_var, width=50)
        engine_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 10))
        
        def browse_engine():
            path = filedialog.askdirectory(title="选择虚幻引擎目录")
            if path:
                engine_path_var.set(path)
                self.add_log(f"选择引擎路径: {path}")
        
        browse_engine_button = ttk.Button(engine_frame, text="浏览", command=browse_engine)
        browse_engine_button.pack(side=tk.RIGHT)
        
        # 自动检测按钮
        auto_detect_frame = ttk.Frame(scrollable_frame)
        auto_detect_frame.pack(fill=tk.X, pady=(0, 20))
        
        def auto_detect():
            self.add_log("开始自动检测路径...")
            self.project_path = self._find_project_path()
            self.engine_path = self._find_engine_path()
            project_path_var.set(str(self.project_path) if self.project_path else "")
            engine_path_var.set(str(self.engine_path) if self.engine_path else "")
            self.add_log("路径自动检测完成")
            messagebox.showinfo("自动检测", "路径自动检测完成")
        
        auto_detect_button = ttk.Button(auto_detect_frame, text="自动检测路径", command=auto_detect)
        auto_detect_button.pack(side=tk.LEFT)
        
        # 按钮框架
        button_frame = ttk.Frame(scrollable_frame)
        button_frame.pack(fill=tk.X)
        
        def apply_settings():
            # 更新项目名称
            new_project_name = project_name_var.get().strip()
            if new_project_name and new_project_name != self.project_name:
                self.project_name = new_project_name
                self.add_log(f"设置项目名称: {self.project_name}")
            
            # 插件名称保持硬编码，不可修改
            # self.plugin_name = "UtilityExtend"  # 硬编码，不可配置
            
            # 更新路径
            project_path = project_path_var.get().strip()
            engine_path = engine_path_var.get().strip()
            
            if project_path:
                self.project_path = Path(project_path)
                self.add_log(f"设置项目路径: {self.project_path}")
            if engine_path:
                self.engine_path = Path(engine_path)
                self.add_log(f"设置引擎路径: {self.engine_path}")
            
            # 保存配置
            self.save_config()
            
            self.add_log("工具设置已保存")
            messagebox.showinfo("完成", "工具设置已保存\n\n注意：修改项目名称后，可能需要重新扫描文件以更新相关路径")
            settings_dialog.destroy()
        
        def cancel_settings():
            self.add_log("取消工具设置")
            settings_dialog.destroy()
        
        # 应用按钮
        apply_button = ttk.Button(button_frame, text="应用设置", command=apply_settings)
        apply_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 取消按钮
        cancel_button = ttk.Button(button_frame, text="取消", command=cancel_settings)
        cancel_button.pack(side=tk.LEFT)
        
        # 配置滚动
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        # 绑定回车键
        settings_dialog.bind('<Return>', lambda e: apply_settings())
        settings_dialog.bind('<Escape>', lambda e: cancel_settings())
        
        # 设置滚动区域
        canvas.configure(scrollregion=canvas.bbox("all"))
    

    
    def open_project_sln(self):
        """打开项目SLN文件进行手动编译"""
        try:
            if not self.project_path:
                error_msg = "未找到项目路径，请在配置菜单中设置路径"
                messagebox.showerror("错误", error_msg)
                return
            
            sln_file = self.project_path / f"{self.project_name}.sln"
            if not sln_file.exists():
                error_msg = f"解决方案文件不存在: {sln_file}"
                messagebox.showerror("错误", error_msg)
                return
            
            # 使用系统默认程序打开SLN文件
            import subprocess
            import platform
            
            if platform.system() == "Windows":
                # Windows系统使用start命令
                subprocess.run(["start", str(sln_file)], shell=True, check=True)
                self.add_log(f"已打开项目解决方案文件: {sln_file}")
                self.set_status("已打开项目SLN文件")
            else:
                # 其他系统使用open命令
                subprocess.run(["open", str(sln_file)], check=True)
                self.add_log(f"已打开项目解决方案文件: {sln_file}")
                self.set_status("已打开项目SLN文件")
                
        except Exception as e:
            error_msg = f"打开项目SLN文件失败: {str(e)}"
            messagebox.showerror("错误", error_msg)
            self.add_log(f"错误: {error_msg}")
    
    def recompile_project(self):
        """重新编译项目"""
        try:
            self.set_status("正在重新编译项目...")
            self.show_progress()
            self.add_log("开始重新编译项目...")
            
            # 检查路径设置
            if not self.project_path:
                error_msg = "未找到项目路径，请在配置菜单中设置路径"
                self.add_log(f"错误: {error_msg}")
                self.set_status("编译失败")
                messagebox.showerror("错误", error_msg)
                self.hide_progress()
                return
            
            if not self.engine_path:
                error_msg = "未找到虚幻引擎路径，请在配置菜单中设置路径"
                self.add_log(f"错误: {error_msg}")
                self.set_status("编译失败")
                messagebox.showerror("错误", error_msg)
                self.hide_progress()
                return
            
            # 验证路径有效性
            if not self.project_path.exists():
                error_msg = f"项目路径不存在: {self.project_path}"
                self.add_log(f"错误: {error_msg}")
                self.set_status("编译失败")
                messagebox.showerror("错误", error_msg)
                self.hide_progress()
                return
                
            if not self.engine_path.exists():
                error_msg = f"引擎路径不存在: {self.engine_path}"
                self.add_log(f"错误: {error_msg}")
                self.set_status("编译失败")
                messagebox.showerror("错误", error_msg)
                self.hide_progress()
                return
            
            self.add_log(f"项目路径: {self.project_path}")
            self.add_log(f"引擎路径: {self.engine_path}")
            self.add_log(f"插件路径: {self.plugin_path}")
            
            # 检查项目文件
            uproject_file = self.project_path / f"{self.project_name}.uproject"
            if not uproject_file.exists():
                error_msg = f"项目文件不存在: {uproject_file}"
                self.add_log(f"错误: {error_msg}")
                self.set_status("编译失败")
                messagebox.showerror("错误", error_msg)
                self.hide_progress()
                return
            
            # 检查解决方案文件
            sln_file = self.project_path / f"{self.project_name}.sln"
            if not sln_file.exists():
                error_msg = f"解决方案文件不存在: {sln_file}"
                self.add_log(f"错误: {error_msg}")
                self.set_status("编译失败")
                messagebox.showerror("错误", error_msg)
                self.hide_progress()
                return
            
            self.add_log("项目文件检查通过")
            
            # 检查是否有编辑器或LiveCode正在运行
            if self._is_editor_running():
                self.add_log("检测到编辑器或LiveCode正在运行")
                result = messagebox.askyesno(
                    "检测到编辑器运行", 
                    "检测到虚幻编辑器或LiveCode正在运行。\n\n"
                    "重新编译需要停止编辑器才能生效。\n\n"
                    "是否要停止编辑器并继续编译？"
                )
                
                if not result:
                    self.add_log("用户选择不停止编辑器，编译取消")
                    self.set_status("编译取消")
                    messagebox.showwarning(
                        "编译取消", 
                        "由于编辑器仍在运行，编译将无法生效。\n"
                        "请先关闭编辑器，然后重新尝试编译。"
                    )
                    self.hide_progress()
                    return
                
                # 尝试停止编辑器
                self.add_log("尝试停止编辑器...")
                if not self._stop_editor():
                    error_msg = "无法停止编辑器，编译取消"
                    self.add_log(f"错误: {error_msg}")
                    self.set_status("编译失败")
                    messagebox.showerror("错误", error_msg)
                    self.hide_progress()
                    return
                self.add_log("编辑器已停止")
            
            # 执行编译
            self.add_log("开始执行编译...")
            self._execute_compilation()
            
        except Exception as e:
            error_msg = f"编译过程中发生错误：{str(e)}"
            self.add_log(f"错误: {error_msg}")
            self.set_status("编译失败")
            messagebox.showerror("编译错误", error_msg)
            self.hide_progress()
        finally:
            # 确保进度条被隐藏
            self.hide_progress()
    
    def _is_editor_running(self):
        """检查编辑器是否正在运行"""
        import psutil
        
        # 查找虚幻编辑器进程
        editor_processes = []
        editor_names = [
            'unrealeditor', 'unrealeditor.exe', 
            'ue4editor', 'ue4editor.exe',
            'ue5editor', 'ue5editor.exe',
            'unrealeditor-win64-shipping', 'unrealeditor-win64-shipping.exe',
            'unrealeditor-win64-debug', 'unrealeditor-win64-debug.exe',
            'unrealeditor-win64-development', 'unrealeditor-win64-development.exe'
        ]
        
        for proc in psutil.process_iter(['pid', 'name', 'exe']):
            try:
                proc_name = proc.info['name'].lower()
                if any(name in proc_name for name in editor_names):
                    editor_processes.append(proc)
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        # 查找LiveCode进程
        livecode_processes = []
        livecode_names = [
            'livecode', 'livecode.exe',
            'livecodeeditor', 'livecodeeditor.exe'
        ]
        
        for proc in psutil.process_iter(['pid', 'name', 'exe']):
            try:
                proc_name = proc.info['name'].lower()
                if any(name in proc_name for name in livecode_names):
                    livecode_processes.append(proc)
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        if editor_processes:
            self.add_log(f"发现 {len(editor_processes)} 个虚幻编辑器进程")
            for proc in editor_processes:
                try:
                    self.add_log(f"  - {proc.info['name']} (PID: {proc.info['pid']})")
                except:
                    pass
        if livecode_processes:
            self.add_log(f"发现 {len(livecode_processes)} 个LiveCode进程")
            for proc in livecode_processes:
                try:
                    self.add_log(f"  - {proc.info['name']} (PID: {proc.info['pid']})")
                except:
                    pass
            
        return len(editor_processes) > 0 or len(livecode_processes) > 0
    
    def _stop_editor(self):
        """尝试停止编辑器进程"""
        import psutil
        
        stopped_count = 0
        failed_count = 0
        
        # 停止虚幻编辑器进程
        editor_names = [
            'unrealeditor', 'unrealeditor.exe', 
            'ue4editor', 'ue4editor.exe',
            'ue5editor', 'ue5editor.exe',
            'unrealeditor-win64-shipping', 'unrealeditor-win64-shipping.exe',
            'unrealeditor-win64-debug', 'unrealeditor-win64-debug.exe',
            'unrealeditor-win64-development', 'unrealeditor-win64-development.exe'
        ]
        
        for proc in psutil.process_iter(['pid', 'name']):
            try:
                proc_name = proc.info['name'].lower()
                if any(name in proc_name for name in editor_names):
                    self.add_log(f"停止虚幻编辑器进程: {proc.info['name']} (PID: {proc.info['pid']})")
                    try:
                        proc.terminate()
                        stopped_count += 1
                    except Exception as e:
                        self.add_log(f"无法停止进程 {proc.info['name']}: {e}")
                        failed_count += 1
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        # 停止LiveCode进程
        livecode_names = [
            'livecode', 'livecode.exe',
            'livecodeeditor', 'livecodeeditor.exe'
        ]
        
        for proc in psutil.process_iter(['pid', 'name']):
            try:
                proc_name = proc.info['name'].lower()
                if any(name in proc_name for name in livecode_names):
                    self.add_log(f"停止LiveCode进程: {proc.info['name']} (PID: {proc.info['pid']})")
                    try:
                        proc.terminate()
                        stopped_count += 1
                    except Exception as e:
                        self.add_log(f"无法停止进程 {proc.info['name']}: {e}")
                        failed_count += 1
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        # 等待进程完全停止
        import time
        if stopped_count > 0:
            self.add_log(f"等待 {stopped_count} 个进程完全停止...")
            time.sleep(3)  # 增加等待时间
            
            # 检查进程是否真的停止了
            still_running = 0
            for proc in psutil.process_iter(['pid', 'name']):
                try:
                    proc_name = proc.info['name'].lower()
                    if any(name in proc_name for name in editor_names + livecode_names):
                        still_running += 1
                except:
                    continue
            
            if still_running > 0:
                self.add_log(f"警告: 仍有 {still_running} 个进程在运行")
            else:
                self.add_log("所有进程已成功停止")
        
        if failed_count > 0:
            self.add_log(f"警告: {failed_count} 个进程无法停止")
        
        self.add_log(f"成功停止 {stopped_count} 个进程")
        return stopped_count > 0
    
    def _execute_compilation(self):
        """执行编译 - 只使用UBT（Unreal Build Tool）进行编译"""
        self.add_log("开始编译项目...")
        
        # 显示主窗口的进度条
        self.show_progress()
        self.set_status("正在编译项目...")
        
        # 清空日志区域，显示编译日志
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        
        # 编译结果和错误标志
        compilation_result = {"success": False, "error": None, "returncode": -1}
        compilation_completed = threading.Event()
        
        # 在新线程中执行编译
        def compile_thread():
            log_dir = None
            log_file_path = None
            try:
                # 创建日志目录
                log_dir = self._create_log_directory()
                log_file_path = log_dir / f"compilation_{self.get_current_time().replace(':', '-').replace(' ', '_')}.log"
                
                self.add_log(f"开始编译项目: {self.project_path}")
                self.add_log(f"日志文件: {log_file_path}")
                
                self.set_status("正在生成项目文件...")
                
                # 使用UBT生成项目文件
                try:
                    ubt_path = self.engine_path / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.exe"
                    if not ubt_path.exists():
                        raise Exception(f"UBT路径不存在: {ubt_path}")
                    
                    generate_cmd = [
                        str(ubt_path), 
                        "-projectfiles", 
                        "-project=" + str(self.project_path / f"{self.project_name}.uproject"), 
                        "-game", 
                        "-rocket", 
                        "-progress"
                    ]
                    
                    self.add_log(f"使用UBT生成项目文件: {ubt_path}")
                    self.add_log(f"执行命令: {' '.join(generate_cmd)}")
                    
                    # 使用Popen实现实时输出
                    process = subprocess.Popen(
                        generate_cmd, 
                        cwd=str(self.project_path), 
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True,
                        encoding='utf-8',
                        bufsize=1,
                        universal_newlines=True
                    )
                    
                    # 实时读取输出
                    stdout_lines = []
                    stderr_lines = []
                    
                    # 创建输出读取线程
                    def read_generate_output(pipe, lines_list, is_stderr=False):
                        try:
                            for line in iter(pipe.readline, ''):
                                if line:
                                    line = line.rstrip()
                                    lines_list.append(line)
                                    # 实时输出到主窗口日志
                                    if is_stderr:
                                        self.root.after(0, lambda l=line: self.add_log(f"生成错误: {l}", "ERROR"))
                                    else:
                                        self.root.after(0, lambda l=line: self.add_log(f"生成输出: {l}"))
                        except Exception as e:
                            self.root.after(0, lambda: self.add_log(f"读取生成输出异常: {e}", "ERROR"))
                    
                    # 启动输出读取线程
                    stdout_thread = threading.Thread(target=read_generate_output, args=(process.stdout, stdout_lines, False), daemon=True)
                    stderr_thread = threading.Thread(target=read_generate_output, args=(process.stderr, stderr_lines, True), daemon=True)
                    stdout_thread.start()
                    stderr_thread.start()
                    
                    # 等待进程完成，带超时检查
                    try:
                        return_code = process.wait(timeout=300)  # 5分钟超时
                    except subprocess.TimeoutExpired:
                        self.add_log("项目文件生成超时，正在终止进程...", "ERROR")
                        process.terminate()
                        try:
                            process.wait(timeout=10)  # 等待进程终止
                        except subprocess.TimeoutExpired:
                            process.kill()  # 强制杀死进程
                        raise Exception("项目文件生成超时")
                    
                    # 等待输出线程完成
                    stdout_thread.join(timeout=5)
                    stderr_thread.join(timeout=5)
                    
                    if return_code == 0:
                        self.add_log("项目文件生成成功")
                        if stdout_lines:
                            self.add_log(f"生成输出: {len(stdout_lines)} 行")
                    else:
                        error_msg = f"项目文件生成失败: {stderr_lines}"
                        self.add_log(error_msg, "ERROR")
                        raise Exception(error_msg)
                        
                except Exception as e:
                    error_msg = f"项目文件生成异常: {str(e)}"
                    self.add_log(error_msg, "ERROR")
                    raise Exception(error_msg)
                
                self.set_status("正在编译插件...")
                
                # 使用UBT编译插件
                try:
                    # 构建UBT编译命令
                    # UBT命令格式: UnrealBuildTool.exe [TargetName] [Platform] [Configuration] [ProjectFile] [AdditionalArguments...]
                    build_cmd = [
                        str(ubt_path),
                        "NEXBoxEditor",  # TargetName - 编辑器目标
                        "Win64",         # Platform
                        "Development",   # Configuration
                        str(self.project_path / f"{self.project_name}.uproject"),  # ProjectFile
                        "-plugin=" + str(self.plugin_path / "UtilityExtend.uplugin")
                    ]
                    
                    self.add_log(f"使用UBT编译插件: {ubt_path}")
                    self.add_log(f"执行编译命令: {' '.join(build_cmd)}")
                    
                    # 设置环境变量
                    env = os.environ.copy()
                    if self.engine_path:
                        env['UE_EDITOR_PATH'] = str(self.engine_path / "Binaries" / "Win64" / "UnrealEditor.exe")
                        env['UE_ENGINE_PATH'] = str(self.engine_path)
                    
                    self.add_log("开始执行编译...")
                    self.set_status("正在编译项目，请稍候...")
                    
                    # 使用Popen实现实时输出
                    process = subprocess.Popen(
                        build_cmd, 
                        cwd=str(self.project_path), 
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True,
                        encoding='utf-8',
                        env=env,
                        bufsize=1,
                        universal_newlines=True
                    )
                    
                    # 实时读取输出
                    stdout_lines = []
                    stderr_lines = []
                    
                    # 创建输出读取线程
                    def read_output(pipe, lines_list, is_stderr=False):
                        try:
                            for line in iter(pipe.readline, ''):
                                if line:
                                    line = line.rstrip()
                                    lines_list.append(line)
                                    # 实时输出到主窗口日志
                                    if is_stderr:
                                        self.root.after(0, lambda l=line: self.add_log(f"编译错误: {l}", "ERROR"))
                                    else:
                                        self.root.after(0, lambda l=line: self.add_log(f"编译输出: {l}"))
                        except Exception as e:
                            self.root.after(0, lambda: self.add_log(f"读取输出异常: {e}", "ERROR"))
                    
                    # 启动输出读取线程
                    stdout_thread = threading.Thread(target=read_output, args=(process.stdout, stdout_lines, False), daemon=True)
                    stderr_thread = threading.Thread(target=read_output, args=(process.stderr, stderr_lines, True), daemon=True)
                    stdout_thread.start()
                    stderr_thread.start()
                    
                    # 等待进程完成，带超时检查
                    try:
                        return_code = process.wait(timeout=1800)  # 30分钟超时
                    except subprocess.TimeoutExpired:
                        self.add_log("编译超时，正在终止进程...", "ERROR")
                        process.terminate()
                        try:
                            process.wait(timeout=10)  # 等待进程终止
                        except subprocess.TimeoutExpired:
                            process.kill()  # 强制杀死进程
                        raise Exception("编译超时，请检查项目大小和编译环境")
                    
                    # 等待输出线程完成
                    stdout_thread.join(timeout=5)
                    stderr_thread.join(timeout=5)
                    
                    self.add_log(f"编译完成，返回码: {return_code}")
                    
                    # 合并输出
                    stdout_content = '\n'.join(stdout_lines)
                    stderr_content = '\n'.join(stderr_lines)
                    
                    # 保存日志到文件
                    self._save_log_to_file(log_file_path, self._get_log_content())
                    self.add_log(f"日志已保存到: {log_file_path}")
                    
                    # 设置编译结果
                    compilation_result["success"] = (return_code == 0)
                    compilation_result["returncode"] = return_code
                    compilation_result["stdout"] = stdout_content
                    compilation_result["stderr"] = stderr_content
                    
                    # 标记编译完成
                    compilation_completed.set()
                    
                    # 在主线程中更新UI
                    self.root.after(0, lambda: compilation_complete(compilation_result, log_dir))
                    
                except Exception as e:
                    self.add_log(f"编译异常: {str(e)}", "ERROR")
                    # 保存错误日志
                    if log_file_path:
                        try:
                            self._save_log_to_file(log_file_path, self._get_log_content())
                            self.add_log(f"已保存错误日志到: {log_file_path}")
                        except Exception as save_e:
                            self.add_log(f"保存错误日志失败: {save_e}")
                    raise e
                
            except Exception as e:
                # 保存错误日志
                if log_file_path:
                    self._save_log_to_file(log_file_path, self._get_log_content())
                
                # 设置错误结果
                compilation_result["error"] = str(e)
                compilation_result["success"] = False
                
                # 标记编译完成
                compilation_completed.set()
                
                # 在主线程中显示错误
                self.root.after(0, lambda: compilation_error(str(e)))
        
        def compilation_complete(result, log_dir):
            self.hide_progress()
            
            if result["success"]:
                self.add_log("编译成功完成！", "SUCCESS")
                self.set_status("编译成功")
                
                # 询问用户下一步操作
                result_dialog = messagebox.askyesnocancel(
                    "编译完成", 
                    "项目编译成功！\n\n现在可以重新启动编辑器了。\n\n请选择下一步操作：\n"
                    "• 是 - 打开项目文件\n"
                    "• 否 - 打开日志文件夹\n"
                    "• 取消 - 仅显示完成消息"
                )
                
                if result_dialog is True:  # 用户选择打开项目
                    try:
                        # 尝试打开项目文件
                        uproject_file = self.project_path / f"{self.project_name}.uproject"
                        if uproject_file.exists():
                            os.startfile(str(uproject_file))
                            self.add_log("已打开项目文件")
                        else:
                            # 如果.uproject文件不存在，尝试打开.sln文件
                            sln_file = self.project_path / f"{self.project_name}.sln"
                            if sln_file.exists():
                                os.startfile(str(sln_file))
                                self.add_log("已打开项目解决方案文件")
                            else:
                                # 如果都不存在，打开项目文件夹
                                os.startfile(str(self.project_path))
                                self.add_log("已打开项目文件夹")
                    except Exception as e:
                        error_msg = f"无法打开项目文件: {str(e)}"
                        self.add_log(f"警告: {error_msg}", "WARNING")
                        # 回退到打开日志文件夹
                        if log_dir:
                            try:
                                os.startfile(str(log_dir))
                                self.add_log("已打开日志文件夹")
                            except Exception as e2:
                                self.add_log(f"警告: 无法打开日志文件夹: {str(e2)}", "WARNING")
                
                elif result_dialog is False:  # 用户选择打开日志文件夹
                    if log_dir:
                        try:
                            os.startfile(str(log_dir))
                            self.add_log("已打开日志文件夹")
                        except Exception as e:
                            error_msg = f"无法打开日志文件夹: {str(e)}"
                            self.add_log(f"警告: {error_msg}", "WARNING")
                

            else:
                self.add_log("编译完成但存在错误", "WARNING")
                self.set_status("编译失败")
                # 显示编译输出
                self._show_compilation_output(result.get("stdout", ""), result.get("stderr", ""))
        
        def compilation_error(error_msg):
            self.hide_progress()
            self.add_log(f"编译过程中发生错误: {error_msg}", "ERROR")
            self.set_status("编译失败")
            messagebox.showerror("编译错误", f"编译过程中发生错误：\n{error_msg}")
        
        # 启动编译线程
        compile_thread = threading.Thread(target=compile_thread, daemon=True)
        compile_thread.start()
        
        # 设置超时检查，防止线程卡死
        def check_timeout():
            if not compilation_completed.is_set():
                self.add_log("编译线程可能卡死，尝试强制结束...", "WARNING")
                self.hide_progress()
                self.set_status("编译超时")
                messagebox.showwarning("编译超时", "编译线程可能卡死，请检查编译环境或重启工具。")
            else:
                # 清理超时检查
                if hasattr(self, '_timeout_check_id'):
                    self.root.after_cancel(self._timeout_check_id)
        
        # 设置30分钟超时检查
        self._timeout_check_id = self.root.after(1800000, check_timeout)  # 30分钟
        
        # 等待编译完成
        def wait_for_completion():
            if compilation_completed.is_set():
                # 清理超时检查
                if hasattr(self, '_timeout_check_id'):
                    self.root.after_cancel(self._timeout_check_id)
                return
            
            # 继续等待
            self.root.after(100, wait_for_completion)
        
        wait_for_completion()
    

    def _create_log_directory(self):
        """创建日志目录"""
        if not self.project_path:
            raise ValueError("项目路径未设置")
        
        # 创建日志目录结构 - 使用更可靠的路径
        try:
            # 首先尝试在项目目录下创建
            log_dir = self.project_path / "Saved" / "Logs" / "UtilityExtend" / "SVGRegisterTool"
            log_dir.mkdir(parents=True, exist_ok=True)
            
            # 测试写入权限
            test_file = log_dir / "test_write.tmp"
            try:
                with open(test_file, 'w') as f:
                    f.write("test")
                test_file.unlink()  # 删除测试文件
                self.add_log(f"日志目录创建成功: {log_dir}")
                return log_dir
            except Exception as e:
                self.add_log(f"警告: 项目目录下无法写入日志: {e}")
                
        except Exception as e:
            self.add_log(f"警告: 无法在项目目录下创建日志目录: {e}")
        
        # 如果项目目录下无法创建，尝试在插件目录下创建
        try:
            log_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Logs"
            log_dir.mkdir(parents=True, exist_ok=True)
            
            # 测试写入权限
            test_file = log_dir / "test_write.tmp"
            try:
                with open(test_file, 'w') as f:
                    f.write("test")
                test_file.unlink()  # 删除测试文件
                self.add_log(f"日志目录创建成功（备用位置）: {log_dir}")
                return log_dir
            except Exception as e:
                self.add_log(f"警告: 插件目录下无法写入日志: {e}")
                
        except Exception as e:
            self.add_log(f"警告: 无法在插件目录下创建日志目录: {e}")
        
        # 最后尝试在当前工作目录下创建
        try:
            log_dir = Path.cwd() / "SVGRegisterTool_Logs"
            log_dir.mkdir(parents=True, exist_ok=True)
            self.add_log(f"日志目录创建成功（当前工作目录）: {log_dir}")
            return log_dir
            
        except Exception as e:
            self.add_log(f"错误: 无法创建任何日志目录: {e}")
            raise ValueError(f"无法创建日志目录: {e}")
    
    def _save_log_to_file(self, log_file_path, log_content):
        """保存日志到文件"""
        try:
            # 确保日志目录存在
            log_file_path.parent.mkdir(parents=True, exist_ok=True)
            
            # 尝试保存到指定路径
            with open(log_file_path, 'w', encoding='utf-8') as f:
                for log_entry in log_content:
                    f.write(log_entry + "\n")
            
            self.add_log(f"日志已保存到: {log_file_path}")
            
        except Exception as e:
            self.add_log(f"保存日志到指定路径失败: {e}")
            
            # 尝试备用保存位置
            try:
                backup_log_path = Path.cwd() / "SVGRegisterTool_Logs" / f"compilation_backup_{int(time.time())}.log"
                backup_log_path.parent.mkdir(parents=True, exist_ok=True)
                
                with open(backup_log_path, 'w', encoding='utf-8') as f:
                    for log_entry in log_content:
                        f.write(log_entry + "\n")
                
                self.add_log(f"日志已保存到备用位置: {backup_log_path}")
                
            except Exception as backup_e:
                self.add_log(f"备用日志保存也失败: {backup_e}")
                # 最后尝试在当前目录保存
                try:
                    final_log_path = Path.cwd() / f"compilation_emergency_{int(time.time())}.log"
                    with open(final_log_path, 'w', encoding='utf-8') as f:
                        for log_entry in log_content:
                            f.write(log_entry + "\n")
                    self.add_log(f"日志已保存到紧急位置: {final_log_path}")
                except Exception as final_e:
                    self.add_log(f"所有日志保存方法都失败: {final_e}")
                    raise Exception(f"无法保存日志文件: {e}")
    
    def _show_compilation_output(self, stdout, stderr):
        """显示编译输出"""
        self.add_log("显示编译输出对话框...")
        
        output_dialog = tk.Toplevel(self.root)
        output_dialog.title("编译输出")
        output_dialog.geometry("1000x700")
        output_dialog.transient(self.root)
        
        # 居中显示对话框
        self.center_dialog(output_dialog)
        
        # 主框架
        main_frame = ttk.Frame(output_dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 标题已移除
        
        # 创建Notebook用于分页显示
        notebook = ttk.Notebook(main_frame)
        notebook.pack(fill=tk.BOTH, expand=True)
        
        # 日志页面
        log_frame = ttk.Frame(notebook)
        notebook.add(log_frame, text="完整日志")
        
        # 日志文本框和滚动条
        log_text = tk.Text(log_frame, wrap=tk.NONE, font=("Consolas", 9))
        log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        log_scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=log_text.yview)
        log_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        log_h_scrollbar = ttk.Scrollbar(log_frame, orient=tk.HORIZONTAL, command=log_text.xview)
        log_h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        
        log_text.configure(yscrollcommand=log_scrollbar.set, xscrollcommand=log_h_scrollbar.set)
        
        # 显示日志内容
        log_content = self._get_log_content()
        if log_content:
            for log_entry in log_content:
                log_text.insert(tk.END, log_entry + "\n")
            self.add_log(f"显示日志内容，共 {len(log_content)} 行")
        else:
            log_text.insert(tk.END, "暂无日志内容\n")
        
        # 编译输出页面
        output_frame = ttk.Frame(notebook)
        notebook.add(output_frame, text="编译输出")
        
        # 编译输出文本框和滚动条
        output_text = tk.Text(output_frame, wrap=tk.NONE, font=("Consolas", 9))
        output_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        output_scrollbar = ttk.Scrollbar(output_frame, orient=tk.VERTICAL, command=output_text.yview)
        output_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        output_h_scrollbar = ttk.Scrollbar(output_frame, orient=tk.HORIZONTAL, command=output_text.xview)
        output_h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        
        output_text.configure(yscrollcommand=output_scrollbar.set, xscrollcommand=output_h_scrollbar.set)
        
        # 显示编译输出
        if stdout:
            output_text.insert(tk.END, "=== 标准输出 ===\n")
            output_text.insert(tk.END, stdout)
            self.add_log(f"显示标准输出，共 {len(stdout)} 字符")
        
        if stderr:
            output_text.insert(tk.END, "\n\n=== 错误输出 ===\n")
            output_text.insert(tk.END, stderr)
            self.add_log(f"显示错误输出，共 {len(stderr)} 字符")
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(20, 0))
        
        # 关闭按钮
        close_button = ttk.Button(button_frame, text="关闭", command=output_dialog.destroy)
        close_button.pack(side=tk.RIGHT)
    
    def view_compilation_logs(self):
        """查看编译日志"""
        self.add_log("查看编译日志...")
        
        if not self.project_path:
            error_msg = "未找到项目路径，请在配置菜单中设置路径"
            self.add_log(f"错误: {error_msg}")
            messagebox.showerror("错误", error_msg)
            return
        
        log_dir = self.project_path / "Saved" / "Logs" / "UtilityExtend" / "SVGRegisterTool"
        if not log_dir.exists():
            self.add_log("没有找到编译日志目录")
            messagebox.showinfo("信息", "没有找到编译日志文件。")
            return
        
        # 获取最新的编译日志文件
        log_files = sorted(log_dir.glob("compilation_*.log"), key=lambda f: f.stat().st_mtime, reverse=True)
        
        if not log_files:
            self.add_log("没有找到编译日志文件")
            messagebox.showinfo("信息", "没有找到编译日志文件。")
            return
        
        latest_log_file = log_files[0]
        self.add_log(f"找到最新日志文件: {latest_log_file.name}")
        
        # 显示日志内容
        self._show_file_content(latest_log_file, latest_log_file.name)

    def _show_file_content(self, file_path, file_name):
        """显示文件内容"""
        self.add_log(f"显示文件内容: {file_name}")
        
        content_dialog = tk.Toplevel(self.root)
        content_dialog.title(f"文件内容 - {file_name}")
        content_dialog.geometry("800x600")
        content_dialog.transient(self.root)
        
        # 居中显示对话框
        self.center_dialog(content_dialog)
        
        # 内容框架
        content_frame = ttk.Frame(content_dialog, padding="10")
        content_frame.pack(fill=tk.BOTH, expand=True)
        
        # 文件路径标签
        path_label = ttk.Label(content_frame, text=f"文件路径: {file_path}", font=("Arial", 9))
        path_label.pack(anchor=tk.W, pady=(0, 10))
        
        # 文本内容
        text_widget = tk.Text(content_frame, wrap=tk.NONE, font=("Consolas", 10))
        text_widget.pack(fill=tk.BOTH, expand=True)
        
        # 滚动条
        v_scrollbar = ttk.Scrollbar(content_frame, orient=tk.VERTICAL, command=text_widget.yview)
        v_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        h_scrollbar = ttk.Scrollbar(content_frame, orient=tk.HORIZONTAL, command=text_widget.xview)
        h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        
        text_widget.configure(yscrollcommand=v_scrollbar.set, xscrollcommand=h_scrollbar.set)
        
        try:
            # 读取文件内容
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            text_widget.insert(1.0, content)
            self.add_log(f"文件内容读取成功，共 {len(content)} 字符")
        except Exception as e:
            error_msg = f"无法读取文件内容: {str(e)}"
            self.add_log(f"错误: {error_msg}")
            text_widget.insert(1.0, error_msg)
        
        # 关闭按钮
        close_button = ttk.Button(content_frame, text="关闭", command=content_dialog.destroy)
        close_button.pack(pady=(10, 0))

    def add_log(self, message, level="INFO"):
        """向日志区域添加消息"""
        # 启用文本框进行编辑
        self.log_text.config(state=tk.NORMAL)
        
        # 添加时间戳和级别
        timestamp = time.strftime("%H:%M:%S")
        
        # 根据级别设置不同的前缀和颜色
        if level == "ERROR":
            log_message = f"[{timestamp}] [错误] {message}\n"
            # 可以在这里添加错误级别的特殊处理
        elif level == "WARNING":
            log_message = f"[{timestamp}] [警告] {message}\n"
            # 可以在这里添加警告级别的特殊处理
        elif level == "SUCCESS":
            log_message = f"[{timestamp}] [成功] {message}\n"
            # 可以在这里添加成功级别的特殊处理
        else:
            log_message = f"[{timestamp}] {message}\n"
        
        # 插入消息
        self.log_text.insert(tk.END, log_message)
        
        # 滚动到底部
        self.log_text.see(tk.END)
        
        # 禁用文本框
        self.log_text.config(state=tk.DISABLED)
        
        # 限制日志行数，避免内存占用过大
        lines = self.log_text.get(1.0, tk.END).split('\n')
        if len(lines) > 1000:  # 保留最近1000行
            self.log_text.config(state=tk.NORMAL)
            self.log_text.delete(1.0, f"{len(lines)-1000}.0")
            self.log_text.config(state=tk.DISABLED)
        
        # 强制更新UI，确保日志实时显示
        self.root.update_idletasks()
    
    def clear_log(self):
        """清空日志区域"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        
    def on_window_resize(self, event):
        """处理窗口大小改变事件"""
        # 更新列宽以适应新的窗口大小
        if hasattr(self, 'svg_tree') and hasattr(self, 'status_tree'):
            # 获取当前窗口宽度
            window_width = self.root.winfo_width()
            
            # 调整左侧面板宽度（保持比例）
            left_width = max(300, int(window_width * 0.3))  # 左侧占30%，最小300px
            right_width = window_width - left_width - 20  # 右侧占剩余空间，减去边距
            
            # 更新列宽
            if left_width > 300:
                self.svg_tree.column("name", width=int(left_width * 0.4))
                self.svg_tree.column("path", width=int(left_width * 0.6))
            
            if right_width > 400:
                self.status_tree.column("details", width=int(right_width * 0.4))
                self.status_tree.column("friendly_name", width=int(right_width * 0.4))
        
    def on_tree_motion(self, event):
        """处理树形控件的鼠标移动事件"""
        # 获取鼠标位置下的项目
        tree = event.widget
        item = tree.identify_row(event.y)
        
        if item:
            # 获取项目信息
            values = tree.item(item, "values")
            if values:
                # 根据不同的树形控件生成不同的工具提示
                if tree == self.svg_tree:
                    tooltip_text = f"文件名: {values[0]}\n路径: {values[1]}"
                elif tree == self.status_tree:
                    tooltip_text = f"状态: {values[0]}\n详情: {values[1]}\n友好名: {values[2]}"
                else:
                    tooltip_text = ""
                
                # 如果工具提示内容改变，更新显示
                if tooltip_text != self.tooltip_text:
                    self.tooltip_text = tooltip_text
                    self.show_tooltip(event, tooltip_text)
        else:
            # 鼠标不在项目上，隐藏工具提示
            self.hide_tooltip()
            
    def on_tree_leave(self, event):
        """处理树形控件的离开事件"""
        self.hide_tooltip()
        
    def show_tooltip(self, event, text):
        """显示工具提示"""
        self.hide_tooltip()
        
        # 创建工具提示窗口
        self.tooltip = tk.Toplevel()
        self.tooltip.wm_overrideredirect(True)
        self.tooltip.wm_geometry(f"+{event.x_root+10}+{event.y_root+10}")
        
        # 设置工具提示样式
        self.tooltip.configure(bg='lightyellow', relief='solid', borderwidth=1)
        
        # 创建标签
        label = tk.Label(self.tooltip, text=text, justify=tk.LEFT,
                        background="lightyellow", relief="solid", borderwidth=1,
                        font=("Tahoma", "8", "normal"))
        label.pack()
        
        # 设置工具提示自动隐藏
        self.tooltip.after(3000, self.hide_tooltip)
        
    def hide_tooltip(self):
        """隐藏工具提示"""
        if self.tooltip:
            self.tooltip.destroy()
            self.tooltip = None
            self.tooltip_text = ""
    
    def bind_shortcuts(self):
        """绑定键盘快捷键"""
        # Ctrl+R: 重新扫描
        self.root.bind('<Control-r>', lambda e: self.refresh_scan())
        # Ctrl+Shift+R: 重新注册图集
        self.root.bind('<Control-Shift-R>', lambda e: self.reregister_icons())
        # Ctrl+I: 导入SVG
        self.root.bind('<Control-i>', lambda e: self.import_svg())
        # Ctrl+Shift+C: 重新编译
        self.root.bind('<Control-Shift-c>', lambda e: self.recompile_project())
        # Ctrl+G: 生成报告
        self.root.bind('<Control-g>', lambda e: self.generate_report())
        # Ctrl+Shift+O: 打开代码文件
        self.root.bind('<Control-Shift-o>', lambda e: self.open_code_files())
        # Ctrl+L: 清空日志
        self.root.bind('<Control-l>', lambda e: self.clear_log())
        # Ctrl+Q: 退出编辑器
        self.root.bind('<Control-q>', lambda e: self.root.quit())
        
        # 为树形视图添加快捷键
        self.svg_tree.bind('<Control-a>', lambda e: self.select_all_svg())
        self.status_tree.bind('<Control-a>', lambda e: self.select_all_status())
        
        # 删除键删除选中的SVG文件
        self.svg_tree.bind('<Delete>', lambda e: self.delete_selected_svg())
        
        # 回车键查看详情
        self.svg_tree.bind('<Return>', lambda e: self.show_svg_details())
        self.status_tree.bind('<Return>', lambda e: self.show_status_details())
    
    def select_all_svg(self):
        """选择所有SVG文件"""
        for item in self.svg_tree.get_children():
            self.svg_tree.selection_add(item)
    
    def select_all_status(self):
        """选择所有状态项"""
        for item in self.status_tree.get_children():
            self.status_tree.selection_add(item)
    
    def delete_selected_svg(self):
        """删除选中的SVG文件"""
        selected = self.svg_tree.selection()
        if not selected:
            return
        
        # 获取选中的文件信息
        file_info = []
        for item in selected:
            values = self.svg_tree.item(item, "values")
            if values:
                file_info.append((item, values[0], values[1]))
        
        if not file_info:
            return
        
        # 确认删除
        if len(file_info) == 1:
            message = f"确定要删除SVG文件 '{file_info[0][1]}' 吗？"
        else:
            message = f"确定要删除选中的 {len(file_info)} 个SVG文件吗？"
        
        if messagebox.askyesno("确认删除", message):
            for item, name, path in file_info:
                try:
                    # 从文件系统中删除
                    if os.path.exists(path):
                        os.remove(path)
                        self.add_log(f"已删除文件: {name}")
                    
                    # 从树形视图中删除
                    self.svg_tree.delete(item)
                    
                    # 从注册状态中删除
                    self.remove_from_status_tree(name)
                    
                except Exception as e:
                    self.add_log(f"删除文件失败 {name}: {str(e)}")
            
            # 刷新状态
            self.analyze_registrations()
            self.update_status_bar()
    
    def remove_from_status_tree(self, svg_name):
        """从状态树中移除指定的SVG项"""
        for item in self.status_tree.get_children():
            values = self.status_tree.item(item, "values")
            if values and values[1] == svg_name:
                self.status_tree.delete(item)
                break
    
    def show_svg_details(self):
        """显示SVG文件详情"""
        selected = self.svg_tree.selection()
        if not selected:
            return
        
        item = selected[0]
        values = self.svg_tree.item(item, "values")
        if values:
            name, path = values[0], values[1]
            details = f"文件名: {name}\n路径: {path}"
            
            # 检查文件是否存在
            if os.path.exists(path):
                file_size = os.path.getsize(path)
                details += f"\n文件大小: {file_size} 字节"
                
                # 尝试读取SVG内容
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        content = f.read()
                        # 提取SVG的viewBox信息
                        import re
                        viewbox_match = re.search(r'viewBox=["\']([^"\']+)["\']', content)
                        if viewbox_match:
                            details += f"\nViewBox: {viewbox_match.group(1)}"
                        
                        # 提取SVG的尺寸信息
                        width_match = re.search(r'width=["\']([^"\']+)["\']', content)
                        height_match = re.search(r'height=["\']([^"\']+)["\']', content)
                        if width_match and height_match:
                            details += f"\n尺寸: {width_match.group(1)} x {height_match.group(1)}"
                except Exception as e:
                    details += f"\n读取文件失败: {str(e)}"
            else:
                details += "\n文件不存在"
            
            messagebox.showinfo("SVG文件详情", details)
    
    def show_status_details(self):
        """显示状态项详情"""
        selected = self.status_tree.selection()
        if not selected:
            return
        
        item = selected[0]
        values = self.status_tree.item(item, "values")
        if values:
            status, details, friendly_name = values[0], values[1], values[2]
            details_text = f"状态: {status}\n详情: {details}\n友好名: {friendly_name}"
            
            # 根据状态提供更多信息
            if status == "已注册":
                details_text += "\n\n该图标已在代码中正确注册"
            elif status == "未注册":
                details_text += "\n\n该图标尚未在代码中注册，需要添加到代码中"
            elif status == "友好名不匹配":
                details_text += "\n\n该图标的友好名称与代码中的设置不匹配"
            elif status == "文件不存在":
                details_text += "\n\n该图标文件已不存在，需要从代码中移除"
            
            messagebox.showinfo("状态详情", details_text)

    def _get_log_content(self):
        """获取当前日志内容"""
        try:
            # 从日志文本框获取内容
            content = self.log_text.get(1.0, tk.END).strip()
            if content:
                return content.split('\n')
            return []
        except Exception as e:
            self.add_log(f"获取日志内容失败: {e}")
            return []

    def show_current_registrations(self):
        """显示当前已注册的图标信息"""
        self.add_log("显示当前已注册的图标信息...")
        
        # 读取样式文件
        style_file = self.source_path / "Private" / "UtilityExtendStyle.cpp"
        registry_file = self.source_path / "Private" / "UtilityExtendIconRegistry.cpp"
        
        registered_icons = []
        
        if style_file.exists():
            try:
                with open(style_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                # 查找用户自定义区域
                user_custom_section = "// 用户自定义区域"
                if user_custom_section in content:
                    section_start = content.find(user_custom_section)
                    section_end = content.find("// ============================================================================", section_start)
                    
                    if section_start != -1 and section_end != -1:
                        section_content = content[section_start:section_end]
                        
                        # 查找所有图标注册
                        import re
                        pattern = r'Style->Set\("UtilityExtend\.([^"]+)", new IMAGE_BRUSH_SVG\(TEXT\("([^"]+)"\), FVector2D\([^)]+\)\)\);'
                        matches = re.findall(pattern, section_content)
                        
                        for icon_name, svg_name in matches:
                            registered_icons.append({
                                'icon_name': f"UtilityExtend.{icon_name}",
                                'svg_name': svg_name,
                                'type': 'Style'
                            })
            except Exception as e:
                self.add_log(f"读取样式文件时出错: {e}")
        
        if registry_file.exists():
            try:
                with open(registry_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                # 查找用户自定义区域
                user_custom_section = "// 用户自定义区域"
                if user_custom_section in content:
                    section_start = content.find(user_custom_section)
                    section_end = content.find("// ============================================================================", section_start)
                    
                    if section_start != -1 and section_end != -1:
                        section_content = content[section_start:section_end]
                        
                        # 查找所有图标注册
                        import re
                        pattern = r'IconInfos\.Add\(FToolbarIconInfo\(TEXT\("([^"]+)"\), TEXT\("([^"]+)"\), TEXT\("([^"]+)"\)\);'
                        matches = re.findall(pattern, section_content)
                        
                        for icon_name, svg_name, description in matches:
                            # 检查是否已经在样式列表中
                            existing = next((icon for icon in registered_icons if icon['icon_name'] == icon_name), None)
                            if existing:
                                existing['type'] = 'Style+Registry'
                            else:
                                registered_icons.append({
                                    'icon_name': icon_name,
                                    'svg_name': svg_name,
                                    'type': 'Registry'
                                })
            except Exception as e:
                self.add_log(f"读取注册文件时出错: {e}")
        
        if registered_icons:
            # 创建显示窗口
            info_window = tk.Toplevel(self.root)
            info_window.title("当前已注册的图标")
            info_window.geometry("800x600")
            info_window.transient(self.root)
            
            # 居中显示对话框
            self.center_dialog(info_window)
            
            # 主框架
            main_frame = ttk.Frame(info_window, padding="20")
            main_frame.pack(fill=tk.BOTH, expand=True)
            
            # 标题已移除
            
            # 创建树形视图
            tree_frame = ttk.Frame(main_frame)
            tree_frame.pack(fill=tk.BOTH, expand=True)
            
            tree = ttk.Treeview(tree_frame, columns=("icon_name", "svg_name", "type"), show="headings", height=20)
            tree.heading("icon_name", text="图标名称")
            tree.heading("svg_name", text="SVG文件名")
            tree.heading("type", text="注册类型")
            
            tree.column("icon_name", width=250)
            tree.column("svg_name", width=200)
            tree.column("type", width=150)
            
            # 添加滚动条
            scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=tree.yview)
            tree.configure(yscrollcommand=scrollbar.set)
            
            tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            
            # 填充数据
            for icon in registered_icons:
                tree.insert("", tk.END, values=(icon['icon_name'], icon['svg_name'], icon['type']))
            
            # 按钮框架
            button_frame = ttk.Frame(main_frame)
            button_frame.pack(fill=tk.X, pady=(20, 0))
            
            # 关闭按钮
            close_button = ttk.Button(button_frame, text="关闭", command=info_window.destroy)
            close_button.pack(side=tk.RIGHT)
            
            self.add_log(f"显示当前已注册的图标信息，共 {len(registered_icons)} 个图标")
        else:
            self.add_log("未找到任何已注册的图标")
            messagebox.showinfo("信息", "未找到任何已注册的图标。")

    def show_backups(self):
        """显示备份管理窗口"""
        backup_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Backups"
        if not backup_dir.exists():
            messagebox.showinfo("信息", "暂无备份文件。")
            return
        
        # 创建备份管理窗口
        backup_window = tk.Toplevel(self.root)
        backup_window.title("备份管理")
        backup_window.geometry("1000x700")
        backup_window.transient(self.root)
        
        # 居中显示对话框
        self.center_dialog(backup_window)
        
        # 主框架
        main_frame = ttk.Frame(backup_window, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 标题已移除
        
        # 创建备份列表
        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        # 创建树形视图
        tree = ttk.Treeview(list_frame, columns=("timestamp", "description", "files", "actions"), show="headings", height=20)
        tree.heading("timestamp", text="备份时间")
        tree.heading("description", text="备份描述")
        tree.heading("files", text="备份文件")
        tree.heading("actions", text="操作")
        
        tree.column("timestamp", width=180)
        tree.column("description", width=200)
        tree.column("files", width=300)
        tree.column("actions", width=200)
        
        # 添加滚动条
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scrollbar.set)
        
        tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # 加载备份列表
        self.load_backup_list(tree)
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(20, 0))
        
        # 刷新按钮
        refresh_button = ttk.Button(button_frame, text="刷新列表", command=lambda: self.load_backup_list(tree))
        refresh_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 创建新备份按钮
        create_backup_button = ttk.Button(button_frame, text="创建新备份", command=lambda: self.create_manual_backup(backup_window, tree))
        create_backup_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 关闭按钮
        close_button = ttk.Button(button_frame, text="关闭", command=backup_window.destroy)
        close_button.pack(side=tk.RIGHT)
        
        self.add_log("打开备份管理窗口")
    
    def load_backup_list(self, tree):
        """加载备份列表到树形视图"""
        # 清空现有项目
        for item in tree.get_children():
            tree.delete(item)
        
        backup_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Backups"
        if not backup_dir.exists():
            return
        
        # 查找所有备份信息文件
        backup_info_files = list(backup_dir.glob("*_info.json"))
        backup_info_files.sort(key=lambda x: x.name, reverse=True)  # 按名称倒序排列（最新的在前）
        
        for info_file in backup_info_files:
            try:
                with open(info_file, 'r', encoding='utf-8') as f:
                    backup_info = json.load(f)
                
                # 格式化时间戳
                timestamp = backup_info.get('timestamp', '')
                if timestamp:
                    try:
                        # 将时间戳转换为可读格式
                        time_obj = time.strptime(timestamp, "%Y%m%d_%H%M%S")
                        formatted_time = time.strftime("%Y-%m-%d %H:%M:%S", time_obj)
                    except:
                        formatted_time = timestamp
                else:
                    formatted_time = "未知时间"
                
                description = backup_info.get('description', '自动备份')
                files_count = len(backup_info.get('files', []))
                files_text = f"{files_count} 个文件"
                
                # 插入到树形视图
                item = tree.insert("", tk.END, values=(formatted_time, description, files_text, "查看/回退"))
                
                # 绑定双击事件
                tree.bind("<Double-1>", lambda e, info=backup_info: self.show_backup_details(info))
                
            except Exception as e:
                self.add_log(f"读取备份信息文件失败: {e}")
    
    def show_backup_details(self, backup_info):
        """显示备份详细信息并提供回退选项"""
        # 创建详细信息窗口
        detail_window = tk.Toplevel(self.root)
        detail_window.title("备份详细信息")
        detail_window.geometry("800x600")
        detail_window.transient(self.root)
        
        # 居中显示对话框
        self.center_dialog(detail_window)
        
        # 主框架
        main_frame = ttk.Frame(detail_window, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 标题已移除
        
        # 备份信息
        info_frame = ttk.LabelFrame(main_frame, text="备份信息", padding="10")
        info_frame.pack(fill=tk.X, pady=(0, 20))
        
        timestamp = backup_info.get('timestamp', '')
        if timestamp:
            try:
                time_obj = time.strptime(timestamp, "%Y%m%d_%H%M%S")
                formatted_time = time.strftime("%Y-%m-%d %H:%M:%S", time_obj)
            except:
                formatted_time = timestamp
        else:
            formatted_time = "未知时间"
        
        ttk.Label(info_frame, text=f"备份时间: {formatted_time}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"备份描述: {backup_info.get('description', '自动备份')}").pack(anchor=tk.W)
        ttk.Label(info_frame, text=f"备份名称: {backup_info.get('backup_name', '未知')}").pack(anchor=tk.W)
        
        # 备份文件列表
        files_frame = ttk.LabelFrame(main_frame, text="备份文件", padding="10")
        files_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 20))
        
        # 创建文件列表
        files_tree = ttk.Treeview(files_frame, columns=("original", "backup"), show="headings", height=10)
        files_tree.heading("original", text="原始文件")
        files_tree.heading("backup", text="备份文件")
        
        files_tree.column("original", width=350)
        files_tree.column("backup", width=350)
        
        # 添加滚动条
        files_scrollbar = ttk.Scrollbar(files_frame, orient=tk.VERTICAL, command=files_tree.yview)
        files_tree.configure(yscrollcommand=files_scrollbar.set)
        
        files_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        files_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # 填充文件列表
        for file_info in backup_info.get('files', []):
            original = Path(file_info['original']).name
            backup = Path(file_info['backup']).name
            files_tree.insert("", tk.END, values=(original, backup))
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(20, 0))
        
        # 回退按钮
        restore_button = ttk.Button(button_frame, text="回退到此备份", 
                                  command=lambda: self.restore_backup(backup_info, detail_window))
        restore_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 删除备份按钮
        delete_button = ttk.Button(button_frame, text="删除此备份", 
                                 command=lambda: self.delete_backup(backup_info, detail_window))
        delete_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 关闭按钮
        close_button = ttk.Button(button_frame, text="关闭", command=detail_window.destroy)
        close_button.pack(side=tk.RIGHT)
    
    def restore_backup(self, backup_info, detail_window):
        """回退到指定备份"""
        if not messagebox.askyesno("确认回退", 
                                  f"确定要回退到备份 '{backup_info.get('description', '自动备份')}' 吗？\n\n"
                                  "这将覆盖当前的文件内容，请确保您已经保存了重要的更改。"):
            return
        
        try:
            # 在回退之前先创建当前状态的备份
            current_backup_name = self.backup_files("回退前的自动备份")
            self.add_log(f"已创建当前状态备份: {current_backup_name}")
            
            # 执行回退
            backup_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Backups"
            files_restored = 0
            
            for file_info in backup_info.get('files', []):
                backup_path = Path(file_info['backup'])
                original_path = Path(file_info['original'])
                
                if backup_path.exists() and original_path.parent.exists():
                    shutil.copy2(backup_path, original_path)
                    files_restored += 1
                    self.add_log(f"已回退文件: {original_path}")
                else:
                    self.add_log(f"警告: 无法回退文件 {original_path}")
            
            if files_restored > 0:
                messagebox.showinfo("回退完成", 
                                  f"已成功回退 {files_restored} 个文件到备份状态。\n\n"
                                  "请重新编译项目以应用更改。")
                detail_window.destroy()
                self.add_log(f"备份回退完成，共回退 {files_restored} 个文件")
            else:
                messagebox.showerror("回退失败", "没有文件被回退，请检查备份文件是否存在。")
                
        except Exception as e:
            messagebox.showerror("回退失败", f"回退过程中发生错误:\n{str(e)}")
            self.add_log(f"备份回退失败: {e}")
    
    def delete_backup(self, backup_info, detail_window):
        """删除指定备份"""
        if not messagebox.askyesno("确认删除", 
                                  f"确定要删除备份 '{backup_info.get('description', '自动备份')}' 吗？\n\n"
                                  "此操作不可撤销！"):
            return
        
        try:
            backup_dir = self.plugin_path / "Third" / "SVGRegisterTool" / "Backups"
            backup_name = backup_info.get('backup_name', '')
            
            # 删除备份文件
            files_deleted = 0
            for file_info in backup_info.get('files', []):
                backup_path = Path(file_info['backup'])
                if backup_path.exists():
                    backup_path.unlink()
                    files_deleted += 1
            
            # 删除信息文件
            info_file = backup_dir / f"{backup_name}_info.json"
            if info_file.exists():
                info_file.unlink()
            
            self.add_log(f"已删除备份: {backup_name}，共删除 {files_deleted} 个文件")
            messagebox.showinfo("删除完成", f"备份已删除。")
            detail_window.destroy()
            
        except Exception as e:
            messagebox.showerror("删除失败", f"删除备份时发生错误:\n{str(e)}")
            self.add_log(f"删除备份失败: {e}")
    
    def create_manual_backup(self, backup_window, tree):
        """手动创建新备份"""
        # 创建输入对话框
        input_dialog = tk.Toplevel(backup_window)
        input_dialog.title("创建新备份")
        input_dialog.geometry("400x200")
        input_dialog.transient(backup_window)
        input_dialog.grab_set()  # 模态对话框
        
        # 主框架
        main_frame = ttk.Frame(input_dialog, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 描述输入
        ttk.Label(main_frame, text="备份描述:").pack(anchor=tk.W, pady=(0, 5))
        description_entry = ttk.Entry(main_frame, width=50)
        description_entry.pack(fill=tk.X, pady=(0, 20))
        description_entry.insert(0, "手动备份")
        description_entry.focus()
        
        # 按钮框架
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(20, 0))
        
        def create_backup():
            description = description_entry.get().strip()
            if not description:
                description = "手动备份"
            
            try:
                backup_name = self.backup_files(description)
                self.add_log(f"手动创建备份成功: {backup_name}")
                messagebox.showinfo("备份完成", f"备份已创建: {backup_name}")
                
                # 刷新备份列表
                self.load_backup_list(tree)
                
                input_dialog.destroy()
                
            except Exception as e:
                messagebox.showerror("备份失败", f"创建备份时发生错误:\n{str(e)}")
                self.add_log(f"手动创建备份失败: {e}")
        
        # 创建按钮
        create_button = ttk.Button(button_frame, text="创建备份", command=create_backup)
        create_button.pack(side=tk.LEFT, padx=(0, 10))
        
        # 取消按钮
        cancel_button = ttk.Button(button_frame, text="取消", command=input_dialog.destroy)
        cancel_button.pack(side=tk.RIGHT)
        
        # 绑定回车键
        input_dialog.bind("<Return>", lambda e: create_backup())
    
    def toggle_file_monitoring(self):
        """切换文件监控状态"""
        if self.file_monitoring_enabled:
            self.stop_file_monitoring()
        else:
            self.start_file_monitoring()
            
        # 更新菜单项文本
        self.update_monitoring_menu_text()
        
    def start_file_monitoring(self):
        """开始文件监控"""
        if not self.file_monitoring_enabled:
            self.file_monitoring_enabled = True
            self.add_log("开始自动监控文件变化...")
            self.set_status("文件监控已启用")
            # 更新菜单项文本
            self.root.after(100, self.check_files_changed)
            
    def stop_file_monitoring(self):
        """停止文件监控"""
        if self.file_monitoring_enabled:
            self.file_monitoring_enabled = False
            self.add_log("停止自动监控文件变化")
            self.set_status("文件监控已禁用")
            if self.monitoring_timer:
                self.root.after_cancel(self.monitoring_timer)
                self.monitoring_timer = None
                
    def check_files_changed(self):
        """检查文件是否发生变化"""
        if not self.file_monitoring_enabled:
            return
            
        try:
            # 检查Resources目录中的SVG文件
            resources_dir = self.plugin_path / "Resources"
            if resources_dir.exists():
                current_files = set()
                for svg_file in resources_dir.glob("*.svg"):
                    current_files.add(svg_file.name)
                    # 检查文件修改时间
                    stat = svg_file.stat()
                    if stat.st_mtime > self.last_scan_time:
                        self.add_log(f"检测到文件变化: {svg_file.name}")
                        self.refresh_scan()
                        break
                        
                # 检查是否有文件被删除
                if hasattr(self, 'svg_files') and self.svg_files:
                    existing_names = {info['name'] + '.svg' for info in self.svg_files}
                    if current_files != existing_names:
                        self.add_log("检测到SVG文件数量变化，自动刷新...")
                        self.refresh_scan()
                        
        except Exception as e:
            self.add_log(f"文件监控检查时发生错误: {str(e)}")
            
        # 设置下次检查
        if self.file_monitoring_enabled:
            self.monitoring_timer = self.root.after(self.file_check_interval, self.check_files_changed)
    
    def update_monitoring_menu_text(self):
        """更新监控菜单项文本"""
        try:
            # 获取文件菜单
            menubar = self.root.nametowidget(self.root.cget("menu"))
            file_menu = menubar.winfo_children()[0]  # 文件菜单通常是第一个
            
            # 查找监控菜单项并更新文本
            for i in range(file_menu.index('end') + 1):
                try:
                    label = file_menu.entrycget(i, 'label')
                    if '自动监控文件变化' in label:
                        new_text = "停止自动监控" if self.file_monitoring_enabled else "自动监控文件变化"
                        file_menu.entryconfig(i, label=new_text)
                        break
                except:
                    continue
        except Exception as e:
            self.add_log(f"更新监控菜单文本时发生错误: {str(e)}")

    def on_closing(self):
        # 停止文件监控
        self.stop_file_monitoring()
        # 清理资源
        self.root.destroy()



def main():
    app = SVGIconManager()
    app.root.mainloop()

if __name__ == "__main__":
    main()
