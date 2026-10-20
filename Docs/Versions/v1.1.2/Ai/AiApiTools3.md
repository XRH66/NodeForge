# MCP 工具与返回 schema

工具调用前请阅读[通用调用规则](ApiReference.md)。需要 handle 的工具使用当前 connected 返回的工程与会话句柄。

## nodeforge_rules_query

按assetKind=Material或Blueprint查询本插件实际生成规则。 前提：当前handle、新callId/修订；query为空查全族，最多256字符，大小写无关OR子串。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：GT串行只读查询/报告，不把词转成图，不改变草稿。 判据：读原GraphRules报告/规则；不支持实例参数查询。 相关文档 pageId：AiTaskRecipes, Material, Blueprint。

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

发现当前目录digest、精确节点/函数签名、引脚及限制。 前提：当前handle、新callId及准确修订；arguments:{}。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：GT串行查询/报告，不修改草稿或资产；节点变化沿同工具发现。 判据：摘要和reports给有界导航；用report_read看真实节点，experimental不是全部实测。 相关文档 pageId：MaterialNodeIndex, BlueprintNodeIndex, DeveloperNodeExtension。

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

查询原callId的状态和收据，处理未知/晚到结果。 前提：当前handle；arguments.callId必须原调用ID；不重新提交业务。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; arguments: 精确操作参数；无参数也必须传空对象。 效果：只读查询并收集原结果；不允许换ID重放或自动清锁。 判据：原调用终态仍须检查原报告及部分写入；pending/unknown继续停后续GT。 相关文档 pageId：AiResultsAndRecovery。

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

选择正常打开的准确工程会话，取得opaque handle。 前提：先读AiPreparation；同用户一次AI接入并在UE启用插件。目标明确时传project；唯一自动选择，多候选需明确工程/实例。 参数：project: connect的可选准确uproject绝对路径；目标已知时明确提供，唯一合格会话可省略。; controlSessionId: connect的可选当前32位小写hex会话ID；多候选中选择准确实例，不猜历史值。 效果：返回连接身份或候选，不启动UE、不抢占现有连接、不生成资产；各handle保留自己的工程和会话。 判据：connected含handle与准确identity；零会话notConnected，多个selectionRequired含candidates。关闭重开后重新connect；environmentBlocked定位身份/占用，不清锁。 相关文档 pageId：AiPreparation, AiQuickStart。

输入schema：

```json
{
  "type": "object",
  "properties": {
    "project": {
      "type": "string",
      "minLength": 1,
      "description": "Optional exact uproject; unique qualified session auto-selects if omitted"
    },
    "controlSessionId": {
      "type": "string",
      "pattern": "^[0-9a-f]{32}$",
      "description": "Select one exact instance when multiple sessions are present"
    }
  },
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
    },
    "candidates": {
      "type": "array",
      "items": {
        "type": "object",
        "additionalProperties": true
      }
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
    },
    {
      "if": {
        "properties": {
          "status": {
            "enum": [
              "notConnected",
              "selectionRequired"
            ]
          }
        }
      },
      "then": {
        "required": [
          "candidates"
        ]
      }
    }
  ]
}
```

## nodeforge_docs_index

查询已安装 NodeForge 的版本、组件标识、功能概览和文档索引。无需启动 UE 或提供 handle，传入空对象即可调用。返回摘要最多 32768 字节，节点详情可按索引继续读取。成功时 status 为 passed，并返回 bundleId、bridgeVersion 与 docsManifestSha256 等版本标识。常用文档 pageId：Readme、AiReadme、DeveloperReadme、UserReadme。

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
