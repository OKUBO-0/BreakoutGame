# 01_06 アイテム

資料: https://docs.google.com/presentation/d/16bx0uiPu2CGR05xNO0VEXrRQlsJvTZ7DaIdakwiI4Q0/edit

## 確認した内容

全55ページの本文・表・画面例・ノード図を確認した。資料そのものは変更していない。

- p.5〜24: Item・Ball・MissAreaのオブジェクトチャンネルと同名プリセットを追加する。ItemはPawnとMissAreaにOverlap、それ以外はIgnore。BallはBallとItemをIgnore、MissAreaをOverlap、それ以外をBlock。MissAreaはBallとItemのみOverlap。PawnはItemをOverlapにする。壁・ブロックはWorldDynamicを使用する。
- p.25〜38: 複数ボールを整数`InGameBallNum`で管理し、ミスでは1個だけ減らす。ゲームオーバーは残球と場の球数が両方0の時だけ。クリア後は場の球数を減らさず、次のレベルには`LeftBallNum + InGameBallNum`を渡す。動作確認用のSpace連射は最後に元に戻す。
- p.39〜40: `GenerateBall`を分離し、アイテムからは残球を減らさずに追加発射できるようにする。通常の`SpawnBall`は場に球がなく残球がある場合だけ発射する。
- p.42〜49: SphereをRootにした`AddBallItem`を作り、正面に黒いAを表示する。SphereのComponentTagsにItemを追加。時間に比例して落下し、Playerタグのコンポーネントに触れたら追加発射して消滅、MissAreaなら消滅だけ行う。確認用の配置物は残さない。
- p.51〜54: ブロックが壊れた時だけ確率でアイテムを生成し、ブロック自身のActorTransformを使う。本文に10%と50%が混在するが、後半の指示とp.52〜53の設定画像がWeight=0.5なので50%を採用した。

## 実装

- Item=GameTraceChannel1、Ball=GameTraceChannel2、MissArea=GameTraceChannel3を`DefaultEngine.ini`に追加。既存のPawnプリセットはItemの応答だけ変更する。
- `ABreakoutAddBallItem`を親に編集可能な`/Game/Breakout/AddBallItem` Blueprintを作成。SphereとTextRenderを持ち、白い球に黒のA、位置(-51,0,0)・Yaw180°・文字サイズ150で表示する。既存カメラでの視認性を確認して文字を調整した。落下速度は教材で任意とされているため600 units/秒を既定値にし、編集可能にする。
- 落下はDeltaSecondsを使い、Sweep付きで移動する。物理シミュレーションを使わず、高速移動でも途中のパドル／MissAreaを検知できるようにする。Overlapが重複しても取得は1回だけ。
- `ABreakoutGameManager.InGameBallNum`を主な球数管理に使い、実際の生成済みActorも集合で追跡する。最新以外のボールが消えても正しく数え、同じActorの破棄通知を二重に数えない。既存の`ActiveBall`は参照用に残すが、球数の判定には使用しない。
- `SpawnBall`は通常発射、`GenerateBall`は無料追加発射。生成に成功した時だけ球数を増やし、通常発射の残球を減らす。クリア・ゲームオーバー・遷移中は追加発射しない。
- クリア時の`InGameBallNum`は以後固定し、結果画面中に球が落ちても持ち越し分を失わない。レベル遷移で古い破棄通知が届いてもゲームオーバーにはしない。
- `ABreakoutBlock.ItemDropChance`を0.5、`ItemClass`をAddBallItemに設定。ダメージを受けただけでは落とさず、破壊時の1回だけ抽選する。
- 同じフレームの衝突ガードをボール単位に変更。同じ球からの重複通知は無視し、異なる2球が同じブロックに当たった場合は2回分のダメージを受け付ける。
- 教材のCubeに相当するパドルは本プロジェクトでは`PaddleMesh`へ改名済み。そのコリジョンをPawnにし、既存のPlayerタグ・狙い撃ちを維持する。両マップのMissAreaと壁に設定を適用する。

## 維持する内容

白いパドル・ボール、壁・カメラ・操作・速度、狙い撃ち、HPと色、BGMと衝突音、Level1の易しい5ブロック構成、Level2の10ブロック・4段配置を維持する。アイテム出現率を100%に固定したり、常設の確認用アイテムを置いたりしない。

## 検証計画

`Breakout.Lesson06.ItemMultiballAndCarryOver`を追加。Ball同士の同位置生成、通常発射の重複拒否、無料追加発射、残球0でのアイテム取得、実Sweepでの取得と取り逃し、非最新球の破棄、破壊時だけのドロップ、確率0/1、重複ドロップ防止、2球同時ダメージ、クリア後落球と複数球の持ち越し、Level2での最後の1球・ゲームオーバー・再開を確認する。Lesson03〜05のテストも回帰実行する。

## 検証結果（2026-10-23）

- UE 5.8.3 / Development Editorビルド成功。
- `Breakout.Lesson03.InputLivesAndRestart`: 成功。
- `Breakout.Lesson04.HpAudioAndLevelTravel`: 成功。
- `Breakout.Lesson05.AimingAndRealSweptHits`: 成功。
- `Breakout.Lesson05.Level2AimingAndRealSweptHits`: 成功。
- `Breakout.Lesson06.ItemMultiballAndCarryOver`: 成功。上記の実Overlap・複数球管理・確率分岐・二重通知・4球の持ち越しと再開を確認した。
- 通常カメラで白い球と黒いAを確認。文字が小さい初期案を150へ調整して再ビルド・全5件のテストを再実行した。
- 画面確認用に一時配置した2個のアイテムを落下・取得させ、`LeftBallNum=3`のまま`InGameBallNum=2`となり、2球が飛ぶことも確認。その後、確認用アクタを削除してマップを保存した。
- 最終ログは`Saved/Logs/Lesson06Check2.log`。参考資料の全ページ画像・画面確認結果は`Saved/Lesson06/`に保持する（いずれも生成ファイルのためGit対象外）。パッケージ化は実行していない。

## 実装結果GIF

`Recordings/01_06.gif`（約22秒・音声なし）。取得動作を確実に映すため録画時だけ2個の確認用アイテムを一時配置し、パドルを自動追従させた。残球3を消費せず2球へ増える様子と、その後のブロック破壊による通常のランダムドロップ・取得を記録した。出現率やボール速度などのゲーム設定は変更していない。録画後に確認用アクタを削除してマップを保存し、残存しないことを確認した。

コミット前にEditorビルドと上記5件のテストを再実行し、すべて成功した（`Saved/Logs/Lesson06Recording.log`）。
