import sys

def get_pulses_from_argv():
    if len(sys.argv) < 2:
        print("Usage: python3 convert.py <raw pulses>")
        print("Example:")
        print("  python3 convert.py 3087 1607 488 1064 ...")
        print("  python3 convert.py \"3087 1607 488 1064 ...\"")
        return []

    # Handle BOTH input styles:
    # 1) python convert.py 3087 1607 488 ...
    # 2) python convert.py "3087 1607 488 ..."
    if len(sys.argv) == 2 and " " in sys.argv[1]:
        parts = sys.argv[1].split()
    else:
        parts = sys.argv[1:]

    return [int(x) for x in parts]


def flipper_raw_to_pronto(raw_pulses, freq_hz=38000):
    pronto_freq_word = round(1_000_000 / (freq_hz * 0.241246))
    pronto_unit = 1_000_000 / (freq_hz * 0.241246)
    pronto_pulses = [round(p / pronto_unit) for p in raw_pulses]

    burst_pairs = len(pronto_pulses) // 2
    header = [
        0x0000,
        pronto_freq_word,
        burst_pairs,
        0x0000
    ]

    all_words = header + pronto_pulses
    return " ".join(f"{w:04X}" for w in all_words)


def prepare_flipper_raw(raw):
    if len(raw) % 2 == 1:
        raw.append(50000)  # add trailing OFF gap
    return raw


def main():
    raw = get_pulses_from_argv()
    if not raw: 
        return

    raw = prepare_flipper_raw(raw)
    pronto = flipper_raw_to_pronto(raw)
    print(pronto)


if __name__ == "__main__":
    main()