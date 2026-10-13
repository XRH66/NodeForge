# MCP 工具参考

本页适用于 NodeForge v0.1.0。调用顺序见 [快速开始](QuickStart.md)，状态解释见 [结果与恢复](ResultsAndRecovery.md)。下列 JSON Schema 保留接口的准确字段与约束。

有 `handle` 的工具使用本次连接返回值；有 `expectedDraftRevision` 的工具使用最近一次 `draft_get` 返回的十进制字符串。新操作先保存自己的 `callId`，查询或恢复时使用同一个 ID。`pending` 或 `unknown` 时停止后续写入并查询原调用。

## nodeforge_operations_describe

读取当前插件的操作名称、参数、host 和 dispatch 定义。使用当前 handle，arguments 传空对象。此操作只读，不需要新的 callId 或草稿修订。

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

## nodeforge_editor_getState

读取编辑器是否忙碌、当前草稿修订、调用与结果状态。使用当前 handle，arguments 传空对象。从 nativeSummary.response.result 查看实际状态，不自行推断资产是否已经通过检查。

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

## nodeforge_draft_get

读取当前草稿及 draftRevision。使用当前 handle，arguments 传空对象。后续修改使用返回的十进制字符串修订号；先确认 draftReadable，再使用完整 text 和 digest。

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

## nodeforge_draft_loadFile

将允许目录内的 JSON 文件载入草稿。path 必须是实际文件的绝对路径；传入新 callId 和当前 expectedDraftRevision。成功后重新读取草稿修订。

载入导出结果时，exportReportPath 与 exportReportSha256 必须成对提供，文件内容应与完整导出语义一致。普通 JSON 可以不附来源引用。此操作只修改草稿，不生成或预留资产；无关输入会清除原来源关系。

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

## nodeforge_draft_replace

将完整 UTF-8 JSON 字符串载入草稿，text 最多 65536 字节。需要新 callId 和当前 expectedDraftRevision。成功后重新读取草稿，实际文本变化会增加修订并使旧检查失效。

初次载入导出结果时，成对提供 exportReportPath/exportReportSha256，保持原图语义。编辑已载入的图时可设 retainExportOrigin:true 保留当前来源，但不能同时传新的报告引用。省略或 false 不表示保留来源；没有来源时不能使用 true。详见 [副本编辑](CopyPreview.md)。

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
