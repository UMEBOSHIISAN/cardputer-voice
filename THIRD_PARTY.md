# クレジットとライセンス

ZIP内のライセンス原文は `source/licenses/`、Gitチェックアウトでは `licenses/` にあります。本プロジェクト固有のコード・素材に対するOSSライセンスは未指定です。各依存ライブラリ・フォント・原典にはそれぞれの利用条件が適用されます。

| ファームウェアの構成要素 | 使用版・出典 | 同梱の表示 |
|---|---|---|
| M5Cardputer | [1.1.1、f139285](https://github.com/m5stack/M5Cardputer/tree/f1392858b9994c3547120e602a57d3553d16ab01) | M5Cardputer.txt、Adafruit-TCA8418.txt |
| M5Unified / M5GFX | [0.2.19](https://github.com/m5stack/M5Unified/tree/0.2.19) / [0.2.26](https://github.com/m5stack/M5GFX/tree/0.2.26) | M5Unified.txt、M5GFX.txt、Adafruit-GFX.txt |
| 日本語表示フォント | M5GFX収録のIPAゴシック変換版。原版は[IPAフォント](https://moji.or.jp/ipafont/) | IPA.txt、IPA-conversion.md |
| Arduino-IRremote | [4.7.1](https://github.com/Arduino-IRremote/Arduino-IRremote/tree/4.7.1) | IRremote.txt |
| Arduino-ESP32 | [2.0.16](https://github.com/espressif/arduino-esp32/tree/2.0.16) | Arduino-ESP32.txt（LGPL 2.1以降）。配布ZIPの third-party/arduino-esp32/ に使用したコア・SPI・Wire・ボード定義のソースを収録 |
| ESP-IDF / TinyUSB | [ESP-IDF 4.4.7](https://github.com/espressif/esp-idf/tree/v4.4.7)、[TinyUSB](https://github.com/hathach/tinyusb) | ESP-IDF.txt、TinyUSB.txt |

Xtensaツールチェーンに含まれるC/C++ランタイムの表示は Newlib.txt、GCC-runtime.txt、GPL-3.0.txt に収録しています。

Arduinoライブラリを変更して再リンクするときは、PlatformIOが取得した framework-arduinoespressif32 2.0.16 の対応するソースを差し替え、プロジェクトで `pio run -t clean`、`make firmware` を実行してください。

## デモ曲

- **かえるのうた**　「かえるのうた」の6モーラに合わせた、半音・強弱・ビブラートの練習フレーズ。
- **さくらさくら**　日本の伝承曲。Shin-Itchiro Yokoyama / Spyridon Papanastasiou の譜面を参照し、単旋律MIDIと母音の割り当てに翻案。[原譜](https://imslp.org/wiki/User:Tsester/Sakura_Sakura_(Yokoyama,_Shin-Itchiro))・[CC BY 3.0](https://creativecommons.org/licenses/by/3.0/)
- **君が代**　内閣府掲載の楽譜から単旋律を採譜。テンポはこのデモ用に四分音符60。[内閣府「国旗・国歌」](https://www8.cao.go.jp/chosei/kokkikokka/kokkikokka)
- **ふるさと（故郷）**　作詞 高野辰之、作曲 岡野貞一。洗足学園音楽大学の教材譜を参照し、歌唱旋律をト長調・四分音符80で入力。全3番を収録。[参照譜](https://www.senzoku.ac.jp/music/career/pdf/score_furusato.pdf)
