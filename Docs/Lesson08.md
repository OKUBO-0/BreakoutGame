# 01_08 パッケージ化

資料: https://docs.google.com/presentation/d/1t1TVHdXjoz_TRg001DRhKfMz_VlqoDUHJv2F3nIqlBM/edit

## 作業内容

全26ページの本文・設定画像・ノード図を確認した。資料そのものは変更しない。

- p.6〜7: ゲームのデフォルトマップをTitleLevelにする。前回の設定を維持。
- p.9〜14: TitleLevelのレベルBlueprintで既存のAddToViewportの後にGetGameUserSettings、SetFullscreenMode（Windowed）、SetScreenResolution（1280×720）、ApplySettingsを接続する。ApplySettingsのコマンドライン上書き確認は画像通り有効。
- p.17〜19: エディタコンテンツ除外、Shipping、完全再ビルド、配布用を有効にする。PakとIoStoreを使用し、デバッグファイルは配布物に含めない。
- p.18: 本文の`Lvl_Korokoro`は別教材の記載。画像のLevel1・Level2も踏まえ、本プロジェクトではTitleLevel・L_InGame・Level2の3マップを明示的に含める。
- p.21〜25: Windows用にパッケージ化し、出力したexeでタイトルからの開始、ウィンドウ表示、1280×720、デバッグ表示がないこと、繰り返しプレイを確認する。

## 補足

`DefaultGameUserSettings.ini`にも1280×720・Windowedの初期値を設定し、タイトルのBeginPlayで同じ値を適用・保存する。Unreal MCPはNoRedist指定があるため、プロジェクトでEditorターゲットに限定し、配布物には含めない。エディタでのMCP接続は維持する。ゲームの難易度やステージ構成は変更しない。

初回Shipping版の実起動で、C++コンストラクタ内のパスから動的に読み込むBP_Ball・アイテム・HUD・入力・効果音がクック対象から漏れていることを配布マニフェストで確認した。`/Game/Breakout`（ゲーム用アセット19個）を追加のクック対象に指定し、生成時に確実に含める。全Contentのクックは行わない。

実際に第1ステージをクリアした際、短いマップ名をディスク検索する`SearchForPackageOnDisk`ではIoStore内の次レベルを発見できない問題を確認した。遷移先を現在のマップと同じフォルダの正式なパッケージパスへ解決し、`DoesPackageExist`で確認するよう修正した。すでに正式なパスが指定されている場合はそのまま使用する。

再現用コマンド（PowerShell）: `& .\Tools\Package-Windows.ps1`。出力先は既定で`Saved/Packages/01_08`。別の提出用フォルダ名は`-OutputDirectory`で指定できる。配布時はexe単体ではなくWindowsフォルダ全体を渡す。

## 検証記録

- Windows Shipping版のビルド・クック・配布フォルダ生成が成功。最終生成ログは`Saved/Lesson08/package-output4.log`。
- `Tools/Package-Windows.ps1 -VerifyOnly`で、起動exe・Shipping本体・必須マップとアセットの配布マニフェストを確認済み。MCPコンテンツは配布対象外。
- 最終配布版を実起動し、タイトル→SpaceでLevel1開始、1280×720のウィンドウ表示、白いパドル・ボール、HUD、発射とブロック破壊を確認。PrintStringなどのデバッグ表示は見られなかった。
- A/D長押しによる左右移動はユーザーの実操作でも確認済み。
- Level1をクリアし、SpaceでLevel2へ遷移することを最終配布版で確認。残りボール数2の引き継ぎと、HP別に色分けされたLevel2のブロックを確認した。
- エディタの回帰テスト6件が全成功。入力・残機・再開、HP・効果音・レベル遷移、照準・実衝突（両レベル）、アイテム・マルチボール・引き継ぎ、タイトルからの開始を検証。短いレベル名と正式パッケージパスの両方をテストする。ログは`Saved/Logs/Lesson08FinalCheck.log`。
- 資料p.25の「少なくとも2周」の通しプレイは未完了。配布版でのゲームオーバーからの再開も未確認であり、回帰テストの成功と実機での確認済み範囲を区別する。

出力: `Saved/Packages/01_08/Windows/BreakoutGame.exe`。配布時は`Windows`フォルダ全体が必要。
パッケージと生成ログはSaved以下に保持し、Git対象には含めない。
