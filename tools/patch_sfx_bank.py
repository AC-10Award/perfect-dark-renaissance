#!/usr/bin/env python3

import argparse
import pathlib
import struct
import sys


def read_u16_be(data, offset):
    return struct.unpack_from(">H", data, offset)[0]


def read_u32_be(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


def write_u32_be(data, offset, value):
    struct.pack_into(">I", data, offset, value)


def align(value, amount):
    return (value + (amount - 1)) & ~(amount - 1)


def read_wav_pcm16_mono(path):
    data = path.read_bytes()

    if data[0:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"{path} is not a RIFF/WAVE file")

    offset = 12
    fmt = None
    pcm = None

    while offset + 8 <= len(data):
        chunk_id = data[offset:offset + 4]
        chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
        chunk_data = data[offset + 8:offset + 8 + chunk_size]

        if chunk_id == b"fmt ":
            if chunk_size < 16:
                raise ValueError("fmt chunk is too small")

            fmt_tag, channels, rate, _, blockalign, bits = struct.unpack_from("<HHIIHH", chunk_data, 0)

            # Support standard PCM and WAVE_FORMAT_EXTENSIBLE PCM.
            if fmt_tag == 0xFFFE:
                if chunk_size < 40:
                    raise ValueError("extensible fmt chunk is too small")
                subformat = chunk_data[24:40]
                pcm_guid = bytes.fromhex("01000000-0000-1000-8000-00aa00389b71".replace("-", ""))
                if subformat != pcm_guid:
                    raise ValueError("unsupported extensible WAV subformat")
                fmt_tag = 0x0001

            fmt = {
                "format": fmt_tag,
                "channels": channels,
                "rate": rate,
                "blockalign": blockalign,
                "bits": bits,
            }
        elif chunk_id == b"data":
            pcm = chunk_data
            break

        offset += 8 + chunk_size + (chunk_size & 1)

    if fmt is None or pcm is None:
        raise ValueError("missing fmt or data chunk")

    if fmt["format"] != 0x0001:
        raise ValueError("only PCM WAV is supported")
    if fmt["channels"] != 1:
        raise ValueError("only mono WAV is supported")
    if fmt["bits"] != 16 or fmt["blockalign"] != 2:
        raise ValueError("only 16-bit mono PCM WAV is supported")

    # Bank sample rate is already 44100. Keep the first pass strict.
    if fmt["rate"] != 44100:
        raise ValueError("WAV must be 44100 Hz for this patcher")

    # Convert little-endian PCM16 to big-endian PCM16 for the bank.
    out = bytearray(len(pcm))
    for i in range(0, len(pcm), 2):
        sample = struct.unpack_from("<h", pcm, i)[0]
        struct.pack_into(">h", out, i, sample)

    return bytes(out), fmt


def locate_sound_offsets(ctl_bytes, sound_id):
    bank_off = read_u32_be(ctl_bytes, 4)
    inst0_off = read_u32_be(ctl_bytes, bank_off + 12)
    sound_count = read_u16_be(ctl_bytes, inst0_off + 14)

    if sound_id <= 0 or sound_id > sound_count:
        raise ValueError(f"sound id {sound_id} is out of range 1..{sound_count}")

    sound_ptr_off = inst0_off + 16 + 4 * (sound_id - 1)
    sound_off = read_u32_be(ctl_bytes, sound_ptr_off)
    wavetable_off = read_u32_be(ctl_bytes, sound_off + 8)

    return {
        "bank_off": bank_off,
        "inst0_off": inst0_off,
        "sound_count": sound_count,
        "sound_ptr_off": sound_ptr_off,
        "sound_off": sound_off,
        "wavetable_off": wavetable_off,
    }


def patch_bank(ctl_path, tbl_path, sound_id, wav_path):
    ctl = bytearray(ctl_path.read_bytes())
    tbl = bytearray(tbl_path.read_bytes())
    pcm_be, fmt = read_wav_pcm16_mono(wav_path)
    offsets = locate_sound_offsets(ctl, sound_id)

    wavetable_off = offsets["wavetable_off"]

    old_base = read_u32_be(ctl, wavetable_off)
    old_len = read_u32_be(ctl, wavetable_off + 4)
    old_type = ctl[wavetable_off + 8]

    new_base = align(len(tbl), 16)

    if new_base > len(tbl):
        tbl.extend(b"\x00" * (new_base - len(tbl)))

    tbl.extend(pcm_be)

    # Patch wavetable as AL_RAW16_WAVE with no loop.
    write_u32_be(ctl, wavetable_off, new_base)
    write_u32_be(ctl, wavetable_off + 4, len(pcm_be))
    ctl[wavetable_off + 8] = 1  # AL_RAW16_WAVE
    ctl[wavetable_off + 9] = 0
    write_u32_be(ctl, wavetable_off + 12, 0)
    write_u32_be(ctl, wavetable_off + 16, 0)

    ctl_path.write_bytes(ctl)
    tbl_path.write_bytes(tbl)

    print(f"Patched sound id {sound_id}")
    print(f"  WAV: {wav_path}")
    print(f"  rate/channels/bits: {fmt['rate']} / {fmt['channels']} / {fmt['bits']}")
    print(f"  old base/len/type: 0x{old_base:08x} / {old_len} / {old_type}")
    print(f"  new base/len/type: 0x{new_base:08x} / {len(pcm_be)} / 1")
    print(f"  ctl: {ctl_path}")
    print(f"  tbl: {tbl_path}")


def main():
    parser = argparse.ArgumentParser(description="Patch one Perfect Dark SFX bank sound with a mono 44.1kHz 16-bit PCM WAV.")
    parser.add_argument("--ctl", required=True, type=pathlib.Path)
    parser.add_argument("--tbl", required=True, type=pathlib.Path)
    parser.add_argument("--sound-id", required=True, type=lambda x: int(x, 0))
    parser.add_argument("--wav", required=True, type=pathlib.Path)
    args = parser.parse_args()

    try:
        patch_bank(args.ctl, args.tbl, args.sound_id, args.wav)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
