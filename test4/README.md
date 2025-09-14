# Test4 - UART Port C Monitor

M5Stack Core2のPort CでUART通信を監視するテストプログラムです。

## 機能

- Port C（GPIO13/GPIO14）でUART通信を受信
- 受信データをシリアルモニターとディスプレイに表示
- JSON形式のデータもテキストとしてそのまま表示（パースなし）

## ハードウェア設定

### Port C接続
- **RX**: GPIO13 (受信)
- **TX**: GPIO14 (送信)
- **ボーレート**: 115200

## ボタン操作

- **Button A**: テストデータ送信（JSON形式）
- **Button B**: ディスプレイクリア
- **Button C**: UART統計情報をシリアルに表示

## 使用方法

1. Port Cに外部デバイスを接続
2. プログラムを書き込み
3. シリアルモニターとディスプレイで受信データを確認

## ディスプレイ表示

- ヘッダー情報（ピン設定、ボーレート）
- 受信データのリアルタイム表示
- 長いデータは自動的に切り詰め表示

## シリアル出力例

```
=== M5Stack Core2 UART Test (Port C) ===
Starting UART communication test...
UART initialized: RX=GPIO13, TX=GPIO14, Baud=115200
Setup complete. Waiting for UART data...
UART RX: {"sensor":"temperature","value":25.6}
UART RX: {"sensor":"humidity","value":60.2}
```

## 注意事項

- 受信データは改行文字（\n, \r）で区切られます
- 200文字を超える長いデータは自動的に分割されます
- ディスプレイには最新のデータから順に表示されます
