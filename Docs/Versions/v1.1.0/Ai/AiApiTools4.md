# MCP 工具与返回 schema

有 handle 的工具使用 connected 返回的本工程/本会话句柄。pending/unknown 时查询原 callId，停止依赖写入。

## nodeforge_docs_read

通过精确pageId读取已绑定版次的完整UTF-8资料。 前提：无需UE/handle；从索引获取pageId，禁止猜任意本机文件。 参数：pageId: docs_index或节点族索引列出的精确pageId，1–128字符；不能传任意文件路径。 效果：只读受控页，每页最多65536字节，核SHA/bytes。 判据：status=passed，bundleId/pageId/text/sha256/docsManifestSha256；先AiQuickStart或DeveloperReadme。 资料pageId：Readme。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

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

从当前会话原报告按pointer取得可核验的原生子树。 前提：handle、已登记reportId、RFC6901 pointer；无需新callId/修订。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; reportId: 当前handle已登记的64位小写hex报告ID，来自原业务返回reports。; pointer: RFC6901 JSON pointer，最多1024字符；空串选择整报告，超限用返回导航逐段读。 /exportedRequest按64KiB完整Graph专用配方取得原text及sourceRef。 效果：只读原报告，最大1MiB；普通pointer选择32768字节，/exportedRequest专用分支交付完整原Graph text（UTF-8最大65536字节）与来源引用；不截断或让用户拼字段。 判据：passed给value；/exportedRequest额外给原text、payloadSha256和sourceRef，显式载入只取text及其中两个报告引用字段；其它超限给navigation/selectionReturned=false。 资料pageId：AiResultsAndRecovery, AiApiReference, AiAssetRead。当前版从docs_index确认；unknown/pending停写查原call。

输入schema：

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

按准确对象路径读取普通材质全局参数、MaterialFunction原生签名或Texture2D真实采样属性/原字段片段。 前提：当前handle、新callId和准确revision；kind=MaterialParameters|MaterialFunction|Texture2D，source完整Package.Asset。当前读取范围来自显式安装绑定，旧目录按原档案。源须已保存、clean且由正式链建立本进程保存来源；预加载无证明仍拒绝，不reload/resave。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：共用GT只读协调，写原报告；不改草稿／origin／revision／LastSuccess／生成收据／导出基线，不保存或编译源。 判据：原CLI finished/code0及operationStatus=passed/overallStatus=succeeded；report_read取/parameters、/functionSignature和/nodeTemplate，或/texture的尺寸/sRGB/compression/nativeSamplerType及properties.Texture/SamplerType。片段不能当完整Graph；读取/采样资格不替代最终原生编译。 资料pageId：AiResources, AiAssetRead, AiResultsAndRecovery, Material。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_graphs_export

从一个已知普通Material或Actor Blueprint导出完整受控Graph v2/v3，formatVersion遗漏默认2，新配方显式3；指定首次确定的新requestId／assetPath。 前提：当前handle、新callId和准确revision；assetKind为Material或Blueprint，formatVersion可选整数2/3；源Package.Asset，允许根、saved/clean，蓝图compiled；目标与项目request未占用，源与目标不同。先读AiAssetRead，不搜索枚举。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：只读原图，沿共享Exporter拒绝无法完整表达的语义；不改当前稿／基线／revision／LastSuccess，不预留或生成目标。 判据：读取原reports的reportId，再report_read pointer=/exportedRequest取得完整text/value/payloadSha256/sourceRef；用原text和sourceRef.exportReportPath/exportReportSha256显式draft.replace，预检、生成、新PID原CLI Verify分开。 资料pageId：AiAssetRead, AiPreparation, AiResultsAndRecovery, AiAssetSettings。当前版从docs_index确认；unknown/pending停写查原call。

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

## nodeforge_resources_query

在显式允许的内容子目录有界查找Texture2D、MaterialFunction或Material的登记元数据，返回准确对象路径。 前提：当前handle、新callId及准确revision；kind与directory必需；其余参数、默认值和上限从本版native argumentsSchema消费。拒绝根级全库扫描。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。; arguments: 精确操作参数；无参数也必须传空对象。; callId: 首次提交前保存的新调用ID，ASCII字母数字和连字符1–64；查询/恢复用原ID。; expectedDraftRevision: draft.get返回的当前uint64十进制字符串；busy/staleRevision停下查询，不自动改修订重试。 效果：原GT共享Service只访问有界文件/磁盘AssetRegistry元数据，写原报告，不加载对象、不建立来源证明、不改草稿/origin/revision/资产。 判据：原CLI/code0只证明本次查询返回；必须另看resourceQuery.complete、registryScanning、truncated、nextAfter及incompleteReason。未完成没有续页游标，缩小目录；完整页按精确路径游标，目录变化可能重复/遗漏。report_read的/resourceQuery/rows取得实际行，发现/政策允许、精确inspect与图可导出分别判断。 资料pageId：AiResources, AiPreparation, AiResultsAndRecovery。当前版从docs_index确认；unknown/pending停写查原call。

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



## nodeforge_disconnect

{
  "name": "nodeforge_disconnect",
  "operation": null,
  "inputSchema": {
    "type": "object",
    "properties": {
      "handle": {
        "type": "string",
        "minLength": 1
      }
    },
    "required": [
      "handle"
    ],
    "additionalProperties": false
  },
  "help": {
    "operation": null,
    "purpose": "正常释放本客户端一个准确handle的连接和租约，供另一对话重新连接。",
    "preconditions": "本客户端connect返回的当前handle；先核对自己的原调用，无pending/unknown、requiresReview或未退出CLI。",
    "effects": "复用共享close核心，只释放PID和instance匹配的自有租约并使该handle失效；不停止UE、不结束AI服务、不删原调用或报告。没有force参数。",
    "success": "finished且released/leaseReleased/handleInvalidated=true，recordsPreserved=true；未解决调用返回unknown并保留原handle和租约；他人handle或租约明确拒绝。另一客户端随后显式connect取得自己的新handle。",
    "guides": [
      "SessionHandoff",
      "AiResultsAndRecovery"
    ]
  },
  "description": "正常释放本客户端一个准确handle的连接和租约，供另一对话重新连接。 前提：本客户端connect返回的当前handle；先核对自己的原调用，无pending/unknown、requiresReview或未退出CLI。 参数：handle: connect返回的当前工程/原生会话opaque handle；关闭重开必须重新connect，旧句柄不能转到其他工程。 效果：复用共享close核心，只释放PID和instance匹配的自有租约并使该handle失效；不停止UE、不结束AI服务、不删原调用或报告。没有force参数。 判据：finished且released/leaseReleased/handleInvalidated=true，recordsPreserved=true；未解决调用返回unknown并保留原handle和租约；他人handle或租约明确拒绝。另一客户端随后显式connect取得自己的新handle。 资料pageId：SessionHandoff, AiResultsAndRecovery。当前版从docs_index确认；unknown/pending停写查原call。",
  "outputSchema": {
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
      "released": {
        "type": "boolean"
      },
      "leaseReleased": {
        "type": "boolean"
      },
      "handleInvalidated": {
        "type": "boolean"
      },
      "recordsPreserved": {
        "type": "boolean"
      },
      "bridgePid": {
        "type": "integer"
      },
      "identity": {
        "type": "object"
      }
    },
    "allOf": [
      {
        "if": {
          "properties": {
            "status": {
              "const": "finished"
            }
          }
        },
        "then": {
          "required": [
            "released",
            "leaseReleased",
            "handleInvalidated",
            "recordsPreserved",
            "unresolvedCalls"
          ]
        }
      }
    ]
  }
}
