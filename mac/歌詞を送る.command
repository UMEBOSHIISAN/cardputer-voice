#!/bin/zsh
cd -- "${0:A:h}" || exit 1
read 'lyric?歌詞をかなで入力してください（例：かえるのうた） > ' || exit 1
./voice-midi "$lyric"
