# 01_03 ボールのリスポーン、残数管理

資料: https://docs.google.com/presentation/d/17IFaJ9jNBJmuPE8cPnzC8ag9s5izNdo99NtCIZqzZFY/edit

## 最終仕様

- 初期配置のボールを取り除き、`RespawnLocationActor` を `(0, 0, 1000)` に配置する。
- GameManager のインスタンス編集可能な `SpawnLocationActor` に発射位置を指定する。
- `IA_Action` は Digital / Boolean、`IMC_InGame` で Space を割り当てる。
- Paddle の `PaddleActions` コンポーネントが Enhanced Input の `Started` を受け、GameManager の `Action` を呼ぶ。既存のパドル移動グラフは維持する。
- 開始時の残弾は3。Spaceを押すと1球発射し、生成に成功した時だけ残弾を1減らす。長押し・連打による多重生成は禁止する。
- ミスでボールが消滅すると発射制限を解除する。自動再発射はせず、次のSpace入力を待つ。
- 残弾0のボールも、場に残っている間はプレイを継続する。その球を失った時にゲームオーバーになる。クリア済みならゲームオーバーにしない。
- `GameInformation` の左上に `LeftBall : 3` を表示し、発射ごとに更新する。
- GameOver の文字は2秒で白から赤へ変化する。中央下に `Push SPACE to Restart`（フォント50）を表示する。
- UIは1920×1080の編集用キャンバスをScaleBoxで16:9に収め、PIEの黒帯に残数表示が出たり、再開案内がパドルと重なったりしないようにする。
- ゲームオーバー後にSpaceを押すと現在のレベルを開き直し、残弾3・全ブロック・結果UIなし・未発射の状態に戻す。

資料16〜27ページの「無限に自動リスポーン」、36〜42ページの入力ログ・多重発射は途中の演習段階。最終状態には残さない。資料63ページの `GetLeftBlockNum` は、前後の手順に合わせて `GetLeftBallNum` として実装する。ブロック配置・カメラ・壁・白いパドルとボールは前回の状態を維持する。

## 実装の場所

既存の01_02と同じく、C++の処理をBlueprintから呼べる形で追加している。

- `Source/BreakoutGame/Public/BreakoutLesson02.h`: 管理変数、発射・リセット関数、HUD・Paddle入力コンポーネント。
- `Source/BreakoutGame/Private/BreakoutLesson02.cpp`: 実際の発射、残弾更新、ミス判定、入力登録、レベル再読み込み。
- `Content/Breakout/Input/`: IA_Action / IMC_InGame。
- `Content/Breakout/UI/GameInformation`: 残数表示。
- `Content/Breakout/UI/GameOverWidget`: GameOverAnimation / PushSpaceTextBox。
- `Content/L_InGame`: SpawnLocationActorの参照と初期配置ボールの削除。

入力と残弾管理はネイティブ処理、UIレイアウト・アニメーション・入力設定・参照位置は編集可能なUnrealアセットで保持する。教材のBlueprintノード図をそのまま複製したものではない。

## 検証方法

EditorのAutomationから `Breakout.Lesson03.InputLivesAndRestart` を実行する。テストはPIE上でSpace入力、生成数、残弾とHUDの一致、多重発射抑止、MissAreaへの接触による消滅、3球目のゲームオーバー、Spaceによるレベル再読み込み、クリア優先を確認する。

ミス判定のテストではPIE内のボールのみをMissAreaへ移動する。保存済みレベルの座標は変更しない。

## 検証結果（2026-10-21）

- UE 5.8.3 / Development Editor ビルド成功。
- `Breakout.Lesson03.InputLivesAndRestart`: 成功。Aキーでのパドル移動、SpaceのStarted入力、1球制限、残弾・HUD更新、3回のMissArea接触、再開案内の文言とフォント50、GameOverAnimationのバインド、赤への変化、レベル再読み込み、10ブロック・残弾3への復帰、クリア優先を確認した。
- 30fpsの表示と2秒のアニメーションは維持し、内部の時間解像度を30,000 ticks/秒へ移行。終了直前を評価するUEの仕様による色の誤差を抑えた。最終色は `(1.0, 0.000017, 0.000017, 1.0)`。
- 画面確認はMCPのCaptureEditorImageで実施。動作テストと画像取得を分離し、画像読み出しの待ち時間による誤判定を避けている。
- 詳細ログ: `Saved/Logs/Lesson03FinalResult.log`（生成ファイルのためGit対象外）。

## 実装結果のGIF

`Docs/Recordings/01_03.gif`（約61秒）。開始時の残数3、発射による残数更新、自然落下による3回のミス、GameOverの白から赤への変化、リスタートによる残数・ブロックの復元を収録した。

録画はMCPによる自動操作。録画時だけ一時アクターから既存の `Action` / `SpawnBall` を呼び出した。ボール位置や残数の直接変更、製品コードの変更は行っていない。録画用アクターはレベルから取り除き、一時Blueprintも削除した。Space入力自体は上記PIE自動テストで別途検証している。
