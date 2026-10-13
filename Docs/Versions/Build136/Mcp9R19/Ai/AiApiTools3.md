# 正式MCP工具与原结果

NodeForge把受控JSON转换为可编辑UE资产。先调用nodeforge_docs_index，再用nodeforge_docs_read读取Readme、AiQuickStart和FeatureMap，确认bundle/Build/桥/DLL/目录身份。资料无需UE；业务需要操作者显式准备并注册当前专用编辑器，再connect。只开放契约中的工具，不支持全部UE节点、循环、数组或生产写入。调用完成和资产最终核验不同；waitingPersistence需原CLI新PID Verify。pending/unknown立即停写，查询原calls.get，不换ID重发。开发原理/源码/扩展/发布见DeveloperReadme。


## nodeforge_rules_query

按assetKind=Material或Blueprint查询本插件实际生成规则。 前提：当前handle、新callId/修订；query为空查全族，最多256字符，大小写无关OR子串。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：GT串行只读查询/报告，不把词转成图，不改变草稿。 判据：读原GraphRules报告/规则；不支持实例参数查询。 资料pageId：AiTaskRecipes, Material, Blueprint。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

```json
{
  "type": "object",
  "properties": {
    "handle": {
      "type": "string",
      "pattern": "^[0-9a-f]{32}$"
    },
    "arguments": {
      "type": "object",
      "additionalProperties": false,
      "properties": {
        "assetKind": {
          "type": "string",
          "enum": [
            "Material",
            "Blueprint"
          ]
        },
        "query": {
          "type": "string",
          "maxLength": 256,
          "x-maxWhitespaceTerms": 8
        }
      },
      "required": [
        "assetKind",
        "query"
      ]
    },
    "callId": {
      "type": "string",
      "pattern": "^[A-Za-z0-9-]{1,64}$"
    },
    "expectedDraftRevision": {
      "type": "string",
      "pattern": "^(0|[1-9][0-9]{0,19})$"
    }
  },
  "required": [
    "handle",
    "arguments",
    "callId",
    "expectedDraftRevision"
  ],
  "additionalProperties": false
}
```

原结果schema：

```json
{
  "type": "object",
  "required": [
    "bridgeVersion",
    "status"
  ],
  "additionalProperties": true,
  "properties": {
    "bridgeVersion": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+\\.\\d+$",
      "description": "Actually loaded bridge release; exact identity is bound by the approved contract and manifest"
    },
    "status": {
      "type": "string",
      "description": "Bridge/CLI state; never replaces native overallStatus or persistence verdict"
    },
    "cliExitCode": {
      "type": "integer"
    },
    "nativeSummary": {
      "type": "object",
      "additionalProperties": true
    },
    "errors": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "code": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "unresolvedCalls": {
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "reports": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "reportId": {
            "type": "string"
          },
          "sha256": {
            "type": "string"
          },
          "bytes": {
            "type": "integer"
          },
          "overallStatus": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    }
  }
}
```

## nodeforge_catalog_get

发现当前目录digest、精确节点/函数签名、引脚及限制。 前提：当前handle、新callId及准确修订；arguments:{}。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：GT串行查询/报告，不修改草稿或资产；节点变化沿同工具发现。 判据：摘要和reports给有界导航；用report_read看真实节点，experimental不是全部实测。 资料pageId：MaterialNodeIndex, BlueprintNodeIndex, DeveloperNodeExtension。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

```json
{
  "type": "object",
  "properties": {
    "handle": {
      "type": "string",
      "pattern": "^[0-9a-f]{32}$"
    },
    "arguments": {
      "type": "object",
      "additionalProperties": false,
      "properties": {},
      "required": []
    },
    "callId": {
      "type": "string",
      "pattern": "^[A-Za-z0-9-]{1,64}$"
    },
    "expectedDraftRevision": {
      "type": "string",
      "pattern": "^(0|[1-9][0-9]{0,19})$"
    }
  },
  "required": [
    "handle",
    "arguments",
    "callId",
    "expectedDraftRevision"
  ],
  "additionalProperties": false
}
```

原结果schema：

```json
{
  "type": "object",
  "required": [
    "bridgeVersion",
    "status"
  ],
  "additionalProperties": true,
  "properties": {
    "bridgeVersion": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+\\.\\d+$",
      "description": "Actually loaded bridge release; exact identity is bound by the approved contract and manifest"
    },
    "status": {
      "type": "string",
      "description": "Bridge/CLI state; never replaces native overallStatus or persistence verdict"
    },
    "cliExitCode": {
      "type": "integer"
    },
    "nativeSummary": {
      "type": "object",
      "additionalProperties": true
    },
    "errors": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "code": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "unresolvedCalls": {
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "reports": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "reportId": {
            "type": "string"
          },
          "sha256": {
            "type": "string"
          },
          "bytes": {
            "type": "integer"
          },
          "overallStatus": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    }
  }
}
```

## nodeforge_calls_get

查询原callId的状态和收据，处理未知/晚到结果。 前提：当前handle；arguments.callId必须原调用ID；不重新提交业务。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。 效果：只读查询并收集原结果；不允许换ID重放或自动清锁。 判据：原调用终态仍须检查原报告及部分写入；pending/unknown继续停后续GT。 资料pageId：AiResultsAndRecovery。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

```json
{
  "type": "object",
  "properties": {
    "handle": {
      "type": "string",
      "pattern": "^[0-9a-f]{32}$"
    },
    "arguments": {
      "type": "object",
      "additionalProperties": false,
      "properties": {
        "callId": {
          "type": "string"
        }
      },
      "required": [
        "callId"
      ]
    }
  },
  "required": [
    "handle",
    "arguments"
  ],
  "additionalProperties": false
}
```

原结果schema：

```json
{
  "type": "object",
  "required": [
    "bridgeVersion",
    "status"
  ],
  "additionalProperties": true,
  "properties": {
    "bridgeVersion": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+\\.\\d+$",
      "description": "Actually loaded bridge release; exact identity is bound by the approved contract and manifest"
    },
    "status": {
      "type": "string",
      "description": "Bridge/CLI state; never replaces native overallStatus or persistence verdict"
    },
    "cliExitCode": {
      "type": "integer"
    },
    "nativeSummary": {
      "type": "object",
      "additionalProperties": true
    },
    "errors": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "code": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "unresolvedCalls": {
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "reports": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "reportId": {
            "type": "string"
          },
          "sha256": {
            "type": "string"
          },
          "bytes": {
            "type": "integer"
          },
          "overallStatus": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    }
  }
}
```

## nodeforge_connect

绑定操作者已注册的当前专用编辑器，取得opaque handle。 前提：先读AiPreparation；准确Config/当前SessionPublic和DLL，必须有UE活动会话。资料读取不需要它。 参数： 效果：建立业务lease和身份，不启动UE、不抢占现有连接、不生成资产。 判据：status=connected及handle；environmentBlocked请定位准备/占用，不自动清锁。 资料pageId：AiPreparation, AiQuickStart。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

```json
{
  "type": "object",
  "properties": {},
  "required": [],
  "additionalProperties": false
}
```

原结果schema：

```json
{
  "type": "object",
  "required": [
    "bridgeVersion",
    "status"
  ],
  "additionalProperties": true,
  "properties": {
    "bridgeVersion": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+\\.\\d+$",
      "description": "Actually loaded bridge release; exact identity is bound by the approved contract and manifest"
    },
    "status": {
      "type": "string",
      "description": "Bridge/CLI state; never replaces native overallStatus or persistence verdict"
    },
    "cliExitCode": {
      "type": "integer"
    },
    "nativeSummary": {
      "type": "object",
      "additionalProperties": true
    },
    "errors": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "code": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "unresolvedCalls": {
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "reports": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "reportId": {
            "type": "string"
          },
          "sha256": {
            "type": "string"
          },
          "bytes": {
            "type": "integer"
          },
          "overallStatus": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "handle": {
      "type": "string"
    },
    "identity": {
      "type": "object"
    },
    "operations": {
      "type": "integer"
    },
    "docsStatus": {
      "type": "string"
    }
  },
  "allOf": [
    {
      "if": {
        "properties": {
          "status": {
            "const": "connected"
          }
        }
      },
      "then": {
        "required": [
          "handle",
          "identity",
          "operations",
          "unresolvedCalls"
        ]
      }
    }
  ]
}
```

## nodeforge_docs_index

第一入口：离线发现准确版本、Build/DLL/桥/API、能力数量及读者/指南/节点族索引。完整维护清单留在本机Manifest与专用资料页。 前提：无需UE或handle；参数空对象。 参数： 效果：只读已批准资料，发现摘要最多32768字节；节点细页按族索引取，不输出整包源码证据清单。 判据：status=passed，manifest.bundleId/bridgeVersion及docsManifestSha256对应当前包。维护依据见SourcesAndLicenses/DeveloperSourceMap。 资料pageId：Readme, AiReadme, DeveloperReadme, UserReadme。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

```json
{
  "type": "object",
  "properties": {},
  "required": [],
  "additionalProperties": false
}
```

原结果schema：

```json
{
  "type": "object",
  "required": [
    "bridgeVersion",
    "status"
  ],
  "additionalProperties": true,
  "properties": {
    "bridgeVersion": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+\\.\\d+$",
      "description": "Actually loaded bridge release; exact identity is bound by the approved contract and manifest"
    },
    "status": {
      "type": "string",
      "description": "Bridge/CLI state; never replaces native overallStatus or persistence verdict"
    },
    "cliExitCode": {
      "type": "integer"
    },
    "nativeSummary": {
      "type": "object",
      "additionalProperties": true
    },
    "errors": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "code": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "unresolvedCalls": {
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "reports": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "reportId": {
            "type": "string"
          },
          "sha256": {
            "type": "string"
          },
          "bytes": {
            "type": "integer"
          },
          "overallStatus": {
            "type": "string"
          }
        },
        "additionalProperties": true
      }
    },
    "manifest": {
      "type": "object"
    },
    "totalPages": {
      "type": "integer"
    },
    "indexScope": {
      "type": "string"
    },
    "nodePagesListed": {
      "type": "boolean"
    },
    "docsManifestSha256": {
      "type": "string"
    }
  },
  "allOf": [
    {
      "if": {
        "properties": {
          "status": {
            "const": "passed"
          }
        }
      },
      "then": {
        "required": [
          "manifest",
          "totalPages",
          "docsManifestSha256",
          "indexScope",
          "nodePagesListed"
        ]
      }
    }
  ]
}
```

