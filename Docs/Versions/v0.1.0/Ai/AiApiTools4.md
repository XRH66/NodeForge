# MCP 工具参考：文档、报告与资源

本页适用于 NodeForge v0.1.0。调用顺序见 [快速开始](QuickStart.md)，状态解释见 [结果与恢复](ResultsAndRecovery.md)。下列 JSON Schema 保留接口的准确字段与约束。

有 `handle` 的工具使用本次连接返回值；有 `expectedDraftRevision` 的工具使用最近一次 `draft_get` 返回的十进制字符串。新操作先保存自己的 `callId`，查询或恢复时使用同一个 ID。`pending` 或 `unknown` 时停止后续写入并查询原调用。

## nodeforge_docs_read

使用 docs_index 或节点索引返回的 pageId 读取完整文档。无需 UE 或 handle。pageId 长度为 1–128 字符，不能传任意文件路径。

每页最多 65536 UTF-8 字节，服务检查字节数与 SHA。成功返回 status=passed，以及 bundleId、pageId、text、sha256 和 docsManifestSha256。

输入 Schema：

```json
{
  "type": "object",
  "properties": {
    "pageId": {
      "type": "string",
      "minLength": 1,
      "maxLength": 128
    }
  },
  "required": [
    "pageId"
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
    },
    "bundleId": {
      "type": "string"
    },
    "pageId": {
      "type": "string"
    },
    "text": {
      "type": "string"
    },
    "sha256": {
      "type": "string"
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
          "bundleId",
          "pageId",
          "text",
          "sha256",
          "docsManifestSha256"
        ]
      }
    }
  ]
}
```

## nodeforge_report_read

按 RFC6901 pointer 读取当前会话已登记报告。handle 使用当前连接，reportId 来自业务返回的 reports，pointer 最多 1024 字符。空串选择整个报告，不需要新 callId 或修订。

报告最多 1 MiB，普通选择结果最多 32768 字节；超限返回 navigation 和 selectionReturned=false，可继续读取更小子树。/exportedRequest 专用分支一次返回最多 65536 UTF-8 字节的完整图文本与 sourceRef。报告只读，不允许指定任意文件。

输入 Schema：

```json
{
  "type": "object",
  "properties": {
    "handle": {
      "type": "string",
      "pattern": "^[0-9a-f]{32}$"
    },
    "reportId": {
      "type": "string",
      "pattern": "^[0-9a-f]{64}$"
    },
    "pointer": {
      "type": "string",
      "maxLength": 1024
    }
  },
  "required": [
    "handle",
    "reportId",
    "pointer"
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
    },
    "reportId": {
      "type": "string"
    },
    "callId": {
      "type": "string"
    },
    "pointer": {
      "type": "string"
    },
    "sha256": {
      "type": "string"
    },
    "value": {},
    "selectionReturned": {
      "type": "boolean"
    },
    "limitBytes": {
      "type": "integer"
    },
    "navigation": {
      "type": "object"
    },
    "text": {
      "type": "string",
      "description": "For /exportedRequest only: exact complete native serialized Graph, at most 65536 UTF-8 bytes"
    },
    "payloadSha256": {
      "type": "string",
      "pattern": "^[0-9a-f]{64}$"
    },
    "sourceRef": {
      "type": "object",
      "required": [
        "exportReportPath",
        "exportReportSha256",
        "source",
        "sourceAssetSha256",
        "assetKind",
        "payloadSha256"
      ],
      "properties": {
        "exportReportPath": {
          "type": "string"
        },
        "exportReportSha256": {
          "type": "string",
          "pattern": "^[0-9a-f]{64}$"
        },
        "source": {
          "type": "string"
        },
        "sourceAssetSha256": {
          "type": "string",
          "pattern": "^[0-9a-f]{64}$"
        },
        "assetKind": {
          "enum": [
            "Material",
            "Blueprint"
          ]
        },
        "payloadSha256": {
          "type": "string",
          "pattern": "^[0-9a-f]{64}$"
        }
      },
      "additionalProperties": false
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
          "reportId",
          "callId",
          "pointer",
          "sha256",
          "value"
        ]
      }
    },
    {
      "if": {
        "required": [
          "status",
          "pointer"
        ],
        "properties": {
          "status": {
            "const": "passed"
          },
          "pointer": {
            "const": "/exportedRequest"
          }
        }
      },
      "then": {
        "required": [
          "text",
          "payloadSha256",
          "sourceRef"
        ]
      }
    },
    {
      "if": {
        "properties": {
          "status": {
            "const": "resultTooLarge"
          }
        }
      },
      "then": {
        "required": [
          "reportId",
          "pointer",
          "selectionReturned",
          "limitBytes",
          "navigation"
        ],
        "properties": {
          "selectionReturned": {
            "const": false
          }
        }
      }
    }
  ]
}
```

## nodeforge_resources_inspect

读取指定资源的实际参数、函数签名或纹理属性。需要当前 handle、新 callId 和当前修订，kind 与 source 使用 Schema 规定的值和精确对象路径。

资源必须符合允许的读取范围、类型和保存状态。操作返回报告，不改变草稿；sourceMemoryUnproven 不会触发自动重载或保存。结果字段和图中引用方法见 [资源读取](AssetRead.md)与 [纹理和函数](Resources.md)。

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
        "kind": {
          "type": "string",
          "enum": [
            "MaterialParameters",
            "MaterialFunction",
            "Texture2D"
          ]
        },
        "source": {
          "type": "string"
        }
      },
      "required": [
        "kind",
        "source"
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

## nodeforge_graphs_export

将支持的已保存材质或 Actor 蓝图导出成完整 JSON。需要新 callId、当前修订、精确 source，以及未占用的新 requestId 和 assetPath；源和目标必须不同。

省略 formatVersion 返回 v2，显式传入 3 返回 v3。操作不改变草稿或预留目标，成功后读取报告的 /exportedRequest，取得完整 text 和来源引用，再显式载入草稿。未知节点或无法表达的属性会拒绝导出，详见 [图导出](AssetRead.md)。

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
        "source": {
          "type": "string"
        },
        "requestId": {
          "type": "string"
        },
        "assetPath": {
          "type": "string"
        },
        "formatVersion": {
          "type": "integer",
          "enum": [
            2,
            3
          ],
          "default": 2
        }
      },
      "required": [
        "assetKind",
        "source",
        "requestId",
        "assetPath"
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

## nodeforge_resources_query

在指定允许目录内查找 Texture2D、MaterialFunction 或 Material。需要新 callId 和当前修订。查询只读取注册表元数据，不加载对象，不创建来源记录或草稿。

从 /resourceQuery 检查 complete、truncated、rows 和 next。列表结果仍需 resources_inspect；不把 discovered 或 policyAllowed 当成可直接使用的证明。分页与目录限制见 [资源查询](Resources.md)。

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
        "kind": {
          "type": "string",
          "enum": [
            "Texture2D",
            "MaterialFunction",
            "Material"
          ]
        },
        "directory": {
          "type": "string",
          "maxLength": 256,
          "minLength": 1
        },
        "nameContains": {
          "type": "string",
          "maxLength": 128,
          "default": ""
        },
        "recursive": {
          "type": "boolean",
          "default": false
        },
        "limit": {
          "type": "integer",
          "minimum": 1,
          "maximum": 200,
          "default": 50
        },
        "after": {
          "type": "string",
          "maxLength": 512,
          "default": ""
        }
      },
      "required": [
        "kind",
        "directory"
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
