# このリポジトリは？
このリポジトリでは、IoTデバイスのデータ保存先に無料で使用できるgoogle sheetsを使うことを検討していく過程を記載します。

# 解決したい問題点
一般的なIoTデータ収録では特定のサービスプロバイダと契約して、データストレージやグラフ化などのサービスを組み合わせてデータ収録系を構築します。<br>
しかし、IoTのフィジビリティ確認やテストの段階では機能がオーバースペックなところも多々あり、何より費用がかかります。<br>
そこで、google sheets と Google Apps Scriptを組み合わせてデータ収集のwebアプリ的なものを作ることにより、簡易的なIoTデータ収録ができないだろうかと考えました。<br>
これにより、インターネットにアクセスできるデバイスであれば比較的簡単にデータをgoogle sheetsに向けてデータを送信することができ、google sheets内部でグラフ化やアラーム発呼などの処理をスクリプトとして記述することで有償サービスの代替えができる可能性があると考えます。

# なにが書かれているか
このリポジトリでは、以下のことをまとめています。
- 基本的なデータ保存のためのGASのコード
- データを送るエッジデバイス（Arduinoを利用）のコード
- その他必要になりそうな機能のコード

このリポジトリのルート部分にはGASのコードが保管されています。

# システムの全体像
<br>
<img src="https://github.com/user-attachments/assets/61072de2-511e-486b-8f23-e3b3107718ad" width="500">
<br><br>

この図にしめしたように、google sheet 内にGASを通じてHTTPプロトコル(POSTメソッド)でデータを送ります。<br>
送られたデータはスプレッドシート上に受信時間のタイムスタンプと共に記録され、シートを開くことで人間が直接確認することができます。<br>
送られるデータはJSONフォーマットで、JSONのキー名に対応するデータをシート上のどの列に書き込むかはGASスクリプトで決めています。

__例：エッジデバイスからのデータが保存されたスプレッドシート__
<br>
<img src="https://github.com/user-attachments/assets/75e31621-432e-40fd-957b-bf36b398cefd" width="500">
<br>（グラフは手作業で追加しています）<br>

将来の拡張として、HTTPでの読み出しリクエストに応答して、表内（保存データ）の一部のデータや、統計データなどを返すようなスクリプトも記述が可能かと思います。

# 使い方
## google sheet:
- google sheet で新しいファイルを作成し、ツール - AppScript　を選択。<br>
- エディタ画面になるので、このリポジトリの.gs拡張子のファイルの内容をコピーする。<br>
- このとき、関数名だけを同じにしておけば、ファイル名は特に同じ出なくても大丈夫だと思う。
- 画面右上の「デプロイ」をおして「新しいデプロイ」を選択。<br>
- ポップアップウインドが出てくるので、左側のベイン「種類の選択」の歯車マークを押して、「ウエブアプリ」を選択<BR>
- 左側の「設定」に必要な事項を記入して、「アクセスできるユーザ」を全員に設定。「デプロイ」を押す<br>
- URLが表示されるので、コピーなどしておく。後ほどデバイスの作成の時に必要。

## デバイス(arduino):
- リポジトリのArduinoディレクトリ以下を手元の機械のArduinoディレクトリ以下にコピーする。
- SAMPLE_connection_IDs.hのファイル名を"connection_ids.h"に変更し、内容を自分の環境に合うように設定。このとき、API_URLに先ほどデプロイしたApp ScriptのURLを記載する。
- このファイルで設定したSENSOR_IDがシートの名前になる。（一つのファイルに複数のシートが存在できる）
- デバイスを用意してArduinoIDEなどからコンパイル後デバイスへダウンロードし、実行する。
- シリアルポートで動作の様子がわかるので、デバグ時にはシリアルターミナルを開いておくといい。
- うまくいけば、先ほどのGoogle Sheet上に新しいタブができて、データが来ているはず。

## 注意
- 複数のデバイスで同じSENSOR_IDを使うことも可能だが、デバイス間の送信タイミングが同期できないのでシート上のタイムスタンプは別々になってしまい、後のデータ処理が面倒になる可能性が考えられる。
- タイムスタンプは受信時に押される。
- デバイスからのデータ送信周期はかなりばらつきがあり、正確ではない。
- デバイスでは、API側からデータ受信がされたかどうかの確認をしていないので、シート上にあるデータの完全性は保証されない。欠落したデータがある可能性がある。
- google sheetsの1ファイルでのデータ数上限は1000万セルのようなので、それを超えるとデータ追加ができなくなる。制限を超えないように古いデータはCVSで保存した後シート上から削除するようなスクリプトの定期実行が必要かもしれない。（とはいえ、1000万セルで10分ごとのデータだと数十年は使える計算）
  
