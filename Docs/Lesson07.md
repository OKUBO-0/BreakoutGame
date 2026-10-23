# 01_07 タイトルレベルの作成と遷移

資料: https://docs.google.com/presentation/d/1ECSVyusgRIGNt4H2VqBGb7xGOofYZh5RcKrOkfNavPA/edit

## 作業内容

全14ページの本文と画像・ノード図を確認した。資料自体は変更していない。

- p.4〜7: Basicテンプレートから新しいレベルを作成し、`TitleLevel`として保存。
- p.8〜9: `TitleWidget`を作成。画面中央より上に白い大きな「ブロック崩し」、下部に白い`[PUSH SPACE]`を置く。完成例p.13に合わせて黒い細いアウトラインを付ける。数値指定がないため1920×1080の基準画面で文字サイズ160／64、中心位置38%／78%とし、画面サイズに合わせて全体を等比縮小する。
- p.10〜11: レベルBlueprintに`BeginPlay → Create TitleWidget → AddToViewport`を作成。
- p.12: レベルBlueprintで`Space Bar / Pressed → OpenLevel (by Name)`を接続。資料の`Level1`は、本プロジェクトで既存の第1ステージにあたる`L_InGame`へ読み替える。
- p.4の起動時にワンクッション置く目的に合わせ、GameDefaultMapとEditorStartupMapをTitleLevelへ変更。パッケージのマップ一覧にも追加する。

## 実装上の補足

`/Game/TitleLevel`と`/Game/Breakout/UI/TitleWidget`はUE上で編集可能なアセットとして作成する。既存ステージは変更しない。タイトル専用のGameModeでゲーム用パドルを生成せず、Basicレベルの床・空を固定カメラで映す。タイトルに入ったら残球の持ち越し値を未設定へ戻し、新しいゲームを3球から開始する。タイトルWidgetの表示とSpace遷移は教材通りレベルBlueprintに置く。

## 検証

`Breakout.Lesson07.TitleAndSpaceTravel`を追加。初期マップ設定、タイトル専用モード、パドル／Manager不在、Widgetが1個だけ画面にあること、日本語タイトルとSpace案内、Space以外で遷移しないこと、実際のSpace入力から第1ステージへの遷移、Widgetの消去、残球3・場の球0で開始、次のSpaceで通常発射することを確認する。Lesson03〜06の5件も回帰実行する。

## 検証結果（2026-10-23）

- UE 5.8.3 / Development Editorビルド成功。
- `Breakout.Lesson03.InputLivesAndRestart`: 成功。
- `Breakout.Lesson04.HpAudioAndLevelTravel`: 成功。
- `Breakout.Lesson05.AimingAndRealSweptHits`: 成功。
- `Breakout.Lesson05.Level2AimingAndRealSweptHits`: 成功。
- `Breakout.Lesson06.ItemMultiballAndCarryOver`: 成功。
- `Breakout.Lesson07.TitleAndSpaceTravel`: 成功。実際のPlayerControllerへのキー入力を使い、タイトルからの遷移とゲーム側の次の発射を確認した。
- 初回のタイトルテストは、UEが`GetGameDefaultMap()`でアセット名を除いたパッケージパスを返す点と期待値が不一致だった。期待値を修正して再ビルド・全6件を再実行し、成功を確認した。
- 完成例p.13と実画面を比較し、白い日本語タイトル・黒い輪郭・下部の案内・空と床の背景を確認。確認画面は`Saved/Lesson07/title-visible.png`、最終ログは`Saved/Logs/Lesson07Check2.log`に保存（Git対象外）。
- 既存のゲームマップ、ボール、パドル、アイテムのアセットに変更なし。パッケージ化は実行していない。

## 実装結果GIF

`Recordings/01_07.gif`（約35秒・960×540・音声なし）を保存した。タイトル画面から実際にSpaceキーを押して第1ステージへ遷移し、残球3・ボール未発射の状態を表示した後、次のSpaceキーでボールを発射する様子を収録している。ゲーム部分では録画補助としてパドルを自動追従させたが、ゲーム設定・レベル・Blueprintは変更していない。取得したフレームの時刻に合わせてGIFを生成した。コミット前のEditorビルドも成功した。
