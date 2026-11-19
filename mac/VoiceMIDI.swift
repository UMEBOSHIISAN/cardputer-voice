import Foundation
import CoreMIDI

struct UserError: Error, CustomStringConvertible { let description: String
    init(_ message: String) { description = message }
}

struct Mora { let initial: UInt8; let shape: UInt8 }

func parseLyrics(_ text: String) throws -> [Mora] {
    let rows = ["あいうえお","かきくけこ","さしすせそ","たちつてと","なにぬねの",
                "はひふへほ","まみむめも","や　ゆ　よ","わゐ　ゑを","らりるれろ",
                "がぎぐげご","ざじずぜぞ","だぢづでど","ばびぶべぼ","ぱぴぷぺぽ"]
    var dictionary: [Character:Mora] = [:]
    for (initial,row) in rows.enumerated() {
        for (vowel,kana) in row.enumerated() where kana != "　" {
            dictionary[kana] = Mora(initial: UInt8(initial),shape: UInt8(vowel))
        }
    }
    for (k,c,v): (Character,UInt8,UInt8) in [("し",15,1),("ち",16,1),("つ",17,2),("じ",18,1),
        ("ぢ",18,1),("づ",11,2),("ふ",19,2),("ん",20,2),("っ",21,2),("を",0,4)] {
        dictionary[k] = Mora(initial:c,shape:v)
    }
    var out: [Mora] = []
    var previousVowel: UInt8?
    for scalar in text.precomposedStringWithCanonicalMapping.unicodeScalars {
        let code=scalar.value
        let kana=Character(String((0x30a1...0x30f6).contains(code) ? UnicodeScalar(code-0x60)! : scalar))
        if kana.isWhitespace || "、。・,.".contains(kana) { continue }
        if kana == "ー", let v=previousVowel { out.append(Mora(initial:0,shape:v)); continue }
        if let small=["ゃ":0,"ゅ":2,"ょ":4][String(kana)] {
            guard let prev=out.last, prev.shape==1, prev.initial != 0 else { throw UserError("拗音の位置を確認してください（例：きゃ）") }
            let glide:UInt8=[15,16,18].contains(prev.initial) ? 0 : 5
            out[out.count-1]=Mora(initial:prev.initial,shape:UInt8(small)+glide)
            previousVowel=UInt8(small)
        } else if let m=dictionary[kana] {
            out.append(m); if m.initial<20 { previousVowel=m.shape }
        } else { throw UserError("この表記は歌えません「"+String(kana)+"」。ひらがな・カタカナに直してください。") }
    }
    guard out.count<=96 else { throw UserError("歌詞は一度に96モーラまでです") }
    return out
}

func property(_ object: MIDIObjectRef,_ key: CFString) -> String {
    var value: Unmanaged<CFString>?
    guard MIDIObjectGetStringProperty(object,key,&value)==noErr, let value else { return "" }
    return value.takeRetainedValue() as String
}

func destination() throws -> MIDIEndpointRef {
    let matches=(0..<MIDIGetNumberOfDestinations()).compactMap { idx -> MIDIEndpointRef? in
        let ep=MIDIGetDestination(idx)
        var entity=MIDIEntityRef(), device=MIDIDeviceRef(), offline:Int32=0
        guard MIDIEndpointGetEntity(ep,&entity)==noErr, entity != 0,
              MIDIEntityGetDevice(entity,&device)==noErr, device != 0 else { return nil }
        _=MIDIObjectGetIntegerProperty(device,kMIDIPropertyOffline,&offline)
        let names=[property(ep,kMIDIPropertyDisplayName),property(device,kMIDIPropertyName),property(device,kMIDIPropertyModel)]
        return offline==0 && names.contains(where:{$0=="Cardputer Voice" || $0.hasPrefix("Cardputer Voice ")}) ? ep : nil
    }
    guard matches.count==1 else { throw UserError("接続中の Cardputer Voice が "+String(matches.count)+" 台です。USB接続を確認してください。") }
    return matches[0]
}

func checked(_ status: OSStatus) throws { if status != noErr { throw UserError("CoreMIDI error "+String(status)) } }

func run() throws {
    let args=Array(CommandLine.arguments.dropFirst())
    if args == ["--test"] {
        let a=try parseLyrics("カエルのうた"); let b=try parseLyrics("きゃっしゅーん")
        guard a.map({$0.initial})==[1,0,9,4,0,3], b.map({$0.shape})==[5,2,2,2,2],
              (try? parseLyrics("歌")) == nil, (try? parseLyrics(String(repeating:"あ",count:97))) == nil
        else {throw UserError("kana test failed")}
        print("PASS: kana, mora grouping, katakana, bounds and invalid input"); return
    }
    let demo=args==["--demo"], probe=args==["--device"], clear=args==["--clear"]
    guard !args.isEmpty else {throw UserError("歌詞を指定してください。--demo / --device / --clear も使えます。")}
    let morae=try parseLyrics(probe || clear ? "" : (demo ? "かえるのうた" : args.joined(separator:" ")))
    guard !morae.isEmpty || clear || probe else { throw UserError("歌詞が空です") }
    var client=MIDIClientRef(), port=MIDIPortRef()
    try checked(MIDIClientCreateWithBlock("Cardputer lyrics" as CFString,&client,nil))
    defer { MIDIClientDispose(client) }
    let target=try destination()
    print("MIDI OUT → "+property(target,kMIDIPropertyDisplayName))
    if probe {return}
    try checked(MIDIOutputPortCreate(client,"Lyrics" as CFString,&port))
    defer { MIDIPortDispose(port) }
    func send(_ bytes: [UInt8]) throws {
        // All messages, including a maximum-sized 96-mora phrase, fit in one packet.
        var list=MIDIPacketList()
        let status=withUnsafeMutablePointer(to:&list){ ptr -> OSStatus in
            let packet=MIDIPacketListInit(ptr)
            return bytes.withUnsafeBufferPointer { data in
                guard data.count<=256 else { return -1 }
                _=MIDIPacketListAdd(ptr,MemoryLayout<MIDIPacketList>.size,packet,0,data.count,data.baseAddress!)
                return MIDISend(port,target,ptr)
            }
        }
        try checked(status)
    }
    let phrase:[UInt8]=[0xf0,0x7d,0x56,0x4b,1,UInt8(morae.count)]+morae.flatMap{[$0.initial,$0.shape]}+[0xf7]
    try send(phrase)
    Thread.sleep(forTimeInterval:0.08)
    if clear {print("歌詞を解除しました"); return}
    print("歌詞を送信しました（"+String(morae.count)+"モーラ）。MIDI ch 1 で演奏できます。")
    if demo {
        for (i,n): (Int,UInt8) in [60,62,63,65,67,65].enumerated() {
            try send([0x90,n,UInt8(76+i*6)])
            Thread.sleep(forTimeInterval:0.24)
            if i==5 {try send([0xb0,1,92]); try send([0xe0,0,72]); Thread.sleep(forTimeInterval:0.45)}
            try send([0x80,n,0]); Thread.sleep(forTimeInterval:0.11)
        }
        // A held note with all five vowel CC values.
        try send([0xe0,0,64]); try send([0xb0,1,0]); try send([0x90,64,100])
        for value: UInt8 in [0,32,64,96,127] {try send([0xb0,20,value]); Thread.sleep(forTimeInterval:0.28)}
        try send([0x80,64,0]); try send([0xb0,21,127]); Thread.sleep(forTimeInterval:0.1)
        print("デモ送信完了")
    }
}
do {try run()} catch {fputs("\(error)\n",stderr); exit(1)}
