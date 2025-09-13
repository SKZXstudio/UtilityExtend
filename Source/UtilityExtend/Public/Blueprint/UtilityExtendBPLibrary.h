// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UtilityExtendBPLibrary.generated.h"

// 前向声明
class UUtilityLoadingNotification;

/**
 * 编辑器通知类型枚举
 */
UENUM(BlueprintType)
enum class EEditorNotificationType : uint8
{
    /** 默认通知 */
    Default         UMETA(DisplayName = "默认"),
    /** 成功通知 */
    Success         UMETA(DisplayName = "成功"),
    /** 错误通知 */
    Error           UMETA(DisplayName = "错误")
};

/**
 * 通知按钮点击事件委托
 * @param NotificationId 通知的唯一ID
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotificationButtonClicked, const FString&, NotificationId);

/**
 * 通知按钮点击事件接口
 */
UINTERFACE(BlueprintType)
class UNotificationButtonClickInterface : public UInterface
{
    GENERATED_BODY()
};

class INotificationButtonClickInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "UtilityExtend|Notification")
    void OnNotificationButtonClicked(const FString& NotificationId);
};

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class UUtilityExtendBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 跟踪创建的通知
    static TArray<TSharedPtr<SNotificationItem>> CreatedNotifications;
    
    // 通知按钮点击事件委托实例
    static FOnNotificationButtonClicked OnNotificationButtonClicked;
    
    // 通知ID映射表 (NotificationId -> NotificationItem)
    static TMap<FString, TSharedPtr<SNotificationItem>> NotificationMap;
    
    // 注册的事件接收者列表
    static TArray<TWeakObjectPtr<UObject>> NotificationEventReceivers;

    // ------------------------------------编辑器通知相关函数------------------------------------
    // 创建常规通知节点（不带按钮）
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "创建编辑器通知", 
        Keywords = "创建编辑器通知 显示通知", 
        Category = "UtilityExtend|编辑器通知"
    ))
    static UPARAM(DisplayName = "通知ID") FString CreateEditorNotification(
        UPARAM(DisplayName = "消息内容") const FString& Message,
        UPARAM(DisplayName = "通知类型") EEditorNotificationType NotificationType = EEditorNotificationType::Default,
        UPARAM(DisplayName = "持续时间") float Duration = 5.0f,
        UPARAM(DisplayName = "自动过期") bool bAutoExpire = true
    );

    // 创建带加载动画的通知节点（支持按钮）
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "创建编辑器等待通知", 
        Keywords = "创建等待通知 带按钮",
        Category = "UtilityExtend|编辑器通知"
    ))
    static UPARAM(DisplayName = "通知ID") FString CreateEditorLoadingNotification(
        UPARAM(DisplayName = "消息内容") const FString& Message,
        UPARAM(DisplayName = "通知对象") UUtilityLoadingNotification*& OutNotificationObject,
        UPARAM(DisplayName = "显示按钮") bool bShowButton = false,
        UPARAM(DisplayName = "按钮文本") const FString& ButtonText = TEXT("确定"),
        UPARAM(DisplayName = "按钮提示") const FString& ButtonTooltip = TEXT("")
    );

    // 创建复杂通知
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "创建复杂通知", 
        Keywords = "create complex notification",
        ToolTip = "创建一个支持多按钮、进度条和事件绑定的复杂通知对象",
        Category = "UtilityExtend|编辑器通知"
    ))
    static UPARAM(DisplayName = "通知返回对象") UUtilityLoadingNotification* CreateComplexNotification(
        UPARAM(DisplayName = "标题") const FString& Title,
        UPARAM(DisplayName = "内容") const FString& Text,
        UPARAM(DisplayName = "按钮文本数组") const TArray<FString>& ButtonTexts,
        UPARAM(DisplayName = "显示进度条") bool bShowProgressBar = true
    );

    // 清除通知
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "移除编辑器通知", 
        Keywords = "移除编辑器通知 清除通知 删除通知", 
        Category = "UtilityExtend|编辑器通知"
    ))
    static UPARAM(DisplayName = "移除成功") bool RemoveEditorNotification(
        UPARAM(DisplayName = "通知ID [ID为空则全部移除]") const FString& NotificationId = TEXT(""),
        UPARAM(DisplayName = "清除全部") bool bRemoveAll = false
    );

    // ------------------------------------插件相关函数------------------------------------
    // 路径相关函数
    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (
        DisplayName = "获取UtilityExtend插件目录", 
        Keywords = "get utilityextend plugin directory",
        ToolTip = "获取UtilityExtend插件的根目录路径",
        Category = "UtilityExtend|路径"
    ))
    static UPARAM(DisplayName = "插件目录路径") FString GetUtilityExtendPluginDirectory();


    // ------------------------------------编辑器操作相关函数------------------------------------
    // 重启编辑器
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "重启编辑器", 
        Keywords = "Restart Editor",
        Category = "UtilityExtend|编辑器操作"
    ))
    static void RestartEditor();

    // UtilityWidget运行相关函数
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "运行编辑器工具控件", 
        Keywords = "Run Utility Widget",
        ToolTip = "运行指定的EditorUtilityWidget，效果等同于右键点击'运行编辑器工具控件'",
        Category = "UtilityExtend|编辑器操作|编辑器工具控件增强"
    ))
    static UPARAM(DisplayName = "运行状态") bool RunUtilityWidget(
        UPARAM(DisplayName = "控件蓝图") class UEditorUtilityWidgetBlueprint* WidgetBlueprint,
        UPARAM(DisplayName = "标签页ID") FString& OutTabId
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "关闭编辑器工具控件", 
        Keywords = "close utility widget tab",
        ToolTip = "通过标签页ID关闭指定的UtilityWidget标签页",
        Category = "UtilityExtend|编辑器操作|编辑器工具控件增强"
    ))
    static UPARAM(DisplayName = "关闭状态") bool CloseUtilityWidgetTab(
        UPARAM(DisplayName = "标签页ID") const FString& TabId
    );

// ------------------------------------实验性功能------------------------------------
    // ===============文件读写相关函数===============

    // 文件读写相关函数
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "用文本方式读取任意文件", 
        Keywords = "Read Text File",
        ToolTip = "从指定路径读取文本文件内容，支持相对路径和绝对路径",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "读取状态") bool ReadTextFile(
        UPARAM(DisplayName = "文件路径") const FString& FilePath,
        UPARAM(DisplayName = "文件内容") FString& OutContent,
        UPARAM(DisplayName = "错误信息") FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "以文本方式写入文件", 
        Keywords = "Write Text File",
        ToolTip = "将文本内容保存到指定路径的文件，支持创建目录和覆盖文件",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "写入状态") bool WriteTextFile(
        UPARAM(DisplayName = "文件路径") const FString& FilePath,
        UPARAM(DisplayName = "文件内容") const FString& Content,
        UPARAM(DisplayName = "错误信息") FString& OutErrorMessage,
        UPARAM(DisplayName = "覆盖文件") bool bOverwrite = true,
        UPARAM(DisplayName = "创建目录") bool bCreateDirectories = true
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "检查文件是否存在", 
        Keywords = "check file exists",
        ToolTip = "检查指定路径的文件是否存在",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "文件存在") bool CheckFileExists(
        UPARAM(DisplayName = "文件路径") const FString& FilePath
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "获取文件大小", 
        Keywords = "Get File Size",
        ToolTip = "获取指定文件的大小（字节）",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "文件大小") int64 GetFileSize(
        UPARAM(DisplayName = "文件路径") const FString& FilePath
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "删除文件", 
        Keywords = "delete file",
        ToolTip = "删除指定路径的文件",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "删除状态") bool DeleteFile(
        UPARAM(DisplayName = "文件路径") const FString& FilePath,
        UPARAM(DisplayName = "错误信息") FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "复制文件", 
        Keywords = "Copy File",
        ToolTip = "将文件从源路径复制到目标路径，支持创建目录和覆盖文件",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "复制状态") bool CopyFile(
        UPARAM(DisplayName = "源文件路径") const FString& SourceFilePath,
        UPARAM(DisplayName = "目标文件路径") const FString& DestFilePath,
        UPARAM(DisplayName = "错误信息") FString& OutErrorMessage,
        UPARAM(DisplayName = "覆盖文件") bool bOverwrite = true,
        UPARAM(DisplayName = "创建目录") bool bCreateDirectories = true
    );

    // 文件对话框相关函数
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "打开文件选择对话框", 
        Keywords = "open file dialog",
        ToolTip = "打开系统文件选择对话框，支持文件类型过滤和多选模式",
        Category = "UtilityExtend|实验性功能|文件处理"
    ))
    static UPARAM(DisplayName = "选中的文件路径") TArray<FString> OpenFileDialog(
        UPARAM(DisplayName = "对话框标题") const FString& DialogTitle = TEXT("选择文件"),
        UPARAM(DisplayName = "默认路径") const FString& DefaultPath = TEXT(""),
        UPARAM(DisplayName = "文件类型过滤器") const FString& FileTypeFilter = TEXT("所有文件|*.*"),
        UPARAM(DisplayName = "允许多选") bool bAllowMultipleSelection = false
    );


    // ===============外部软件调用相关函数===============
    
    // 外部软件调用相关函数
    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Launch External Application", 
        Keywords = "启动外部程序 运行exe 调用外部软件",
        ToolTip = "启动外部可执行文件，支持命令行参数和工作目录设置",
        Category = "UtilityExtend|实验性功能|ExternalApp"
    ))
    static UPARAM(DisplayName = "启动状态") bool LaunchExternalApplication(
        UPARAM(DisplayName = "可执行文件路径") const FString& ExecutablePath, 
        UPARAM(DisplayName = "命令行参数") const FString& Arguments = TEXT(""), 
        UPARAM(DisplayName = "工作目录") const FString& WorkingDirectory = TEXT(""), 
        UPARAM(DisplayName = "分离启动") bool bLaunchDetached = true, 
        UPARAM(DisplayName = "隐藏启动") bool bLaunchHidden = false, 
        UPARAM(DisplayName = "最小化启动") bool bLaunchMinimized = false, 
        UPARAM(DisplayName = "最大化启动") bool bLaunchMaximized = false, 
        UPARAM(DisplayName = "正常启动") bool bLaunchNormal = false
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Launch External Application With Process Info", 
        Keywords = "启动外部程序详细信息 运行exe详细信息",
        ToolTip = "启动外部程序并返回详细信息，包括进程ID和错误信息",
        Category = "UtilityExtend|实验性功能|ExternalApp"
    ))
    static UPARAM(DisplayName = "启动状态") bool LaunchExternalApplicationWithInfo(
        UPARAM(DisplayName = "可执行文件路径") const FString& ExecutablePath, 
        UPARAM(DisplayName = "命令行参数") const FString& Arguments, 
        UPARAM(DisplayName = "工作目录") const FString& WorkingDirectory, 
        UPARAM(DisplayName = "分离启动") bool bLaunchDetached, 
        UPARAM(DisplayName = "隐藏启动") bool bLaunchHidden, 
        UPARAM(DisplayName = "最小化启动") bool bLaunchMinimized, 
        UPARAM(DisplayName = "最大化启动") bool bLaunchMaximized, 
        UPARAM(DisplayName = "正常启动") bool bLaunchNormal, 
        UPARAM(DisplayName = "输出进程ID") FString& OutProcessID, 
        UPARAM(DisplayName = "输出错误信息") FString& OutErrorMessage
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Is External Application Running", 
        Keywords = "检查外部程序是否运行 检查进程",
        ToolTip = "检查指定名称的程序是否正在运行",
        Category = "UtilityExtend|实验性功能|ExternalApp"
    ))
    static UPARAM(DisplayName = "是否运行") bool IsExternalApplicationRunning(
        UPARAM(DisplayName = "进程名称") const FString& ProcessName
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Terminate External Application", 
        Keywords = "终止外部程序 结束进程",
        ToolTip = "强制终止指定的外部程序",
        Category = "UtilityExtend|实验性功能|ExternalApp"
    ))
    static UPARAM(DisplayName = "终止状态") bool TerminateExternalApplication(
        UPARAM(DisplayName = "进程名称") const FString& ProcessName
    );

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Get All Running Processes", 
        Keywords = "获取所有运行进程 列出进程",
        ToolTip = "获取系统中所有正在运行的进程列表",
        Category = "UtilityExtend|实验性功能|ExternalApp"
    ))
    static UPARAM(DisplayName = "进程列表") TArray<FString> GetAllRunningProcesses();

    UFUNCTION(BlueprintCallable, meta = (
        DisplayName = "Wait For External Application", 
        Keywords = "等待外部程序 等待进程",
        ToolTip = "等待指定的外部程序启动，支持超时设置",
        Category = "UtilityExtend|实验性功能|ExternalApp"
    ))
    static UPARAM(DisplayName = "等待状态") bool WaitForExternalApplication(
        UPARAM(DisplayName = "进程名称") const FString& ProcessName, 
        UPARAM(DisplayName = "超时时间") float TimeoutSeconds = 30.0f
    );

};