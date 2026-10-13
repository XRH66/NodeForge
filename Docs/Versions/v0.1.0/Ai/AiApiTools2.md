# MCP 工具参考：副本与生成

本页适用于 NodeForge v0.1.0。调用顺序见 [快速开始](QuickStart.md)，状态解释见 [结果与恢复](ResultsAndRecovery.md)。下列 JSON Schema 保留接口的准确字段与约束。

有 `handle` 的工具使用本次连接返回值；有 `expectedDraftRevision` 的工具使用最近一次 `draft_get` 返回的十进制字符串。新操作先保存自己的 `callId`，查询或恢复时使用同一个 ID。`pending` 或 `unknown` 时停止后续写入并查询原调用。

## nodeforge_draft_restore

恢复服务保留的上一份可恢复草稿。需要新 callId、当前 expectedDraftRevision 和可恢复输入。恢复后读取完整草稿与修订。此操作不会还原磁盘资产，也不会重新执行生成。

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

## nodeforge_draft_prepareCopy

为当前材质或 Actor 蓝图草稿产生新的 requestId 和 assetPath，并提交派生草稿。需要新 callId、当前修订，arguments 传空对象。支持 Graph v2/v3；如果草稿带来源，会重新检查源和报告。

工具会写准备记录，但不预留目标或生成资产。检查 nativeSummary.response.result 中的 draftApplied、修订和 copyPreparation，再用 draft_get 读取实际新目标与完整图。报告已写出不表示草稿一定提交成功。详见 [副本编辑](CopyPreview.md)。

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
      "additionalProperties": true,
      "description": "原CLI摘要；response.result是原call，其result才是业务结果，unknown保留真实部分写入/提交事实。",
      "properties": {
        "response": {
          "type": "object",
          "additionalProperties": true,
          "properties": {
            "result": {
              "type": "object",
              "additionalProperties": true,
              "properties": {
                "result": {
                  "type": "object",
                  "additionalProperties": true,
                  "properties": {
                    "state": {
                      "type": "string"
                    },
                    "code": {
                      "type": "string"
                    },
                    "cliExitCode": {
                      "type": "integer"
                    },
                    "draft": {
                      "type": "object",
                      "additionalProperties": true,
                      "properties": {
                        "draftRevision": {
                          "type": "string"
                        },
                        "digest": {
                          "type": "string"
                        },
                        "copyPreparationReport": {
                          "type": "string"
                        },
                        "canRestore": {
                          "type": "boolean"
                        }
                      }
                    },
                    "nativeReport": {
                      "type": "object"
                    },
                    "inputSnapshot": {
                      "type": "string"
                    },
                    "inputDigest": {
                      "type": "string"
                    },
                    "controlEvidencePath": {
                      "type": "string"
                    },
                    "controlEvidenceSha256": {
                      "type": "string"
                    },
                    "draftApplied": {
                      "type": "boolean"
                    },
                    "previousDraftRevision": {
                      "type": "string"
                    },
                    "newDraftRevision": {
                      "type": "string"
                    },
                    "copyPreparation": {
                      "type": "object",
                      "additionalProperties": true,
                      "properties": {
                        "preparedInputPath": {
                          "type": "string"
                        },
                        "preparedInputDigest": {
                          "type": "string"
                        },
                        "originalRequestId": {
                          "type": "string"
                        },
                        "originalAssetPath": {
                          "type": "string"
                        },
                        "newRequestId": {
                          "type": "string"
                        },
                        "newAssetPath": {
                          "type": "string"
                        },
                        "targetReserved": {
                          "type": "boolean"
                        },
                        "assetGenerated": {
                          "type": "boolean"
                        }
                      }
                    }
                  },
                  "allOf": [
                    {
                      "if": {
                        "required": [
                          "state"
                        ],
                        "properties": {
                          "state": {
                            "const": "completed"
                          }
                        }
                      },
                      "then": {
                        "required": [
                          "state",
                          "code",
                          "cliExitCode",
                          "draft",
                          "nativeReport",
                          "inputSnapshot",
                          "inputDigest",
                          "controlEvidencePath",
                          "controlEvidenceSha256",
                          "draftApplied",
                          "previousDraftRevision",
                          "newDraftRevision",
                          "copyPreparation"
                        ]
                      }
                    }
                  ]
                }
              }
            }
          }
        }
      }
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

## nodeforge_graphs_preview

比较当前草稿与绑定来源的结构和布局差异。需要可靠的 exportOrigin、新 callId、当前修订，arguments 传空对象；不接受任意 previousReport。

操作写差异报告并更新展示，不改变草稿或资产，不预留目标。成功后从 reports 取得 reportId，读取 /graphDifference。之后编辑会使旧预览失效，预览通过也不表示编译、运行或画面已经通过检查。

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
      "additionalProperties": true,
      "description": "原CLI摘要；response.result是原call，其result才是业务结果，unknown保留真实部分写入/提交事实。",
      "properties": {
        "response": {
          "type": "object",
          "additionalProperties": true,
          "properties": {
            "result": {
              "type": "object",
              "additionalProperties": true,
              "properties": {
                "result": {
                  "type": "object",
                  "additionalProperties": true,
                  "properties": {
                    "state": {
                      "type": "string"
                    },
                    "code": {
                      "type": "string"
                    },
                    "cliExitCode": {
                      "type": "integer"
                    },
                    "draft": {
                      "type": "object",
                      "additionalProperties": true,
                      "properties": {
                        "draftRevision": {
                          "type": "string"
                        },
                        "digest": {
                          "type": "string"
                        },
                        "copyPreparationReport": {
                          "type": "string"
                        },
                        "canRestore": {
                          "type": "boolean"
                        }
                      }
                    },
                    "nativeReport": {
                      "type": "object"
                    },
                    "inputSnapshot": {
                      "type": "string"
                    },
                    "inputDigest": {
                      "type": "string"
                    },
                    "controlEvidencePath": {
                      "type": "string"
                    },
                    "controlEvidenceSha256": {
                      "type": "string"
                    }
                  },
                  "allOf": [
                    {
                      "if": {
                        "required": [
                          "state"
                        ],
                        "properties": {
                          "state": {
                            "const": "completed"
                          }
                        }
                      },
                      "then": {
                        "required": [
                          "state",
                          "code",
                          "cliExitCode",
                          "draft",
                          "nativeReport",
                          "inputSnapshot",
                          "inputDigest",
                          "controlEvidencePath",
                          "controlEvidenceSha256"
                        ]
                      }
                    }
                  ]
                }
              }
            }
          }
        }
      }
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

## nodeforge_graphs_preflight

检查当前 JSON、节点、依赖和目标能否生成。需要新 callId、当前修订，arguments 传空对象。此操作只检查并输出报告，不创建、编译、保存或预留资产。阅读 GraphPreflight 报告和诊断，通过后再明确调用 generate。

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

## nodeforge_graphs_generate

从当前已预检的草稿创建新资产。支持 Material、Actor Blueprint 和有限的 MaterialInstance；材质实例的 MCP 生成和保存尚未验证。需要新 callId、当前修订，以及未占用的 requestId 和 assetPath。

工具创建、编译和保存资产及请求收据，不覆盖已有目标。finished 仅表示 CLI 结束；检查原生 checks 和回读。waitingPersistence 按 [保存核验](Preparation.md)进行独立重开检查。

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
