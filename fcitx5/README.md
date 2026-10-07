# Meltype for Fcitx5（試用版）

LinuxでMeltypeをFcitx5の入力方式として使うフロントエンドです。上流のNativeAOTコアとMozcヘルパーを使い、入力判定や辞書は独自に置き換えていません。Omarchy / Hyprland / Arch Linux ARM上で作成・検証しました。

このforkの変更は、CodexをFull Accessにした環境で対話しながら作成・適用しています。上流のMeltypeは https://github.com/yksr-melt/Meltype 。公式のFcitx5フロントエンドではありません。

## 動作

- 英語・日本語の混在入力、変換候補、文節表示、候補クリック。
- 最初の子音から未確定文字をFcitx5のポップアップに表示。アプリ本文へのインライン表示は使いません。確定した文字だけ本文へ入ります。
- 変換候補は縦9件ずつ。10件目以降でも表示ページが選択位置に追従します。
- PageDown / PageUpで9候補ずつ進む／戻る。候補ウィンドウのページ操作でも後の候補を表示できます。
- Ctrl・Alt・Superを含むキー操作とキー解放は変換せずアプリへ渡します。デスクトップ側のショートカットが先に処理される場合はその設定に従います。Shiftは大文字入力などに使います。
- パスワード・機密入力欄では入力を処理しません。
- 通信処理なし。Fcitx5の通知アイコンなどの既存設定を変更しません。

| キー | 動作 |
|---|---|
| Space / ↓ | 変換、または次の候補 |
| ↑ | 前の候補 |
| PageDown / PageUp | 変換中に9件ずつ候補移動 |
| ← / → | 文節選択 |
| Enter | 確定 |
| Esc | Meltypeコアの変換取消処理 |
| F6 / F7 / F9 / F10 | ひらがな／カタカナ／全角英字／半角英字 |

## 前提

Fcitx5がすでにデスクトップで動く環境が対象です。確認した組み合わせはFcitx5 5.1.23、.NET SDK 10.0.401、Bazelisk 1.29.0、aarch64。x86_64用ビルドの分岐もありますが、このforkの動作確認はaarch64で行っています。Windows専用のアプリ判定・設定画面・自動更新は移植していません。

必要なもの：.NET 10 SDK、C++20対応コンパイラー、clang、zlib開発ファイル、pkg-config、Fcitx5Coreヘッダー、jsoncpp、Git、Bazelisk。ArchではFcitx5のヘッダーは`fcitx5`パッケージに含まれます。他ディストリビューションでは開発用パッケージも必要です。

```bash
sudo pacman -S --needed base-devel git fcitx5 fcitx5-configtool jsoncpp clang zlib curl
# 別途、CPUに対応した.NET 10 SDKを導入してください。
dotnet --list-sdks

git clone https://github.com/masuidrive/Meltype.git
cd Meltype
```

Bazeliskは公式リリースなどから用意し、`bazelisk`の絶対パスを`BAZEL`へ指定します。ARM用は`bazelisk-linux-arm64`、x86_64用は`bazelisk-linux-amd64`です。

```bash
BAZEL="/absolute/path/to/bazelisk" bash native/mozc/build-mozc-helper.sh "$HOME/.local/src/meltype-mozc" "$HOME/.cache/meltype-bazel"
bash fcitx5/build.sh
```

Mozcの初回ビルドでは依存関係のダウンロードと時間が必要です。スクリプトは上流の`native/mozc/MOZC_COMMIT`で版を固定します。Fcitx5アダプターだけの再ビルドは`bash fcitx5/build-adapter.sh`。

## インストールと既定化

未確定文字を先に確定し、Fcitx5を終了してから導入します。実行中の共有ライブラリーを上書きしないためです。

```bash
fcitx5-remote -e
bash fcitx5/install.sh
```

アドオンライブラリーは`pkg-config --variable=libdir Fcitx5Core`配下の`fcitx5`へ配置します。異なる配置の環境では`FCITX5_ADDON_DIR=/path/to/addons bash fcitx5/install.sh`を使います。システムライブラリーの導入だけsudoを使用します。

再ログインし、`fcitx5-configtool`でMeltypeを追加・選択してください。日本語入力を常にMeltypeにする場合は、普段のグループを`keyboard-us`と`meltype`にし、Mozcなど他の日本語入力方式をグループから外します。復旧用のパッケージ自体は残してかまいません。保存された`~/.config/fcitx5/profile`の`DefaultIM=meltype`を確認してください。

Fcitx5を使う既存のGTK / Qt / Wayland / X11入力設定は維持します。IBusへ切り替えません。Fcitx5の設定ファイルを編集する場合、必ず終了後にバックアップしてから変更してください。終了時に設定が保存されます。

配置先：

- `~/.local/share/meltype/`：NativeAOTライブラリー、Mozcヘルパー、ライセンス
- `~/.local/share/fcitx5/addon/meltype.conf`：アドオン定義
- `~/.local/share/fcitx5/inputmethod/meltype.conf`：入力方式定義
- `/usr/lib/fcitx5/libfcitx5-meltype.so`：Archでのアダプター配置
- `~/.local/share/Meltype/`：設定・学習データ（大文字M、通常のXDGデータホームの場合）

## 検証

```bash
bash fcitx5/build-adapter.sh  # 全34候補のページ追従と前後のページ操作
# インストール後。以下は別DBusセッションで動き、通常のFcitx5を置き換えません。
bash fcitx5/test-isolated.sh
# コアのテスト
dotnet run --project src/Meltype.Core.Tests -c Release
```

入力テストは一時的な設定・学習ディレクトリーで英語`hello`、日本語`nihongo`の確定、修飾キーの素通し、変換中のページキーを確認します。Fcitx5はインストール済みアダプターを読みます。修正したソースを検証するときは先に再ビルド・再導入してください。ポップアップの見た目とアプリごとの挙動は実際の入力欄でも確認してください。

## 戻す方法

Fcitx5の設定ツールでMozcなどをグループに戻して選択し、Meltypeを外してください。アンインストール時はFcitx5終了後に上記のアダプター・定義・ランタイムを削除します。設定・学習データは必要に応じて残します。このforkはGPL-3.0-or-later。上流のLICENSEとTHIRD-PARTY-NOTICES、Mozcのライセンスを同梱・保持します。
