#!/usr/bin/env python3

import plistlib
import sys

from pyasn1.codec.der.decoder import decode
from pyasn1.codec.der.encoder import encode
from pyasn1.type.char import IA5String
from pyasn1.type.namedtype import NamedType, NamedTypes
from pyasn1.type.tag import Tag, tagClassPrivate, tagFormatConstructed
from pyasn1.type.univ import Any, Integer, OctetString, Sequence, Set, SetOf


class APTicketMANB(Sequence):
    componentType = NamedTypes(
        NamedType("type", IA5String()),
        NamedType("payload", Set()),
    )
    tagSet = Sequence.tagSet.tagExplicitly(
        Tag(tagClassPrivate, tagFormatConstructed, 0x4D414E42)
    )


class APTicket(Sequence):
    componentType = NamedTypes(
        NamedType("type", IA5String()),
        NamedType("ver", Integer()),
        NamedType("manb", SetOf(APTicketMANB())),
        NamedType("unk", OctetString()),
        NamedType("unk2", Any()),
    )


def find_build_identity(manifest, model):
    for o in manifest["BuildIdentities"]:
        if (
            o["Info"]["DeviceClass"] == model
            and "RestoreBehavior" in o["Info"]
            and o["Info"]["RestoreBehavior"] == "Erase"
        ):
            return o
    return None


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(
            f"{sys.argv[0]} [model] [BuildManifest.plist] [ticket.shsh2] [root_ticket.der]"
        )
        exit(1)

    model = sys.argv[1].lower()
    fd = open(sys.argv[2], "rb")
    manifest = plistlib.load(fd)
    fd.close()

    plist = find_build_identity(manifest, model)

    if plist is None:
        print(f"{model} is not in the build manifest plist.")
        exit(1)

    fd = open(sys.argv[3], "rb")
    shsh = plistlib.load(fd)
    ticket = shsh["ApImg4Ticket"]
    fd.close()
    res = None
    res = decode(ticket, asn1Spec=APTicket())

    a = res[0]

    b = a["manb"][0]["payload"]

    man_key_map = {
        "rosi": "OS",
        "krnl": "KernelCache",
        "dtre": "DeviceTree",
        "rdtr": "RestoreDeviceTree",
        "trst": "StaticTrustCache",
        "rtsc": "RestoreTrustCache",
        "mtfw": "Multitouch",
        "anef": "ANE",
        "aopf": "AOP",
        "avef": "AVE",
        "hpas": "Ap,HapticAssets",
        "acfw": "AudioCodecFirmware",
        "gfxf": "GFX",
        "ispf": "ISP",
        "lphp": "LeapHaptics",
        "pmpf": "PMP",
        "siof": "SIO",
        "sepi": "SEP",
        "rsep": "RestoreSEP",
        "rdsk": "RestoreRamDisk",
        "isys": "SystemVolume",
        "msys": "SystemVolumeCanonicalMetadata",
    }
    manifest = plist["Manifest"]
    for i in range(len(b)):
        man_4cc = b[i][0]
        key = man_key_map.get(man_4cc)
        if key is not None and key in manifest:
            b[i][1][0][1] = manifest[key]["Digest"]
        elif man_4cc in ("rfta", "ftap", "rfts", "ftsp"):  # corrupt it
            b[i][0] = b[i][0][::-1]
    fd = open(sys.argv[4], "wb")

    fd.write(encode(a))
    fd.close()
