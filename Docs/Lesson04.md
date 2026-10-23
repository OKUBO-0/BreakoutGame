# 01_04 ブロック耐久値・サウンド・レベル切り替え

資料: https://docs.google.com/presentation/d/1X8P94DKQgYNXBOwP-4E2I2Vvl-AUFQ4E6oVHw38mBZk/edit

## 作業内容

- ブロックにインスタンス編集可能な整数 `Hp`（クラス既定値1）を追加。ボールの衝突で1減らし、0になった時にだけ破壊・破壊数の加算を行う。同一フレームの重複コールバックは1回にまとめ、ボール以外の接触は無視する。
- `TextRender` でHPを表示。位置 `(-51,0,0)`、回転 `(Pitch=0,Yaw=180,Roll=0)`、中央揃え、黒、WorldSize 150。`ReloadHp` が開始時・建設時・被弾時に数字と色を更新する。
- `BlockMaterial` のVector Parameter `BaseColor` をBase Colorに接続。ブロックごとのDynamic Material Instanceを生成する。`ColorTable[Hp-1]` は1:白、2:水色、3:ピンク、4:薄赤、5:薄黄。範囲外なら色を変更せず、不正アクセスを避ける。
- ボールのHitイベントで `Breakout_SE_Knock` を2D再生する。壁・パドル・ブロックが対象。同一フレームの多重SEを抑止する。
- `Breakout_BGM_Cue` はLoopingノードとWavePlayerを接続。AmbientSoundを各ステージに1つ配置し、開始時に音量0.25で再生する。
- GameManagerにインスタンス編集可能な `NextLevelName` と `OpenNextLevel` を追加。SpaceのAction処理はゲームオーバー時:同じ面を再開、クリア時:次の面、通常時:発射と分岐する。
- `BreakoutGameInstance` の `LeftBallNum` は既定値-1。`IsValidBallNum` は0以上を有効とする。次の面へ移る直前に現在の残球+1を保存し、次のGameManagerが開始時に読み込む。クリア後にボールが落ちてもクリアに使った1球を戻す。
- ゲームオーバーからの再開では引き継ぎ値を-1へ戻し、古い残球数を再適用しない。次のレベルが未設定・存在しない場合は遷移も残球数変更もしない。

## レベル構成

既存の `L_InGame` を教材のLevel1相当として維持する。ユーザーの難易度調整依頼により、Level1は5個・すべてHP1へ変更（必要ヒット数30→5）。Y=-1200,-600,0,600,1200、Z=3000の1段配置として、従来のZ=4000/4400より手前に置く。カメラ・壁・パドル・ボールの白い材質と操作・速度は維持する。次のレベル名は `Level2`。初回のみ移行タグで反映し、以降の手動編集は維持する。

`Level2` は同じプレイ領域を複製し、10ブロックを1・2・3・4個の4段の三角形に配置して区別する。教材はLevel3を指定していないため、Level2の次のレベル名は未設定とする。Level2をクリアした後は結果画面にとどまる。あとから `NextLevelName` に有効なマップ名を設定すれば続行できる。

## 音源

教材では音源の持参を求めているため、外部サンプルを使わないオリジナルのPCM WAVを生成した。

- `SourceArt/Audio/Breakout_BGM.wav`: 44.1kHz / 16bit / mono / 16秒、120 BPMの8小節。柔らかいシンセ音とベース・打楽器による短いループ。
- `SourceArt/Audio/Breakout_SE_Knock.wav`: 約0.13秒の衝突音。
- 再生成: リポジトリのルートで `node Tools/generate_lesson04_audio.mjs`。
- Unreal上の音源・SoundCueは `Content/Breakout/Audio/` に保持する。

## 実装方針と検証

01_02・01_03と同様、ゲーム処理はBlueprintから呼べるC++で実装し、マテリアル・音源・SoundCue・GameInstance Blueprint・レベルは編集可能なアセットとして保存する。教材のノード図をそのまま複製したものではない。

`Breakout.Lesson04.HpAudioAndLevelTravel` はHP5の衝突・重複ガード・数字と色・配列の範囲外・BGMループ・衝突SEのオーディオデバイスでの再生・クリア・Level2への遷移・残球引き継ぎ・Level2での再開をPIE上で確認する。音の検証では元WAVの16秒を超えてBGMが再生中であることも確認する。既存の `Breakout.Lesson03.InputLivesAndRestart` を回帰テストとして維持する。

## 検証結果（2026-10-21）

- UE 5.8.3 / Development Editor のビルド成功。
- `Breakout.Lesson03.InputLivesAndRestart`: 成功。
- `Breakout.Lesson04.HpAudioAndLevelTravel`: 成功。残球1・発射中の1球からLevel2へ残球2を引き継ぎ、同一GameInstanceの継続・HUD更新・新しい面のブロック初期化・再開時の残球3への復帰を確認した。
- BGMは元WAVの16秒を超える17秒後も再生中。SEはボールのHitイベントを通じてオーディオデバイス上のActiveSoundとして再生されることを確認した。
- L_InGameとLevel2をPIEで表示し、HP数字・5色・白いパドル・UI・異なるブロック配置をMCPのエディタ画像で確認した。
- ゲームインスタンスのプロジェクト設定は生成済みの `BreakoutGameInstance` Blueprintを参照。パッケージ用のMapsToCookに両マップを追加した。パッケージ化そのものは今回実行していない。
- 詳細ログ: `Saved/Logs/Lesson04Check.log`（生成ファイルのためGit対象外）。

## 難易度調整後の検証（2026-10-21）

- Level1を5ブロック・HP1・Z=3000に変更してマップへ保存。Level2は変更しない。
- Development Editorビルド成功。`Breakout.Lesson03.InputLivesAndRestart` と `Breakout.Lesson04.HpAudioAndLevelTravel` がともに成功。
- HP5の検証はPIE内だけのテスト用ブロックとして行い、通常のLevel1のHPは1を維持する。5個破壊でのクリア、Level2の10ブロック・4段配置、残球引き継ぎ・再開を確認。
- 詳細ログ: `Saved/Logs/EasyIntroCheck.log`（Git対象外）。

## 実装結果GIF（2026-10-23）

- `Docs/Recordings/01_04.gif`: 約41秒、960×540。Level1の発射・5個の破壊・クリア表示・Level2への遷移・残球2→3の引き継ぎ・Level2のHPと色別配置を実機PIEで録画した。
- 録画時のみMCPでパドルをボールへ自動追従させ、一時Blueprintから発射／次の面へのActionを実行した。衝突・破壊・クリア・残球引き継ぎは通常のゲーム処理。ブロックの強制破壊やクリア状態の直接変更は行っていない。自動追従と一時Blueprintはゲーム本体には含めない。
- 録画中のウィンドウサイズ変更に合わせてゲーム表示部分を切り出し、GIFの寸法を統一した。GIF形式のため音声は含まれない。
- 2026-10-23にDevelopment Editorビルド成功を再確認。操作・再開・HP・音声・レベル切り替えの自動テスト結果は上記ログの通り。
