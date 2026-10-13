# MCP 工具参考：规则与连接

本页适用于 NodeForge v0.1.0。调用顺序见 [快速开始](QuickStart.md)，状态解释见 [结果与恢复](ResultsAndRecovery.md)。下列 JSON Schema 保留接口的准确字段与约束。

有 `handle` 的工具使用本次连接返回值；有 `expectedDraftRevision` 的工具使用最近一次 `draft_get` 返回的十进制字符串。新操作先保存自己的 `callId`，查询或恢复时使用同一个 ID。`pending` 或 `unknown` 时停止后续写入并查询原调用。

## nodeforge_rules_query

按 assetKind=Material 或 Blueprint 查询生成规则。需要当前 handle、新 callId 和当前修订。query 为空时查询该类全部规则；非空时最多 256 字符、8 个词，按大小写无关的 OR 子串筛选。

结果在 GraphRules 报告中。查询在 Game Thread 串行执行，不修改草稿，也不会自动把查询词转换成图。不支持材质实例参数查询。

输入 Schema：

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

返回 Schema：

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

读取当前 catalogDigest、节点和函数签名、引脚与限制。需要当前 handle、新 callId 和当前修订，arguments 传空对象。

通过 reports 与 report_read 读取完整节点数据。此操作不修改草稿或资产。目录中的 experimental 状态表示具体组合仍需检查，不代表所有节点组合均已验证。

输入 Schema：

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

返回 Schema：

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

查询已经提交的操作及其晚到结果。使用当前 handle，将 arguments.callId 设置为原操作的调用 ID，不重新提交业务。

即使操作已经结束，也要检查报告与可能的部分写入。pending 或 unknown 时继续停止新写入；此工具不会自动清锁或改变调用 ID。

输入 Schema：

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

返回 Schema：

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

连接安装工具已准备好的编辑器，取得当前 handle。无需参数，但需要对应工程的编辑器正在运行、配置与插件匹配且会话可用。

成功返回 status=connected 和 handle。此工具不启动 UE、不抢占已有连接，也不生成资产。environmentBlocked 时按 [连接准备](Preparation.md)检查工程和占用状态。读取文档无需 connect。

输入 Schema：

```json
{
  "type": "object",
  "properties": {},
  "required": [],
  "additionalProperties": false
}
```

返回 Schema：

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

读取版本、组件信息、能力数量和文档导航。无需 UE 或 handle，参数为空对象。成功时检查 status=passed、manifest.bundleId、bridgeVersion 和 docsManifestSha256。

索引最多 32768 字节。具体页面通过 docs_read 读取，节点页从对应类别索引进入。超过限制时返回错误，不把截断内容视为完整索引。

输入 Schema：

```json
{
  "type": "object",
  "properties": {},
  "required": [],
  "additionalProperties": false
}
```

返回 Schema：

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
