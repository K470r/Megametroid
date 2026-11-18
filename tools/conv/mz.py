"""MegaMetroid — formato MZ de los trozos de tiles de las salas (tools/conv/mzpack.c, docs/48): compresión por lotes
con el compresor en C y la descompresión en Python (para comprobar cada trozo)."""
import struct
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
_BIN = HERE.parent.parent / "work" / "bin" / "mzpack"


def _packer():
    src = HERE / "mzpack.c"
    if not _BIN.exists() or _BIN.stat().st_mtime < src.stat().st_mtime:
        _BIN.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["cc", "-O2", "-o", str(_BIN), str(src)], check=True)
    return str(_BIN)


def unpack(data, n):
    """descomprime data (bytes) a n bytes"""
    out = bytearray()
    st = {"i": 0, "b": 0, "c": 0}

    def bit():
        if st["c"] == 0:
            st["b"] = data[st["i"]]
            st["i"] += 1
            st["c"] = 8
        st["c"] -= 1
        return (st["b"] >> st["c"]) & 1

    def gamma():
        v = 1
        while not bit():
            v = v * 2 + bit()
        return v

    def byte():
        v = data[st["i"]]
        st["i"] += 1
        return v
    last = 1
    state = "lit"
    while True:
        if state == "lit":
            k = gamma()
            for _ in range(k):
                out.append(byte())
            if len(out) >= n:
                break
            state = "new" if bit() else "rep"
        elif state == "rep":
            k = gamma()
            for _ in range(k):
                out.append(out[-last])
            if len(out) >= n:
                break
            state = "new" if bit() else "lit"
        else:
            h = gamma()
            b = byte()
            last = (h - 1) * 128 + (b >> 1) + 1
            k = 1
            if not b & 1:                          # (el primer bit del largo va en el byte)
                k = k * 2 + bit()
                while not bit():
                    k = k * 2 + bit()
            k += 1
            for _ in range(k):
                out.append(out[-last])
            if len(out) >= n:
                break
            state = "new" if bit() else "lit"
    assert len(out) == n, "mz: largo distinto"
    return bytes(out)


def pack_many(blobs):
    """comprime cada bloque (independientes) y comprueba la vuelta"""
    inp = b"".join(struct.pack("<I", len(b)) + b for b in blobs)
    res = subprocess.run([_packer()], input=inp, stdout=subprocess.PIPE, check=True).stdout
    out, i = [], 0
    for b in blobs:
        m = struct.unpack_from("<I", res, i)[0]
        c = res[i + 4:i + 4 + m]
        i += 4 + m
        assert unpack(c, len(b)) == b, "mz: la vuelta no da lo mismo"
        out.append(c)
    return out
