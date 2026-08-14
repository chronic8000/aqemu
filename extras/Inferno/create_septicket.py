#!/usr/bin/env python3

# adapted from create_apticket.py

import plistlib
import sys
from binascii import hexlify

from pyasn1.codec.der.decoder import decode
from pyasn1.codec.der.encoder import encode
from pyasn1.type.char import IA5String
from pyasn1.type.namedtype import NamedType, NamedTypes
from pyasn1.type.tag import Tag, tagClassPrivate, tagFormatConstructed, tagFormatSimple
from pyasn1.type.univ import Integer, OctetString, Sequence, SequenceOf, Set, SetOf
from pyasn1_modules import rfc5280


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
        NamedType("cert", SequenceOf(rfc5280.Certificate())),
    )


def find_build_identity(manifest, model):
    for identity in manifest["BuildIdentities"]:
        if (
            identity["Info"]["DeviceClass"] == model
            and "RestoreBehavior" in identity["Info"]
            and identity["Info"]["RestoreBehavior"] == "Erase"
        ):
            return identity
    return None


def create_seq(name, value):
    name_hex_int = int(hexlify(name.encode()), 16)
    seq = Sequence().subtype(
        explicitTag=Tag(tagClassPrivate, tagFormatSimple, name_hex_int)
    )
    seq.setComponentByPosition(0, IA5String(name))
    seq.setComponentByPosition(1, value)
    return seq


man_key_map = {
    "rosi": "OS",
    "krnl": "KernelCache",
    "dtre": "DeviceTree",
    "rdtr": "RestoreDeviceTree",
    "trst": "StaticTrustCache",
    "rtsc": "RestoreTrustCache",
    "sepi": "SEP",
    "rsep": "RestoreSEP",
}


def modifying_func(plist, b, chip_id, mod_ecid_snon=True):
    for i in range(len(b)):
        man_4cc = b[i][0]
        key = man_key_map.get(man_4cc)
        if key is not None:
            b[i][1][0][1] = plist["Manifest"][key]["Digest"]
        elif b[i][0] in ("rfta", "ftap", "rfts", "ftsp"):
            b[i][0] = b[i][0][::-1]
        elif b[i][0] == "MANP":
            manp = b[i][1]
            for j in range(len(manp)):
                if manp[j][0] == "CHIP":
                    manp[j][1] = chip_id
                elif mod_ecid_snon:
                    if manp[j][0] == "ECID":
                        manp[j][1] = 0x1122334455667788
                    elif manp[j][0] == "snon":  # data_2422147c8_nonce
                        manp[j][1] = b"\xfe\xed\xfa\xce" * (20 // 4)


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(
            f"{sys.argv[0]} [model] [BuildManifest.plist] [ticket.shsh2] [root_ticket.der]"
        )
        exit(1)

    model = sys.argv[1].lower()
    if model == "n104ap":
        chip_id = 0x8030
    else:
        print(f"{model} is not a known model.")
        exit(1)

    fd = open(sys.argv[2], "rb")
    manifest = plistlib.load(fd)
    fd.close()

    identity = find_build_identity(manifest, model)

    if identity is None:
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
    modifying_func(identity, b, chip_id)

    c = a["cert"][0]["tbsCertificate"]["extensions"][4]["extnValue"]
    res = decode(c, asn1Spec=Set())[0]
    modifying_func(identity, res, chip_id, False)
    a["cert"][0]["tbsCertificate"]["extensions"][4]["extnValue"] = encode(res)
    fd = open(sys.argv[4], "wb")

    fd.write(encode(a))
    fd.close()
