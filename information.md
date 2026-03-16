> **用途**：將此文件作為 `CLAUDE.md` 或系統提示詞，餵給 Claude Code，讓它在改造 Godot 原始碼時擁有完整的技術藍圖與指導方針。

---

## 一、專案背景與目標

你正在改造從 GitHub 下載的 Godot Engine 原始碼（MIT License），目標是打造一個**對 AI 輔助開發（特別是 Claude Code）高度友善**的 Godot 分支版本。改造涵蓋以下核心方向：

1. **內建 MCP Server**：讓 Godot 編輯器本身就是一個 MCP（Model Context Protocol）伺服器，Claude Code 可直接連接操控
2. **CLI / Headless 強化**：強化無頭模式與命令列介面，讓 Claude Code 不依賴 GUI 就能驅動引擎
3. **GDScript 工具鏈強化**：提升 GDScript 的 LSP、靜態分析、自動補全能力，讓 AI 產生的程式碼品質更高
4. **結構化輸出**：所有錯誤、日誌、場景資訊都輸出為機器可讀的 JSON/結構化格式
5. **熱重載與即時回饋迴路**：改善腳本與場景的熱重載機制，縮短 AI 迭代週期

---

## 二、Godot 原始碼架構速覽

改造前你需要理解 Godot 的模組化架構：

```
godot/
├── core/           # 核心引擎（Object 系統、Variant、記憶體管理）
├── servers/        # 抽象伺服器（RenderingServer, PhysicsServer 等）
├── scene/          # 場景系統（Node, SceneTree, 所有節點類型）
├── editor/         # 編輯器（EditorNode, EditorPlugin, Inspector）
├── modules/        # 可插拔模組（gdscript/, mono/, websocket/ 等）
├── platform/       # 平台後端（linuxbsd/, windows/, macos/）
├── drivers/        # 圖形驅動（vulkan/, gles3/）
├── thirdparty/     # 第三方庫
├── main/           # 引擎入口點（main.cpp）
└── tests/          # 單元測試
```

**關鍵擴展點**：
- `modules/` 目錄是加入新功能的首選位置，每個模組有獨立的 `register_types.cpp`
- `editor/plugins/` 可加入編輯器插件
- `core/io/` 處理所有 I/O 與網路通訊
- `modules/gdscript/` 包含完整的 GDScript 解析器、分析器、編譯器與虛擬機

---

## 三、改造任務清單與技術指引

### 任務 A：內建 MCP Server 模組

**目標**：在 `modules/` 下建立 `mcp_server` 模組，讓 Godot 編輯器啟動時自動監聽 MCP 連線。

**技術細節**：

```
modules/mcp_server/
├── register_types.cpp/h    # 模組註冊入口
├── mcp_server.cpp/h        # WebSocket/stdio MCP 協議實作
├── mcp_tools.cpp/h         # 暴露給 AI 的工具定義
├── mcp_resources.cpp/h     # 暴露給 AI 的資源定義
├── SCsub                   # SCons 建構腳本
└── config.py               # 模組配置
```

**需實作的 MCP Tools（JSON-RPC 2.0）**：

1. **場景操作類**
   - `create_node(parent_path, type, name, properties)` — 建立節點
   - `delete_node(node_path)` — 刪除節點
   - `modify_node(node_path, properties)` — 修改節點屬性
   - `get_scene_tree()` — 取得完整場景樹（JSON）
   - `save_scene(path)` / `load_scene(path)` — 場景讀寫

2. **腳本操作類**
   - `create_script(path, content, language)` — 建立腳本
   - `edit_script(path, content)` — 覆寫腳本內容
   - `validate_script(path)` — 語法與語義驗證，回傳結構化錯誤
   - `get_script_symbols(path)` — 取得函數、變數、信號清單
   - `attach_script(node_path, script_path)` — 附加腳本到節點

3. **專案操作類**
   - `get_project_structure()` — 回傳 res:// 下的完整檔案樹
   - `get_project_settings()` — 取得 project.godot 設定
   - `set_project_setting(key, value)` — 修改專案設定

4. **執行與除錯類**
   - `run_project()` / `stop_project()` — 啟動/停止遊戲
   - `run_scene(path)` — 執行特定場景
   - `get_debug_output()` — 取得最近的 stdout/stderr
   - `get_errors()` — 取得腳本錯誤（結構化 JSON）
   - `execute_expression(expression)` — 在執行時期求值表達式
   - `set_breakpoint(script_path, line)` — 設定斷點

5. **編輯器操作類**
   - `take_screenshot()` — 擷取編輯器/遊戲視窗截圖（base64）
   - `get_editor_state()` — 當前選取的節點、開啟的場景等
   - `execute_menu_item(menu_path)` — 執行選單命令

**實作提示**：
- 使用 `core/io/websocket_server.h`（Godot 已有 WebSocket 支援）做為傳輸層
- MCP 協議走 JSON-RPC 2.0，參考 https://modelcontextprotocol.io/specification
- 在 EditorPlugin 的 `_enter_tree()` 中啟動 MCP server
- 預設監聽 `localhost:6550`，透過 EditorSettings 可調整
- 所有工具回傳值必須是結構化 JSON，不可有純文字敘述

**config.py 範例**：
```python
def can_build(env, platform):
    return True  # 所有平台都啟用

def configure(env):
    pass

def get_doc_classes():
    return ["MCPServer", "MCPTool"]
```

---

### 任務 B：CLI / Headless 模式強化

**目標**：讓 Claude Code 能透過命令列完成大部分開發任務而不需要開啟 GUI。

**需強化的命令列旗標**（修改 `main/main.cpp`）：

```bash
# 現有的 headless 模式
godot --headless --path /project

# 新增的 AI 友善旗標
godot --headless --path /project --mcp-stdio          # 透過 stdin/stdout 做 MCP
godot --headless --path /project --mcp-port 6550      # 透過 WebSocket 做 MCP
godot --headless --path /project --validate-scripts    # 批次驗證所有腳本
godot --headless --path /project --export-scene-json scene.tscn  # 場景轉 JSON
godot --headless --path /project --import-scene-json scene.json  # JSON 轉場景
godot --headless --path /project --run-tests           # 執行 GDScript 單元測試
godot --headless --path /project --lint-gdscript       # GDScript 靜態分析
godot --headless --path /project --query "get_all_nodes_of_type(CharacterBody3D)"
```

**結構化輸出格式**：
所有 CLI 輸出統一為 JSON Lines（每行一個 JSON 物件）：

```json
{"type": "info", "timestamp": "2026-03-12T10:00:00Z", "message": "Project loaded", "data": {"scenes": 12, "scripts": 34}}
{"type": "error", "timestamp": "2026-03-12T10:00:01Z", "source": "res://player.gd", "line": 42, "message": "Invalid operand", "code": "PARSE_ERROR"}
{"type": "warning", "timestamp": "2026-03-12T10:00:01Z", "source": "res://enemy.gd", "line": 15, "message": "Unused variable 'speed'", "code": "UNUSED_VAR"}
{"type": "result", "timestamp": "2026-03-12T10:00:02Z", "command": "validate-scripts", "success": true, "data": {"passed": 33, "failed": 1, "warnings": 5}}
```

**實作位置**：
- `main/main.cpp` — 新增命令列參數解析
- 新建 `core/io/structured_logger.cpp/h` — 結構化日誌系統
- 修改 `core/error/error_macros.h` — 讓 ERR_PRINT 系列宏支援結構化輸出

---

### 任務 C：GDScript 工具鏈強化

**目標**：讓 Claude Code 產生的 GDScript 品質更高、錯誤更少。

#### C1: 增強 LSP（Language Server Protocol）

修改 `modules/gdscript/language_server/`：

- 新增 **semantic tokens** 支援（讓 AI 理解程式碼結構）
- 改善 **hover 資訊**：包含完整的方法簽名、預設值、文件說明
- 強化 **diagnostics**：將所有警告分類並給出修復建議（JSON 格式）
- 新增 **code actions**：自動修復常見問題（缺少型別標註、未使用變數等）
- 新增 **inline hints**：推斷的型別、參數名稱

#### C2: 靜態分析器強化

在 `modules/gdscript/` 下新增 `gdscript_linter.cpp/h`：

```
規則清單（可在 project.godot 中配置啟用/停用）：
- UNTYPED_DECLARATION  — 變數/參數缺少型別標註
- UNUSED_VARIABLE      — 未使用的變數
- UNUSED_SIGNAL        — 已宣告但未連接的信號
- SHADOWED_VARIABLE    — 變數遮蔽外層作用域
- UNSAFE_CAST          — 不安全的型別轉換
- MISSING_RETURN_TYPE  — 函數缺少回傳型別
- CYCLOMATIC_COMPLEXITY — 函數複雜度超過閾值
- MISSING_DOCSTRING    — 公開函數缺少文件註解
- SIGNAL_NAMING        — 信號命名不符合 snake_case
- NODE_PATH_LITERAL    — 硬編碼的節點路徑（建議用 @export 或 %UniqueNode）
```

輸出格式：
```json
{
  "file": "res://player.gd",
  "diagnostics": [
    {
      "rule": "UNTYPED_DECLARATION",
      "severity": "warning",
      "line": 5,
      "column": 4,
      "message": "Variable 'speed' has no type annotation",
      "suggestion": "var speed: float = 300.0",
      "auto_fixable": true
    }
  ]
}
```

#### C3: GDScript API 自動文件產生器

新增指令 `--dump-api-json`：

```bash
godot --headless --dump-api-json api.json
```

輸出所有內建類別、方法、屬性、信號的完整 JSON，格式：
```json
{
  "classes": [
    {
      "name": "CharacterBody3D",
      "inherits": "PhysicsBody3D",
      "properties": [
        {"name": "velocity", "type": "Vector3", "description": "...", "default": "Vector3(0,0,0)"}
      ],
      "methods": [
        {
          "name": "move_and_slide",
          "return_type": "bool",
          "arguments": [],
          "description": "..."
        }
      ],
      "signals": [...]
    }
  ]
}
```

這讓 Claude Code 可以在 CLAUDE.md 中引用完整的 API 文件，大幅提升程式碼產生的準確性。

---

### 任務 D：場景格式的雙向 JSON 轉換

**目標**：讓 `.tscn` / `.tres` 與 JSON 可以互相轉換，AI 更容易讀寫場景。

**實作位置**：`core/io/` 下新增 `resource_format_json.cpp/h`

**JSON 場景格式設計**：
```json
{
  "format_version": 1,
  "type": "PackedScene",
  "resources": {
    "ext_1": {"type": "Texture2D", "path": "res://icon.svg"}
  },
  "nodes": [
    {
      "type": "Node2D",
      "name": "Root",
      "properties": {"position": [100, 200]},
      "children": [
        {
          "type": "Sprite2D",
          "name": "Player",
          "properties": {
            "texture": "@ext_1",
            "position": [0, 0]
          },
          "script": "res://player.gd"
        }
      ]
    }
  ],
  "connections": [
    {
      "signal": "body_entered",
      "from": "Root/Area2D",
      "to": "Root",
      "method": "_on_area_body_entered"
    }
  ]
}
```

**CLI 指令**：
```bash
godot --headless --tscn-to-json input.tscn output.json
godot --headless --json-to-tscn input.json output.tscn
```

---

### 任務 E：熱重載與即時回饋迴路

**目標**：Claude Code 修改檔案後，引擎能立即反應。

**需改造的機制**：

1. **GDScript 熱重載**（`modules/gdscript/gdscript.cpp`）：
   - 檔案變更時自動重載，不需手動按 Ctrl+Shift+R
   - 重載結果透過 MCP 回傳（成功/失敗/錯誤明細）
   - 保留遊戲狀態的增量重載（而非完整重啟）

2. **場景熱重載**（`scene/main/scene_tree.cpp`）：
   - `.tscn` 檔案變更時自動更新場景樹
   - 差異化更新（只更新變更的節點）

3. **資源熱重載**（`core/io/resource_loader.cpp`）：
   - 圖片、音效等資源檔案變更時自動重載

4. **MCP 事件推送**：
   - 當 AI 修改檔案觸發重載時，MCP server 主動推送事件：
   ```json
   {"event": "script_reloaded", "path": "res://player.gd", "success": true, "errors": []}
   {"event": "scene_updated", "path": "res://main.tscn", "nodes_changed": ["Player", "Enemy"]}
   ```

---

### 任務 F：測試框架強化

**目標**：讓 Claude Code 可以撰寫與執行 GDScript 單元測試。

在 `modules/` 下建立 `gdtest` 模組：

```gdscript
# res://tests/test_player.gd
extends GDTest

func test_player_moves_right():
    var player = preload("res://player.tscn").instantiate()
    add_child(player)
    player.input_direction = Vector2.RIGHT
    await process_frames(5)
    assert_gt(player.position.x, 0.0, "Player should move right")
    player.queue_free()

func test_player_takes_damage():
    var player = Player.new()
    player.health = 100
    player.take_damage(25)
    assert_eq(player.health, 75, "Health should decrease by damage amount")
```

**CLI 執行**：
```bash
godot --headless --path /project --run-tests res://tests/
```

**JSON 輸出**：
```json
{
  "test_suite": "res://tests/test_player.gd",
  "results": [
    {"test": "test_player_moves_right", "status": "passed", "duration_ms": 120},
    {"test": "test_player_takes_damage", "status": "failed", "duration_ms": 5,
     "assertion": "assert_eq", "expected": 75, "actual": 80,
     "message": "Health should decrease by damage amount"}
  ],
  "summary": {"total": 2, "passed": 1, "failed": 1, "skipped": 0}
}
```

---

## 四、建構配置

**SCons 建構指令**（確保新模組一起編譯）：

```bash
# 完整版（含編輯器 + MCP Server）
scons platform=linuxbsd target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes -j$(nproc)

# Headless 版（CI/CD 或純 CLI 使用）
scons platform=linuxbsd target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes vulkan=no opengl3=no headless=yes -j$(nproc)

# 匯出模板（最終遊戲不含開發工具）
scons platform=linuxbsd target=template_release module_mcp_server_enabled=no module_gdtest_enabled=no -j$(nproc)
```

---

## 五、CLAUDE.md 配置範本

以下是改造完成後，遊戲開發專案中應使用的 CLAUDE.md：

```markdown
# Godot Project — Claude Code 配置

## 引擎資訊
- Godot 版本：4.x-claude-fork
- MCP Server 預設埠：6550
- 語言：GDScript（啟用嚴格型別模式）

## 編碼規範
- 所有變數必須有型別標註：`var speed: float = 300.0`
- 所有函數必須有回傳型別：`func get_health() -> int:`
- 信號使用 snake_case：`signal health_changed(new_health: int)`
- 節點引用優先使用 %UniqueNode 而非硬編碼路徑
- 場景中的可配置參數使用 @export

## MCP 工作流
1. 修改 .gd 檔案後會自動觸發熱重載
2. 使用 `get_errors()` 確認是否有語法錯誤
3. 使用 `run_project()` 測試遊戲
4. 使用 `take_screenshot()` 確認視覺結果
5. 使用 `--run-tests` 執行單元測試確認功能正確

## 專案結構
- res://scenes/     — 場景檔案
- res://scripts/    — GDScript 腳本
- res://resources/  — 自訂資源
- res://tests/      — 單元測試
- res://assets/     — 美術/音效資源

## 常用建構指令
- 驗證所有腳本：`godot --headless --path . --validate-scripts`
- 執行測試：`godot --headless --path . --run-tests res://tests/`
- 靜態分析：`godot --headless --path . --lint-gdscript`
- 匯出 API 文件：`godot --headless --dump-api-json api.json`
```

---

## 六、改造優先順序建議

| 優先級 | 任務 | 理由 |
|--------|------|------|
| P0 | CLI 結構化輸出 | 基礎設施，所有其他功能都依賴它 |
| P0 | MCP Server 模組（基礎框架） | Claude Code 連線的核心通道 |
| P1 | 場景操作 MCP Tools | AI 能操控場景是最大的生產力提升 |
| P1 | 腳本驗證 + 錯誤回傳 | 讓 AI 能自我修正程式碼錯誤 |
| P1 | GDScript Linter | 提升 AI 產生程式碼的品質 |
| P2 | JSON 場景格式 | AI 更容易讀寫場景結構 |
| P2 | 熱重載強化 | 縮短 AI 迭代週期 |
| P2 | 測試框架 | 讓 AI 能驗證自己的產出 |
| P3 | LSP 強化 | 改善即時開發體驗 |
| P3 | API 文件產生 | 提升 AI 的上下文品質 |

---

## 七、注意事項與限制

1. **執行緒安全**：MCP Server 在獨立執行緒運行，操作場景樹時必須使用 `call_deferred()` 或 `MessageQueue`
2. **安全性**：MCP Server 預設只監聽 localhost，生產環境不應暴露
3. **相容性**：所有改造都以模組形式加入，不修改核心 API，確保能跟隨上游更新 rebase
4. **匯出模板**：MCP Server 和測試框架在匯出模板中應被排除（`module_xxx_enabled=no`）
5. **記憶體**：MCP 回傳大型場景樹時需注意記憶體用量，支援分頁查詢
6. **GDExtension 相容**：確保新模組不影響 GDExtension 的 ABI 穩定性

---

## 八、參考資源

- Godot 原始碼架構文件：https://docs.godotengine.org/en/stable/engine_details/architecture/
- MCP 協議規範：https://modelcontextprotocol.io/specification
- 現有 Godot MCP 社群實作（參考但不直接使用）：
  - https://github.com/Coding-Solo/godot-mcp
  - https://github.com/ee0pdt/Godot-MCP
  - https://gdaimcp.com/
- Godot 模組開發文件：https://docs.godotengine.org/en/stable/contributing/development/core_and_modules/custom_modules_in_cpp.html
- Claude Code MCP 整合文件：https://code.claude.com/docs/en/overview
