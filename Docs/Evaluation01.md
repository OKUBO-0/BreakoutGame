# 評価課題01 提出記録

資料: https://docs.google.com/presentation/d/1OAXTxzlLTB92kFkjben1mudNk9ot0lFGgDPGytCllb8/edit

## 提出条件

全8ページを確認。p.4〜5はゲームの評価項目、p.7は提出方法。実行に必要なファイルと`readme.md`をZIPに含め、名前を`ND1_クラス記号_出席番号_シ_メイ_Test01.zip`にする。アレンジ箇所はREADMEに記載する。

本人指定のクラス・出席番号・カタカナ氏名により、提出名は`ND1_LE4B_06_オオクボ_タク_Test01.zip`。

## 不要ファイルの確認

- Git管理ファイルにexe・dll・pdb・obj・zip・検証ログは含まれていない。Saved以下の配布物・キャッシュ・ログは.gitignoreの対象。
- 提出ZIPにはWindows Shippingパッケージの実行用ファイル26個と、ルートREADMEを`readme.md`として同梱する。
- UATの`Manifest_UFSFiles_Win64.txt`と`Manifest_NonUFSFiles_Win64.txt`は検証用なので提出対象から除外する。元のパッケージには保存し、再検証に使用する。
- DLL、Pak・IoStore、ランタイム前提条件、NOTICESは維持する。依存性を確認せず実行用DLLを削ることはしない。
- ソース、プロジェクト、テスト、履歴用GIF、音源原本、資料画像、キャッシュは提出ZIPに含めない。開発用リポジトリには維持する。
- 元データの削除は行っていない。提出対象を選別したコピーを作成する。

## 内容と検証範囲

READMEに操作方法、ルール、2ステージの構成、白いパドル・ボールと壁の調整、オリジナル音源を記載した。評価項目の実装根拠はLesson03〜08の記録および対応するC++・Blueprintアセットを参照する。採点結果は推定しない。

Lesson08の最終Windows Shipping版を使用。配布版でA/D移動（本人確認）、発射、ブロック破壊、Level1クリア、SpaceによるLevel2遷移と残球の引き継ぎを確認済み。エディタ回帰テスト6件は全成功。配布版でのゲームオーバー再開と、2周の通しプレイは未確認。

最終Level2の次レベルは未指定であり、クリア後Spaceを押しても結果画面に留まる。READMEに明記した。課題p.4のクリア時レベル遷移はLevel1からLevel2で実装しているが、最終面にも遷移を求める運用であれば追加対応が必要。

`Tools/Prepare-Submission.ps1`で提出フォルダとZIPを作成し、ZIP内の全27ファイルを読み戻してステージングファイルとのSHA-256一致を確認した。提出物は`Saved/Submissions`以下でGit対象外。再生成時は既存の同名出力を上書きしない。

再生成コマンド:

```powershell
& .\Tools\Prepare-Submission.ps1 -ClassCode LE4B -AttendanceNumber 06 -FamilyName オオクボ -GivenName タク
```
