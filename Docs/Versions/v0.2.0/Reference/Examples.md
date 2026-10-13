# 完整 JSON 示例（v2）

下面三个例子展示材质、Actor 蓝图和材质实例的完整 v2 请求。v0.2.0 同时支持 v3，新的内容也可以从[完整 v3 示例](../Ai/JsonArchitectureExamples.md)开始。

## 使用步骤

1. 完成[安装](../User/PluginInstallation.md)，打开 NodeForge 面板。
2. 选择一个例子，把 `requestId` 和 `assetPath` 改成尚未使用的编号与路径。
3. 确认 `catalogDigest` 与当前插件返回的目录摘要一致。
4. 将 JSON 载入草稿，点击检查；通过后生成，并打开新资产查看结果。

重复使用同一目标路径或请求编号会被拒绝。材质实例还需要先读取自己的父材质，不能直接使用示例占位值。

## 颜色材质

创建一个带颜色和粗糙度参数的 Surface 材质。生成后可以在 UE 中修改 `SurfaceColor` 和 `SurfaceRoughness`。


```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleMaterial",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/Examples/StaticColor",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [
    {
      "id": "Color",
      "class": "/Script/Engine.MaterialExpressionVectorParameter",
      "properties": {
        "ParameterName": "SurfaceColor",
        "DefaultValue": [
          0.05,
          0.55,
          0.8,
          1
        ]
      }
    },
    {
      "id": "Rough",
      "class": "/Script/Engine.MaterialExpressionScalarParameter",
      "properties": {
        "ParameterName": "SurfaceRoughness",
        "DefaultValue": 0.3
      }
    }
  ],
  "edges": [
    {
      "from": "Color",
      "output": "0",
      "to": "Material",
      "input": "BaseColor"
    },
    {
      "from": "Rough",
      "output": "0",
      "to": "Material",
      "input": "Roughness"
    }
  ],
  "settings": {
    "domain": "Surface",
    "blend": "Opaque",
    "shading": "DefaultLit",
    "twoSided": false
  }
}
```

## 输出文本的 Actor 蓝图

BeginPlay 事件调用 PrintString，把两个字符串拼接后的结果写入日志。示例关闭屏幕文字输出；要观察它，将生成的蓝图放入场景并运行，查看 UE 的 Output Log。


```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleBlueprint",
  "assetType": "Blueprint",
  "assetPath": "/Game/NodeForgeTests/Examples/BlueprintConcat",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [
    {
      "id": "Begin",
      "class": "/Script/BlueprintGraph.K2Node_Event",
      "properties": {
        "owner": "/Script/Engine.Actor",
        "member": "ReceiveBeginPlay"
      }
    },
    {
      "id": "Join",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetStringLibrary",
        "member": "Concat_StrStr"
      },
      "defaults": {
        "A": "Data-driven ",
        "B": "function output"
      }
    },
    {
      "id": "Print",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetSystemLibrary",
        "member": "PrintString"
      },
      "defaults": {
        "bPrintToScreen": false,
        "bPrintToLog": true
      }
    }
  ],
  "edges": [
    {
      "from": "Begin",
      "output": "then",
      "to": "Print",
      "input": "execute"
    },
    {
      "from": "Join",
      "output": "ReturnValue",
      "to": "Print",
      "input": "InString"
    }
  ],
  "settings": {
    "parentClass": "/Script/Engine.Actor",
    "variables": []
  }
}
```

## 材质实例

这个例子展示结构，父路径与全零摘要是占位值。先保存自己的父材质，再用 `resources.inspect(kind=MaterialParameters)` 读取它，把返回的实际路径与文件 SHA256 填入 `parent` 和 `parentAssetSha256`。

三个参数映射为空时，实例使用父材质的值。需要覆盖参数时，按读取到的参数名称和类型填写；详见[材质实例](MaterialInstance.md)。


```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleInstance",
  "assetType": "MaterialInstance",
  "assetPath": "/Game/NodeForgeTests/Examples/Instance",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [],
  "edges": [],
  "settings": {
    "parent": "/Game/NodeForgeTests/Examples/Parent.Parent",
    "parentAssetSha256": "0000000000000000000000000000000000000000000000000000000000000000",
    "scalarParameters": {},
    "vectorParameters": {},
    "textureParameters": {}
  }
}
```

## 通过 AI 使用

连接后先读取当前草稿，载入完整 JSON，再使用返回的新修订号进行预检和生成。工具返回任务编号后，查询该任务的进度和报告。

完整操作顺序见[AI 快速开始](../Ai/QuickStart.md)，遇到报错或调用中断时查看[保存检查与错误处理](ResultsAndRecovery.md)。
