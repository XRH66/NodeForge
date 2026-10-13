# 安装 NodeForge

本指南适用于 **NodeForge v0.2.0、Windows 64 位、官方 UE 5.3.2**。普通使用无需安装 Python 或配置 AI。

## 1. 下载插件

下载 [NodeForge 插件 ZIP](https://github.com/XRH66/NodeForge/releases/download/v0.2.0/NodeForge-0.2.0-UE5.3.2-Windows.zip)，解压后可以看到 `NodeForge` 文件夹。

Release 页的 `Source code` 是源码压缩包，安装时请选择上面的插件 ZIP。

GitHub 仓库目前为私有：下载前需要登录有仓库访问权的账号。看到 404 时，先确认账号权限，再从 [v0.2.0 Release](https://github.com/XRH66/NodeForge/releases/tag/v0.2.0) 查看附件。

## 2. 复制到引擎

关闭使用这套引擎的编辑器，把整个 `NodeForge` 文件夹复制到 **UE 5.3.2 引擎目录的 `Engine/Plugins/Marketplace`**。没有 `Marketplace` 文件夹时，新建一个。只需要复制一次，同一引擎打开的各个工程都可以启用插件。

安装后的目录应如下所示：

```text
UE_5.3/
└─ Engine/Plugins/Marketplace/NodeForge/
   ├─ NodeForge.uplugin
   ├─ Binaries/
   ├─ Examples/
   └─ ...
```

注意不要多套一层 `NodeForge` 文件夹。安装目录受 Windows 保护时，复制或补装可能需要写入权限；日常使用不需要管理员身份。

## 3. 启用插件

打开工程，在插件管理器中搜索并启用 **NodeForge**。如果 UE 提示重启编辑器，按提示重启。

从 **窗口（Window）→ NodeForge** 打开插件面板。首次打开时，核对自动识别的工程和引擎位置，点击 **确认当前工程并开始使用**。同一工程以后打开时无需重复确认。

## 4. 试用附带示例

按 [入门教程](GettingStarted.md) 生成第一个材质。之后如果想用 AI 操作插件，继续阅读 [连接 AI](AiInstallation.md)。

## 安装遇到问题

**窗口菜单中没有 NodeForge。** 检查插件是否启用，以及 `NodeForge.uplugin` 是否位于上面的目录。

**UE 提示模块缺失或引擎版本不匹配。** 检查下载的是插件 ZIP，并确认使用官方 UE 5.3.2。这个下载包不适用于其他 UE 版本或定制引擎。

**已经装过 NodeForge。** 先关闭工程，再检查现有安装。不要在工程和引擎目录中各放一份插件；AI 接入可以在同一安装位置补装。旧版迁移见[升级说明](Versions/v0.2.0/Version/CompatibilityAndMigration.md)。

## 高级：只安装到一个工程

同一个 ZIP 也可以把 `NodeForge` 放入工程的 `Plugins/NodeForge`。引擎直接目录 `Engine/Plugins/NodeForge` 同样支持。选择其中一个位置即可；同一工程发现多份同名插件时会报告冲突，不会替你删除旧副本。工程级 AI 补装需额外传 `-Project` 和准确 `-PluginDirectory`，见[AI 安装指南](AiInstallation.md)。
