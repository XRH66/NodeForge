# 空间输入：给外部AI的规则

本页适用于Windows、UE5.3.2、Build42隔离插件，真实目录为[UE532MaterialSpatial](../Catalog/UE532MaterialSpatial.json)，摘要`588f572606053d53eebe80b3461b0e01baac2326f30e778c2cdbc3f03e4e2801`。遵循[Graph v2协议](../Contracts/GraphProtocol.md)及[已有纹理/函数规则](TextureFunctionsAiRules.md)。加载的插件必须匹配本目录；根工程和RC1尚未升级。当前测试进度及未测限制见[空间输入说明](../Docs/MaterialSpatialInputs.md)。

输出是NodeForgeGraph version2的九字段JSON，不输出脚本或材质代码。使用新requestId、新`/Game/NodeForgeTests/`目标；nodes按声明逐项生成，所有节点必须到达启用的材质输出。真实属性/引脚/参数值均以目录为准，不凭显示名称猜测。节点的`position`可省略以使用自动布局；提供时为`[x,y]`两整数，连接不依赖nodes数组顺序。

新增节点均用`/Script/Engine.MaterialExpression`前缀：

| 后缀 | 属性及输出 |
| --- | --- |
| WorldPosition | `WorldPositionShaderOffset`取目录中精确WPT_*名称；输出`0`是XYZ、`1`是XY、`2`是Z。 |
| ObjectPositionWS | `OriginType`为`Absolute`或`CameraRelative`；输出`0`为原生物体位置，不能冒充任意Actor引用，实例化网格的具体语义由UE决定。 |
| CameraPositionWS | 无可写属性/输入，输出`0`为摄像机世界位置。 |
| CameraVectorWS | 无可写属性/输入，输出`0`为原生像素到摄像机方向；本机编译器限制为像素/计算阶段，不用于顶点WorldPositionOffset。 |
| VertexNormalWS | 无可写属性/输入，输出`0`为原生顶点世界法线。 |
| PixelNormalWS | 无可写属性/输入，输出`0`为原生像素世界法线；不可用于Normal输入或顶点WorldPositionOffset。 |

枚举只能写目录精确字符串，不能写数字、中文别名、MAX或臆造值。空间向量不会自动变换为切线空间；本轮未新增坐标变换表达式。相减比较时必须保持坐标原点一致，例如WorldPosition的CameraRelativeNoOffsets与ObjectPositionWS.CameraRelative配对。包含/排除WPO的选项按原生语义处理；不同坐标原点混用不保证效果正确。

使用现有Math/Time/Parameter节点组合：世界坐标→分量→Scale→与Time×Speed相加→Sine→颜色插值，可得到动态世界坐标条纹；对象位置相减可形成相对中心的颜色；Normal与CameraVector做Dot/Abs可形成视角着色。这些只是数据样例，没有效果专用导入分支。参考[输入集合](../Tests/MaterialSpatial/Readme.md)，不得把这些项目自制测试数据声称为外部AI原始输出。

旧Build41及更早摘要不能获得新空间节点权限，旧受支持范围保留。原生着色器阶段限制可能在编译时失败，不能将生成或着色器请求提交写成整体成功；保存独立重开、运行/画面、真实UI、无插件交付分别依据证据确认。
