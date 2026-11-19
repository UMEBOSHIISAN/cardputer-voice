# クレジットとライセンス

ZIP内のライセンス原文は `source/licenses/`、Gitチェックアウトでは `licenses/` にあります。本プロジェクト固有のコード・素材に対するOSSライセンスは未指定です。各依存ライブラリ・フォント・原典にはそれぞれの利用条件が適用されます。

## 原作者と着想の経緯

**Jameson Hodge さん**が制作した **CHIMERA（Chimera Vocal Synth）** は、ブラウザで舌・唇・顎を動かして演奏できる声のシンセサイザーです。「声道シミュレータ日本語版」は同作に着想を得たと明記しています。Cardputer Voiceは、その日本語版から五母音の値と口の姿勢を受け継ぎました。[Jameson Hodge さん](https://github.com/jamesondh) · [CHIMERAの原作](https://github.com/jamesondh/chimera-vocal-synth) · [CHIMERAの作者による参考文献・謝辞](https://github.com/jamesondh/chimera-vocal-synth/blob/main/docs/architecture.md#references-and-credits) · [日本語版のクレジット](https://github.com/UMEBOSHIISAN/vocal-tract-synth-ja/blob/9ea1f16a8c9431c6e3500bcadf5377211c613b65/README.md#クレジット)

## 声のモデルと画面の出典

- **声道シミュレータ日本語版** の `index.html` にある `ANAT.vowelRef` を五母音のフォルマントに、`VPOSE` を顎・唇・舌の描画に使用しました。[参照した版 9ea1f16](https://github.com/UMEBOSHIISAN/vocal-tract-synth-ja/blob/9ea1f16a8c9431c6e3500bcadf5377211c613b65/index.html)
- 日本語版の五母音は合成用の代表値です。参照版の出典表では、対応する個別の計測論文は未特定とされています。[五母音の値](https://github.com/UMEBOSHIISAN/vocal-tract-synth-ja/blob/9ea1f16a8c9431c6e3500bcadf5377211c613b65/index.html#L1010) · [口の姿勢](https://github.com/UMEBOSHIISAN/vocal-tract-synth-ja/blob/9ea1f16a8c9431c6e3500bcadf5377211c613b65/index.html#L1160) · [参照版の出典表](https://github.com/UMEBOSHIISAN/vocal-tract-synth-ja/blob/9ea1f16a8c9431c6e3500bcadf5377211c613b65/README.md#L246)

## READMEの図と接続

| 図 | 描画に使った資料 |
|---|---|
| 母音を弾く（GIF） | 本体の鍵盤割り当てと口の座標を使って作図。波形はド（C4）を押したまま各母音に切り替え、v1.0.0の合成コードをMacで実行して得たPCMです。[鍵盤割り当て](src/voice.hpp#L30) · [合成処理](src/voice.hpp#L95) · [口の描画](src/screen.hpp#L68) |
| 『ふるさと』冒頭4小節 | 同梱MIDIの歌詞イベント、音高、ノートの開始・終了時刻から作図。参照譜は洗足学園音楽大学「教職ピアノ実習」の1ページ目です。[MIDI](mac/ふるさと.mid) · [教材譜](https://www.senzoku.ac.jp/music/career/pdf/score_furusato.pdf#page=1) |
| USB-MIDIと音声出力の接続図 | Macの歌詞送信、本体のUSB-MIDI受信、およびCardputer-Advの内蔵スピーカー・3.5 mm端子を図示。[Mac側](mac/VoiceMIDI.swift) · [本体側](src/midi_usb.hpp) · [M5Stack公式の機器仕様](https://docs.m5stack.com/en/core/Cardputer-Adv) |

Mac用ツールはAppleのCore MIDIで機器を選択し、Audio Toolboxのシーケンサーで曲を再生します。[Core MIDI](https://developer.apple.com/documentation/coremidi) · [Audio Toolbox](https://developer.apple.com/documentation/audiotoolbox)

## ライブラリ・フォント

| ファームウェアの構成要素 | 使用版・出典 | 同梱の表示 |
|---|---|---|
| M5Cardputer | [1.1.1、f139285](https://github.com/m5stack/M5Cardputer/tree/f1392858b9994c3547120e602a57d3553d16ab01) | M5Cardputer.txt、Adafruit-TCA8418.txt |
| M5Unified / M5GFX | [0.2.19](https://github.com/m5stack/M5Unified/tree/0.2.19) / [0.2.26](https://github.com/m5stack/M5GFX/tree/0.2.26) | M5Unified.txt、M5GFX.txt、Adafruit-GFX.txt |
| 日本語表示フォント | M5GFX収録のIPAゴシック変換版。原版は[IPAフォント](https://moji.or.jp/ipafont/) | IPA.txt、IPA-conversion.md |
| Arduino-IRremote | [4.7.1](https://github.com/Arduino-IRremote/Arduino-IRremote/tree/v4.7.1) | IRremote.txt |
| Arduino-ESP32 | [2.0.16](https://github.com/espressif/arduino-esp32/tree/2.0.16) | Arduino-ESP32.txt（LGPL 2.1以降）。配布ZIPの third-party/arduino-esp32/ に使用したコア・SPI・Wire・ボード定義のソースを収録 |
| ESP-IDF / TinyUSB | [ESP-IDF 4.4.7](https://github.com/espressif/esp-idf/tree/v4.4.7)、[TinyUSB](https://github.com/hathach/tinyusb) | ESP-IDF.txt、TinyUSB.txt |

Xtensaツールチェーンに含まれるC/C++ランタイムの表示は Newlib.txt、GCC-runtime.txt、GPL-3.0.txt に収録しています。

Arduinoライブラリを変更して再リンクするときは、PlatformIOが取得した framework-arduinoespressif32 2.0.16 の対応するソースを差し替え、プロジェクトで `pio run -t clean`、`make firmware` を実行してください。

## デモ曲

- **かえるのうた**　「かえるのうた」の6モーラに合わせた、半音・強弱・ビブラートの練習フレーズ。
- **さくらさくら**　日本の伝承曲。Shin-Itchiro Yokoyama / Spyridon Papanastasiou の譜面を参照し、単旋律MIDIと母音の割り当てに翻案。[原譜](https://imslp.org/wiki/User:Tsester/Sakura_Sakura_(Yokoyama,_Shin-Itchiro))・[CC BY 3.0](https://creativecommons.org/licenses/by/3.0/)
- **君が代**　内閣府掲載の楽譜から単旋律を採譜。テンポはこのデモ用に四分音符60。[内閣府「国旗・国歌」](https://www8.cao.go.jp/chosei/kokkikokka/kokkikokka)
- **ふるさと（故郷）**　作詞 高野辰之、作曲 岡野貞一。洗足学園音楽大学の教材譜を参照し、歌唱旋律をト長調・四分音符80で入力。全3番を収録。[参照譜](https://www.senzoku.ac.jp/music/career/pdf/score_furusato.pdf)
