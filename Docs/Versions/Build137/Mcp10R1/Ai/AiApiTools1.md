> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 正式MCP工具与原结果

公开副本中的请求、目标、进程、会话及来源报告身份为虚构示例；历史产品结论保持，原始证明由维护者内部保留。实际调用须使用本次分配和返回的完整原 text/sourceRef，示例身份与零摘要不能作为来源证明。

NodeForge把受控JSON转换为可编辑UE资产。先调用nodeforge_docs_index，再用nodeforge_docs_read读取Readme、AiQuickStart和FeatureMap，确认bundle/Build/桥/DLL/目录身份。资料无需UE；业务需要操作者显式准备并注册当前专用编辑器，再connect。只开放契约中的工具，不支持全部UE节点、循环、数组或生产写入。调用完成和资产最终核验不同；waitingPersistence需原CLI新PID Verify。pending/unknown立即停写，查询原calls.get，不换ID重发。开发原理/源码/扩展/发布见DeveloperReadme。


## nodeforge_operations_describe

读取当前正式操作、参数、host和dispatch定义。 前提：先connect取得handle；不要求新callId或修订。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。 效果：只读缓存，不修改草稿或资产。 判据：原CLI/native返回保持；确认当前定义与准确版本一致。 资料pageId：AiApiReference, FeatureMap。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_editor_getState

读取当前busy、草稿修订、结果状态和调用现场。 前提：当前handle；不猜PID或历史状态。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。 效果：只读缓存，不改变业务状态。 判据：从nativeSummary.response.result读真实状态，未验证资产保持未验。 资料pageId：AiResultsAndRecovery, DeveloperArchitecture。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_draft_get

取得当前输入及准确字符串draftRevision，为后续检查或写草稿准备。 前提：当前handle；无参传arguments:{}。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。 效果：只读；输入有大小及可读性边界。 判据：使用真实返回的修订和字节摘要，旧检查不用于后来编辑的草稿。 资料pageId：AiQuickStart, GraphAuthoring。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_draft_loadFile

把准入JSON文件载入当前草稿。 前提：新callId及准确修订；arguments.path是已准入文件绝对路径。 导出来源载入时，arguments.exportReportPath与exportReportSha256必须同时给出，引用原报告，text/path内容必须是原完整导出结果（只允许格式差异）。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：修改草稿和修订；不生成、不预留资产。 无关载入清掉旧导出基线；同源编辑／恢复按真实稿关联保留来源；普通JSON可不附导出引用。 判据：读原调用和新草稿修订；加载成功后再预检。 资料pageId：AiTaskRecipes, AiApiReference, AiAssetRead。当前版从docs_index确认；unknown/pending停写查原call。

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
        "path": {
          "type": "string"
        },
        "exportReportPath": {
          "type": "string"
        },
        "exportReportSha256": {
          "type": "string",
          "pattern": "^[0-9a-f]{64}$"
        }
      },
      "required": [
        "path"
      ],
      "dependentRequired": {
        "exportReportPath": [
          "ReplaceWithCurrentOriginalExportReportPath"
        ],
        "exportReportSha256": [
          "0000000000000000000000000000000000000000000000000000000000000000"
        ]
      }
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

## nodeforge_draft_replace

载入完整JSON，或显式保留当前可信导出来源编辑草稿。 前提：新callId及当前准确修订；完整UTF-8 text≤65536字节。初次载入原导出须成对exportReportPath/exportReportSha256且语义原样。编辑已载入来源时明确retainExportOrigin:true，禁止同时给任一新来源字段；省略/false仍走原严格Replace。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：只改草稿，true保留匹配原文/来源/准备/恢复关系并核真实源与原报告。无关Replace清来源；实际文本变化才revision+1，旧结果stale，不自动预检/生成。 判据：读原call及draft.get完整稿/新修订；失败原稿保持。无来源不能retain，unknown/pending先查原call，不重发。 资料pageId：AiQuickStart, Examples, AiAssetRead, AiCopyPreview。当前版从docs_index确认；unknown/pending停写查原call。

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
        "text": {
          "type": "string"
        },
        "exportReportPath": {
          "type": "string"
        },
        "exportReportSha256": {
          "type": "string",
          "pattern": "^[0-9a-f]{64}$"
        },
        "retainExportOrigin": {
          "type": "boolean"
        }
      },
      "required": [
        "text"
      ],
      "dependentRequired": {
        "exportReportPath": [
          "ReplaceWithCurrentOriginalExportReportPath"
        ],
        "exportReportSha256": [
          "0000000000000000000000000000000000000000000000000000000000000000"
        ]
      },
      "allOf": [
        {
          "if": {
            "properties": {
              "retainExportOrigin": {
                "const": true
              }
            },
            "required": [
              "retainExportOrigin"
            ]
          },
          "then": {
            "not": {
              "anyOf": [
                {
                  "required": [
                    "exportReportPath"
                  ]
                },
                {
                  "required": [
                    "exportReportSha256"
                  ]
                }
              ]
            }
          }
        }
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

