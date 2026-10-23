# 01_05 狙い撃ち

資料: https://docs.google.com/presentation/d/1SoLhJfiacDsA19ee3Fe5Uf3PkgtIgptGp5QGgDpsXFA/edit

## 確認した仕様と作業内容

全48ページの本文と画像を確認した。資料自体は変更していない。

- p.5〜14: パドル天面の衝突位置から法線を補正する。`(HitLocation.Y - Origin.Y) / BoxExtent.Y` に最大傾斜角20°を掛け、その角度で法線をRoll回転する。中央は真上、左右端では左右へ20°傾く。天面以外は元の法線とfalseを返す。
- p.15〜27: 法線の確認用関数を用意する。方向を正規化し長さ200、矢印サイズ100、青、表示時間0、太さ10で描く。半数10、インデックス-10〜10の21本をパドル天面に並べる。傾斜0°・20°・60°を確認し、通常プレイでは描画を切る。
- p.29〜38: 反射方向を`Atan2(Y,Z)`で真上0°の角度に変換し、`-90 + MinHorizontalAngleDeg`〜`90 - MinHorizontalAngleDeg`に制限する。`(X=0,Y=sin(angle),Z=cos(angle))`で単位ベクトルへ戻す。最小水平角度の既定値はp.34の設定画像にある10°。
- p.41〜46: パドルの衝突メッシュのComponentTagsに`Player`を追加。本プロジェクトでは教材のCube相当が`PaddleMesh`へ改名済みなので、そのコンポーネントへ付ける。相手コンポーネントのタグとパドルの識別、天面判定がすべて成立した時だけ補正法線と角度制限を使う。それ以外は従来の鏡面反射とする。どちらの場合も衝突音を残す。

## 教材の記述の読み替え

p.34本文にある角度範囲は、p.36〜38の説明・ノード図と一致しない。実装は後者の`Atan2(Y,Z)`、`[-90+m,90-m]`、`Y=sin,Z=cos`を採用した。m=10°なら上方向から±80°、水平から最低10°となる。

## 実装の配置

- `UBreakoutPaddleAimComponent`をBP_Paddleの`PaddleAim`コンポーネントとして追加。`MaxTiltNormalDeg`を編集できる。`GetTopNormal`、`DrawDebugArrow`、`DrawDebugTopSurfaceNormals`、`SetDebugNormalsEnabled`をBlueprintへ公開する。
- `UBreakoutAimingLibrary`に`ReflectDirection`と`ClampDirection`を実装。BP_Ballの既存HitイベントのDirection計算へ接続し、移動・発射・音・ミス処理は保持する。
- `UBreakoutBallRulesComponent.MinHorizontalAngleDeg`を10°で追加。パドル天面に当たった場合だけ使う。
- 01_02〜01_04同様、計算はBlueprintから呼べるC++、レベルとBlueprintは通常の編集可能アセットとして保存する。教材のマクロ・ノード一式をそのまま複製したものではない。
- 通常はデバッグ描画も追加コンポーネントのTickも無効。確認時だけ`SetDebugNormalsEnabled(true)`を呼ぶか、配置したPaddleAimで描画フラグを有効にしてPIEを開始する。
- 幅ゼロのパドルでは除算せず元の法線を返す。接触位置は左右端に制限し、異常な角度設定も範囲に収める。ゼロ方向のClampは真上へ戻す。

## 維持する内容

01_04で調整したLevel1の5個・HP1・Z=3000、Level2の10個・4段・HP1〜5、残球引き継ぎ、白いパドルとボール、壁とカメラ、速度・入力・BGM・衝突SEを維持する。資料の例にある大量のブロック配置へは戻さない。

## 検証

`Breakout.Lesson05.AimingAndRealSweptHits`および`Breakout.Lesson05.Level2AimingAndRealSweptHits`を追加。両マップでコンポーネントとPlayerタグ、法線の左右・中央・側面・下面、傾斜0/20/60°、10°Clamp・左右保持・単位長・ゼロ方向、デバッグ切り替えを確認する。最後にBP_Ballの実際のTick／Sweep移動でパドルへ左・中央・右から衝突させ、上方向へ狙い分けられることを検証する。計算関数を単独で呼ぶだけのテストにはしない。

既存のLesson03・Lesson04の自動テストも回帰実行する。

## 検証結果（2026-10-23）

- UE 5.8.3 / Development Editorビルド成功。
- `Breakout.Lesson03.InputLivesAndRestart`: 成功。
- `Breakout.Lesson04.HpAudioAndLevelTravel`: 成功。
- `Breakout.Lesson05.AimingAndRealSweptHits`: 成功。
- `Breakout.Lesson05.Level2AimingAndRealSweptHits`: 成功。
- 実Sweepの反射方向は両マップとも、左寄り`(0,-0.588,0.809)`、中央`(0,0,1)`、右寄り`(0,0.588,0.809)`となった。法線・最小角・タグの計算だけでなく、保存したBP_BallのHitイベントから実際に新しい処理へ接続されていることを確認した。
- 青い法線矢印をPIEで描画し、傾斜0°・20°・60°の見た目を確認。確認後はパドルのテンプレートを20°・デバッグOFFへ戻して保存した。
- 初回に判明したコンポーネント名の違いは`PaddleMesh`へタグを付けることで修正した。既存のUI色テストは、非表示のエディタビューポートではアニメーションが評価されないため、別のPIEウィンドウで実際に描画して検証する形に変更した。ゲームのアニメーション自体は変更していない。
- 最終テストログ: `Saved/Logs/Lesson05Check2.log`。参考スライドの本文・全ページ画像、確認画像は`Saved/Lesson05/`（いずれも生成ファイルのためGit対象外）。

## 実装結果GIF（2026-10-23）

- `Docs/Recordings/01_05.gif`: 約30秒、960×540。Level2でパドルの当たり位置をずらし、反射方向の変化とブロックのHP減少・色の変化を実機PIEで録画した。GIFのため音声は含まれない。
- 録画中のみMCPでパドルを自動追従させ、中心からのずれも変化させて狙い撃ちを見せている。発射Action用の一時Blueprintを使用し、録画後に削除した。反射・衝突・HP・破壊は通常のゲーム処理であり、方向やHPの強制書き換えはしていない。自動追従は実装本体に保存していない。
- コミット前にもDevelopment Editorビルド成功を確認した。
