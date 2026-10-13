# 旋转成员与对象默认输入

本目录保留新增能力的原始测试模板。Build43已实际通过[22例构图/保存重开](../../Reports/20261008GraphV2/Rotators1/Summary.json)和[11图PIE](../../Reports/20261009UsableEditor/RotatorRuntime1/Summary.json)；精度修正Build44的受影响子集另见BlueprintRotatorPrecision，不能直接继承旧构建结果。由[构图运行器](../../Scripts/RunGraphRegression.py)绑定本次真实原生目录，再由[运行检查](../../Scripts/RunBlueprintMotionChecks.py)检查Actor实际变换和消息。原始准备过程见[PrepareFixtures.py](../../Reports/20261009UsableEditor/RotatorExpansion1/PrepareFixtures.py)。

新类型为`rotator`；成员默认及未连接旋转输入默认必须是恰含`pitch`、`yaw`、`roll`的对象，分量为有限数字且绝对值不超过1e12。图输入中的对象字段与RuntimeCases中的数组不同：运行器的`expectedRotation`顺序固定为`[Pitch,Yaw,Roll]`，比较容差1e-6。下列三轴取不同数值，避免轴交换被零值掩盖。

| 输入 | 预期运行行为 |
| --- | --- |
| RotatorRead | 读取Facing的CDO默认，Actor旋转为[15,40,-25]。 |
| RotatorChanged | 只改成员默认，Actor为[-12.5,72.25,8.75]。 |
| RotatorStore | MakeRotator产生[-20,65,35]，先写成员，再读取并应用。 |
| RotatorReordered | 与Store同节点及默认，只改执行连接；先应用原值再写成员，Actor为[15,40,-25]。 |
| RotatorDirect | 不声明变量，NewRotation对象默认直接得到[22.5,-55,12.25]。 |
| RotatorStoreDefault | Set的Facing输入用对象默认，写回后Actor为[5.5,-35.25,-10.75]。 |
| RotatorShuffled | Store的nodes/edges倒序；结构、逐ID布局、实际旋转均应与Store一致。 |

上述消息均要求恰一次、归属于实际测试Actor的`Rotator applied`；位置保持[0,0,0]、缩放[1,1,1]。四个旧摘要兼容例保留真实原值：vector来自BlueprintMotion1已绑定输入，int/name来自BlueprintMemberScalars，Sequence来自BlueprintSequence。它们仍检查原有位置、变换及精确消息，不把摘要占位符当作旧版本。

Cases包含11正例、11拒绝例：成员数组/字符串/缺轴/额外轴/布尔分量/过大分量、函数默认数组/字符串分量、vector到rotator错误连接、Build42摘要使用新成员或新默认。拒绝必须在构建前发生并无资产写入；原始失败收据须保留。没有声明覆盖全部非法输入排列或非有限JSON数字解析；通用运行器以严格JSON重新序列化，不能直接保留Infinity/NaN载荷。

新能力模板保留`CATALOG_DIGEST_FROM_REAL_ENGINE`，运行器仅派生执行输入时替换。四旧正例及两旧42负例设置`preserveCatalogDigest`，其中42摘要固定为`588f572606053d53eebe80b3461b0e01baac2326f30e778c2cdbc3f03e4e2801`。模板本身不能直接导入。RuntimeCases列11个正例；新目录导出与模板预检、同构建保存重开和实际PIE均由主任务执行及记录，新族真实UI和交付也不由本目录声明通过。
