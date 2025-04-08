// web app
//  2025/4/8 Miyamoto

// (1) Postリクエストを受け取ると doPost が実行される
// Sensor-Device用の処理  Postで送られてきたJSONデータをスプレッドシートに挿入する
// 		リクエストに添付されているJSONデータをパースして処理
//    JSONでのデータ構造：{"sheet_name":"sensor2","temperature":99.99, "humidity":99.99, "pressure": 1000.0}'
// 			sheet_name:データを追加するシート名
//        該当するシート名がなければ、指定された名前であらたにシートを作成しデータを挿入。
// 			temperature, humidity, pressure：データ本体

function doPost(e) {
  // 送信されてくるJSONデータから、要素を取り出す
  //  JSONデータがついていないPOSTではテストデータを使う
  try{
    var params = JSON.parse(e.postData.getDataAsString());
    Logger.log("Successfully Received JSON data.");
  }
  catch (error){
    Logger.log("No data found. Use test data.");
    // テスト用データ
    var params = JSON.parse('{"sheet_name":"sensor2","temperature":99.99, "humidity":10.00, "pressure": 1001.1}');
    // var params = JSON.parse('{"temperature":99.99, "humidity":10.00, "pressure": 1001.1}');
    // var params = JSON.parse('{"sheet_name":"sensor2","temperature":22.10}');
  }
  
  //  データの取り出し 
  // JSONのインデックス名と同じメンバー名で各データにアクセスします
  var sheet_name = params.sheet_name;   //"sheet_name"のデータを取り出す
  // シート名の指定がなければデフォルトのシート名にデータを追記する
  if (!sheet_name) {
    var sheet_name = 'sensor1';  // デフォルトのシート名
  }
  var temperature = params.temperature;
  var humidity = params.humidity;
  var pressure = params.pressure;

  Logger.log(sheet_name);
  Logger.log(temperature);

  // データを追記するシートを読み出します。
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(sheet_name);

  //	指定したシートがない場合の処理;
  if (!sheet) {
    //  指定された名前でシートを作成し、ヘッダ行を追記（ヘッダ行は適宜変更してください）
    addSheet(sheet_name); // refer addSheet.gs file
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(sheet_name);
    sheet.getRange(1, 1).setValue('Date-Time');
    sheet.getRange(1, 2).setValue('temperature');
    sheet.getRange(1, 3).setValue('humidity');
    sheet.getRange(1, 4).setValue('pressure');
  }

  // データをシートに追加
  sheet.insertRows(2,1);  // 2行1列目に列を挿入する
  sheet.getRange(2, 1).setValue(new Date());      // 受信日時を記録
  sheet.getRange(2, 2).setValue(temperature);     // 温度を記録
  sheet.getRange(2, 3).setValue(humidity);        // 湿度を記録
  sheet.getRange(2, 4).setValue(pressure);        // 気圧を記録

  return;
}


// (２) GETリクエストを受け取ると doGet が実行される
// 
//   viewer-device用に最新のデータをスプレッドシート（"A2:B2"）から読み込んで、JSONにして返す
//    A2: タイムスタンプ、B2: 温度データの最新値
// 
function doGet() {
   // スプレッドシートを指定 (シート名は適宜変更してください)
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName("sensor1"); // シート名を指定

  // A2:B2 のデータを取得
  var range = sheet.getRange("A2:B2");  // 最新のデータを選択
  var values = range.getValues();

  // データの存在を確認
  if (values.length !== 1 || values[0].length !== 2) {
    return ContentService.createTextOutput(JSON.stringify({ error: "データの形式が不正です (A2:B2 を期待)" }))
        .setMimeType(ContentService.MimeType.JSON);
  }

  // データをtimestampとtemperatureとして抽出
  //   タイムスタンプの読み出しと、タイムゾーンの補正
  //   シートから読み出した時刻フォーマットされたデータはUTCで正規化されているので、JSTに戻して文字列に変換
  var timestamp = values[0][0];
    // タイムゾーンを JST に設定
  var jstTimeZone = SpreadsheetApp.getActiveSpreadsheet().getSpreadsheetTimeZone();
  // JST で日付をフォーマット (YYYY/MM/DD HH:MM)
  var formattedDate = Utilities.formatDate(timestamp, jstTimeZone, 'yyyy/MM/dd HH:mm');

  var temperature = values[0][1];

  // 結果をJSON形式で返す
  var output = JSON.stringify({
    // timestamp: timestamp,
    timestamp: formattedDate,
    temperature: temperature
  });

  return ContentService.createTextOutput(output).setMimeType(ContentService.MimeType.JSON);
}
