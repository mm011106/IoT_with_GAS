
//  指定されたシート名のシートがない場合に新しくシートを作る関数
//    引数：シート名 
// 
function addSheet(newSheetName) {
  // アクティブなスプレッドシートを取得
  var spreadsheet = SpreadsheetApp.getActiveSpreadsheet();

  // 追加するシートの名前を指定
  // var newSheetName = "New Sheet Name";

  // 同じ名前のシートが存在しないか確認
  if (!spreadsheet.getSheetByName(newSheetName)) {
    // 新しいシートを作成
    spreadsheet.insertSheet(newSheetName);
    Logger.log("Sheet added successfully: " + newSheetName);
  } else {
    Logger.log("Sheet with the name '" + newSheetName + "' already exists.");
  }
}