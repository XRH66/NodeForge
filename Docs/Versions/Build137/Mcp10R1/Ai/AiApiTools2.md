> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 正式MCP工具与原结果

NodeForge把受控JSON转换为可编辑UE资产。先调用nodeforge_docs_index，再用nodeforge_docs_read读取Readme、AiQuickStart和FeatureMap，确认bundle/Build/桥/DLL/目录身份。资料无需UE；业务需要操作者显式准备并注册当前专用编辑器，再connect。只开放契约中的工具，不支持全部UE节点、循环、数组或生产写入。调用完成和资产最终核验不同；waitingPersistence需原CLI新PID Verify。pending/unknown立即停写，查询原calls.get，不换ID重发。开发原理/源码/扩展/发布见DeveloperReadme。


## nodeforge_draft_restore

恢复服务保留的上一份可恢复输入。 前提：新callId及准确修订；必须有可恢复草稿。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：修改草稿/修订，不恢复资产磁盘或重放生成。 判据：读恢复后的真实草稿；旧结果不归给新稿。 资料pageId：AiResultsAndRecovery, UserInterfaceGuide。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_draft_prepareCopy

为当前受控Material/Actor Blueprint草稿正式准备新副本，并提交完整派生稿。 前提：新callId、准确expectedDraftRevision、arguments:{}；当前合法Graph v2两族。源稿已导出载入时沿当前可信来源核报告/实际saved source。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：仅改requestId/assetPath并写准备记录，提交成功revision+1；不预约、不生成资产或收据，原资产/输入文件保持。完整稿由draft.get读取。 判据：核原nativeSummary、draftApplied=true、前后revision与copyPreparation，再draft.get核完整稿及真实新身份；不能把native报告写出当提交成功。unknown/pending停写查原call/draft；生成仍须preflight再核目标。 资料pageId：AiCopyPreview, UserCopyPreview, AiResultsAndRecovery。当前版从docs_index确认；unknown/pending停写查原call。

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

比较当前草稿与其已绑定可信源图，读取精确结构与布局差异。 前提：新callId、当前expectedDraftRevision、arguments:{}；当前draft有可靠ExportOrigin，不能另传previousReport。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：写原报告并更新展示；不改稿/revision/准备关系或资产，不预约目标；差异不证明编译、运行或画面。 判据：原CLI及native operationStatus=passed；从reports登记reportId，report_read /graphDifference及子树取得完整差异。源/原报告/目标冲突拒绝，编辑后旧结果stale；unknown/pending停写查原call。 资料pageId：AiCopyPreview, UserCopyPreview, AiResultsAndRecovery, AiAssetSettings。当前版从docs_index确认；unknown/pending停写查原call。

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

检查当前受控图JSON、对象、节点和目标是否可生成。 前提：已准备唯一目标/request的新草稿；新callId及准确修订。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：只检查/出报告；不生成、编译、保存或预留名称。 判据：原GraphPreflight预检结论及诊断；通过后才明确generate。 资料pageId：AiTaskRecipes, GraphAuthoring, AiAssetSettings。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_graphs_generate

从当前已预检的同一草稿新建可编辑Material、Actor Blueprint或受限MaterialInstance。 前提：操作者分配唯一身份、工程独占与真实依赖；新callId及准确修订。实例MCP实测未做。 参数：handle: connect返回的当前桥会话opaque handle；不得复用历史值。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：新建、编译、保存新资产及请求收据；同完整目标/request拒绝，原资产不覆盖。 判据：finished仅CLI终态；原Import通常waitingPersistence，必须看checks/readback并另做新PID Verify。 资料pageId：AiTaskRecipes, AiResultsAndRecovery, AiPreparation, AiAssetSettings。当前版从docs_index确认；unknown/pending停写查原call。

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

