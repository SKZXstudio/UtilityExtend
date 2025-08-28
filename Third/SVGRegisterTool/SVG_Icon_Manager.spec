# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

a = Analysis(
    [r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\SVG_Icon_Manager.py'],
    pathex=[r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool'],
    binaries=[],
    datas=[
        (r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\Resources\icon.ico', '.'),
        (r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\Resources\icon.svg', '.'),
        (r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\README.md', '.'),
        (r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\使用说明.md', '.')
    ],
    hiddenimports=['psutil'],  # 添加psutil依赖
    hookspath=[],
    hooksconfig={},
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
    name=r'图标注册工具',  # 使用中文名称
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,  # 无控制台窗口
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\Resources\icon.ico',  # 使用本地图标
    distpath=r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\dist',
    workpath=r'F:\Project\UnrealEngine\NEXBox\Client\Plugins\UtilityExtend\Third\SVGRegisterTool\build',
)
