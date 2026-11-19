#!/usr/bin/swift
import Foundation
import CoreMIDI
import AudioToolbox

// Melody: Teiichi Okano. Lyrics: Tatsuyuki Takano. All three verses.
// https://www.senzoku.ac.jp/music/career/pdf/score_furusato.pdf
struct UserError: Error, CustomStringConvertible { let description: String
    init(_ message: String) { description = message }
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


func play() throws {
    let folder=URL(fileURLWithPath:CommandLine.arguments[0]).standardizedFileURL.deletingLastPathComponent()
    var seq:MusicSequence?, player:MusicPlayer?
    try checked(NewMusicSequence(&seq)); guard let seq else {throw UserError("Sequence unavailable")}
    defer { DisposeMusicSequence(seq) }
    try checked(MusicSequenceFileLoad(seq,folder.appendingPathComponent("ふるさと.mid") as CFURL,.midiType,[]))
    var track:MusicTrack?, iterator:MusicEventIterator?
    try checked(MusicSequenceGetIndTrack(seq,0,&track))
    guard let track else {throw UserError("Vocal track missing")}
    try checked(NewMusicEventIterator(track,&iterator)); guard let iterator else {throw UserError("No notes")}
    defer {DisposeMusicEventIterator(iterator)}
    var phrases=0
    var count=0, lastBeat=0.0, has:DarwinBoolean=false
    try checked(MusicEventIteratorHasCurrentEvent(iterator,&has))
    while has.boolValue {
        var beat=0.0, type:MusicEventType=0, data:UnsafeRawPointer?, size:UInt32=0
        try checked(MusicEventIteratorGetEventInfo(iterator,&beat,&type,&data,&size))
        if type==kMusicEventType_MIDIRawData, let data {
            let length=data.load(as:UInt32.self)
            if length==97 {
                let bytes=Array(UnsafeRawBufferPointer(start:data.advanced(by:4),count:Int(length)))
                guard Array(bytes.prefix(6))==[240,125,86,75,1,45], bytes.last==247,
                      abs(beat-Double(phrases*48))<0.002 else {throw UserError("Unexpected lyric event")}
                phrases+=1
            }
        }
        if type==kMusicEventType_MIDINoteMessage, let data {
            let n=data.load(as:MIDINoteMessage.self)
            guard n.channel==0, (36...96).contains(n.note), beat>=lastBeat-0.002 else {throw UserError("Unexpected melody timing or channel")}
            count+=1; lastBeat=beat+Double(n.duration)
        }
        try checked(MusicEventIteratorNextEvent(iterator))
        try checked(MusicEventIteratorHasCurrentEvent(iterator,&has))
    }
    var duration=0.0
    try checked(MusicSequenceGetSecondsForBeats(seq,lastBeat,&duration))
    guard phrases==3, count==135, abs(lastBeat-143.25)<0.002, abs(duration-107.4375)<0.01 else {throw UserError("Score verification failed")}
    if CommandLine.arguments.contains("--check") {print("Verified 3 lyric loads, 135 notes, 80 BPM"); return}
    var client=MIDIClientRef()
    try checked(MIDIClientCreateWithBlock("Furusato score" as CFString,&client,nil))
    defer { MIDIClientDispose(client) }
    let target=try destination()
    try checked(MusicSequenceSetMIDIEndpoint(seq,target))
    try checked(NewMusicPlayer(&player)); guard let player else {throw UserError("Player unavailable")}
    defer { MusicPlayerStop(player); DisposeMusicPlayer(player) }
    try checked(MusicPlayerSetSequence(player,seq))
    try checked(MusicPlayerPreroll(player))
    print("ふるさと（全3番） → "+property(target,kMIDIPropertyDisplayName)); fflush(stdout)
    try checked(MusicPlayerStart(player))
    Thread.sleep(forTimeInterval:duration+0.4)
    var beat=0.0; try checked(MusicPlayerGetTime(player,&beat))
    guard beat>=lastBeat-0.01 else {throw UserError("最後の音符まで到達していません")}
    print("全3番、48小節を最後まで再生しました。")
}
do {try play()} catch {fputs(String(describing:error)+"\n",stderr); exit(1)}
