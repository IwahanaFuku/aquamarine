# Aquamarine

OpenGL 3.3 + GLFW + ImGui を用いて構築中の  
**学習目的の 3D モデリング／ビューア基盤**です。

Blender 等の DCC ツールを参照しつつ、  
「3D ツールが内部で何をしているか」を理解することを目的にしています

---

## 特徴 / 目的

- OpenGL の **低レベル概念（VAO/VBO/EBO/FBO）を実装ベースで理解**
- GLFW + ImGui による **ツール系アプリケーション構成**
- 将来的なモデリング機能追加を見据えた **責務分離設計**
- 数学処理は GLM に統一（右手系 / OpenGL 流儀）

---

## 現在できること

### 描画
- グリッド描画（ライン）
- ワイヤーフレームキューブ
- インデックス付きメッシュ描画（EBO 使用）
- 選択面の半透明ハイライト描画

### カメラ
- Orbit Camera
  - 左ドラッグ：回転（yaw / pitch）
  - 中ドラッグ：パン（カメラ基準）
  - ホイール：ズーム
- `glm::lookAt` + `glm::perspective` 使用

### 入力
- GLFW コールバックによるマウス入力管理
- ImGui と入力の競合を考慮（`WantCaptureMouse`）

### ピッキング
- FBO + 整数テクスチャ（`GL_R32UI`）による **ID バッファ方式**
- 面単位（Cube 6面）での選択判定

---

## 使用技術

| 種類 | 内容 |
|----|----|
| Language | C++20 |
| Graphics | OpenGL 3.3 Core |
| Window / Input | GLFW |
| UI | Dear ImGui |
| Math | GLM |
| Build | CMake |
| Platform | Windows（Visual Studio） |

---

## ビルド方法

### 依存関係
- GLFW（FetchContent）
- GLM（FetchContent）
- Dear ImGui（FetchContent）
- glad（vendor）

### ビルド手順（例）

```bash
cmake -S . -B out/build/debug
cmake --build out/build/debug
ビルド後、assets/ ディレクトリは自動的に実行ファイル横へコピーされます。
```

## ディレクトリ構成
```
src/
├─ app/            # アプリ全体の制御（最薄）
├─ camera/         # OrbitCamera 実装
├─ platform/       # GLFW / ImGui / 入力管理
├─ render/         # 描画・メッシュ・ピッキング
│  ├─ geometry_gen # CPU側ジオメトリ生成
│  ├─ mesh         # VAO/VBO/EBO 管理
│  ├─ renderer     # 描画パス
│  └─ picker       # FBO ピッキング
assets/
└─ shaders/        # GLSL（vertex/fragment 統合）
```

## 設計方針
### 責務分離
- Platform
  - GLFW 初期化
  - ウィンドウ / 入力管理
- Renderer
  - 描画のみ（状態を持たない）
- Picker
  - ピッキング専用 FBO / Shader 管理
- Camera
  - 入力 → 行列変換の責務のみ
- App
  - 各システムの接着剤（ロジックを持たない）

### OpenGL 資源管理
- VAO / VBO / EBO / FBO / Shader は RAII 風管理
- init() / destroy() 明示
- 二重 delete 安全

---
## シェーダー設計
- vertex / fragment を 1ファイルに統合
- #define VERTEX / #define FRAGMENT により分岐
- C++ 側で define を注入してビルド

```glsl
#version 330 core

#ifdef VERTEX
// vertex shader
#endif

#ifdef FRAGMENT
// fragment shader
#endif
```

## TODO
- 一般メッシュ対応（position / normal / uv）
- 法線可視化
- 複数オブジェクト管理
- トランスフォーム（translate / rotate / scale）
- 面・辺・頂点選択
- シーン構造（Scene / Node）
- Undo / Redo
- OBJ, FBX書き出し
- スキンドメッシュ対応
- パーティクル対応

## ライセンス
MIT License