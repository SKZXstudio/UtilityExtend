# SVGRegisterTool Python环境

这个文件夹包含了SVGRegisterTool工具运行所需的完整Python环境。

## 环境信息

- **Python版本**: 3.13.5
- **环境类型**: 虚拟环境 (venv)
- **创建时间**: 2025年8月11日

## 目录结构

```
Python/
├── venv/                    # Python虚拟环境
│   ├── Scripts/            # Windows可执行文件
│   │   ├── python.exe      # Python解释器
│   │   ├── pip.exe         # 包管理器
│   │   └── activate.bat    # 激活脚本
│   ├── bin/                # Linux/macOS可执行文件
│   │   ├── python          # Python解释器
│   │   ├── pip             # 包管理器
│   │   └── activate         # 激活脚本
│   ├── Lib/                # Python库文件
│   └── Include/            # 头文件
├── requirements.txt         # 依赖包列表
└── README.md               # 本文件
```

## 使用方法

### Windows用户

1. **直接运行** (推荐):
   ```
   双击 run_tool_python.bat
   ```

2. **手动激活环境**:
   ```cmd
   Python\venv\Scripts\activate
   python SVG_Icon_Manager.py
   ```

### Linux/macOS用户

1. **直接运行** (推荐):
   ```bash
   chmod +x run_tool_python.sh
   ./run_tool_python.sh
   ```

2. **手动激活环境**:
   ```bash
   source Python/venv/bin/activate
   python SVG_Icon_Manager.py
   ```

## 优势

1. **无需安装Python**: 工具自带完整的Python环境
2. **版本兼容**: 确保使用正确的Python版本
3. **依赖完整**: 包含所有必要的Python包
4. **跨平台**: 支持Windows、Linux、macOS
5. **便携性**: 可以复制到其他机器上使用

## 注意事项

1. **不要删除**: 请勿删除Python文件夹，否则工具无法运行
2. **路径要求**: 工具必须在SVGRegisterTool目录中运行
3. **权限要求**: 确保有足够的文件读写权限
4. **网络要求**: 首次运行可能需要网络连接来下载依赖

## 故障排除

### 问题1: 找不到Python环境
**解决方案**: 确保Python文件夹完整，包含venv子文件夹

### 问题2: 权限不足
**解决方案**: 以管理员身份运行，或检查文件夹权限

### 问题3: 依赖缺失
**解决方案**: 重新运行 `pip install -r requirements.txt`

## 更新Python环境

如果需要更新Python环境：

1. 删除现有的venv文件夹
2. 重新运行 `python -m venv Python/venv`
3. 激活环境并安装依赖

## 技术支持

如果遇到问题，请检查：
1. Python文件夹是否完整
2. 运行脚本是否在正确的目录中
3. 系统是否有足够的权限
4. 网络连接是否正常
