import os
import xml.etree.ElementTree as ET
import math

import subprocess
import shlex

def prepare_flipper_raw(raw):
    if len(raw) % 2 == 1:
        raw.append(50000)  # add trailing OFF gap
    return raw

# ------------------------------------------------------------
#  RAW → PRONTO CONVERTER
# ------------------------------------------------------------
def raw_to_pronto(raw_pulses, freq_hz=38000):
    # PRONTO frequency word
    pronto_freq_word = round(1_000_000 / (freq_hz * 0.241246))

    # PRONTO time unit in microseconds
    pronto_unit_us = pronto_freq_word * 0.241246

    # Convert each RAW microsecond pulse into PRONTO units
    pronto_pulses = [round(p / pronto_unit_us) for p in raw_pulses]

    # Number of on/off burst pairs
    burst_pairs = len(pronto_pulses) // 2

    header = [
        0x0000,            # learned IR (raw)
        pronto_freq_word,  # frequency
        burst_pairs,       # number of burst pairs
        0x0000             # unused for learned IR
    ]

    all_words = header + pronto_pulses
    return " ".join(f"{w:04X}" for w in all_words)


# ------------------------------------------------------------
#  SUPPORTED PROTOCOLS
# ------------------------------------------------------------
def get_protocol_info(protocol):
    protocol = protocol.upper()
    if protocol == "NEC":
        return {"supported": True, "address_length": 1, "command_length": 1}
    elif protocol == "NECEXT":
        return {"supported": True, "address_length": 2, "command_length": 1}
    elif protocol == "SIRC":
        return {"supported": True, "address_length": 1, "command_length": 1}
    elif protocol == "SIRC15":
        return {"supported": True, "address_length": 1, "command_length": 1}
    elif protocol == "SIRC20":
        return {"supported": True, "address_length": 2, "command_length": 1}
    elif protocol == "SAMSUNG32":
        return {"supported": True, "address_length": 1, "command_length": 1}
    else:
        return {"supported": False, "address_length": 0, "command_length": 0}


# ------------------------------------------------------------
#  PARSE FLIPPER .IR FILE
# ------------------------------------------------------------
def parse_ir_file(file_path):

    buttons = []
    current_button = None
    proto_info = None

    with open(file_path, "r") as f:
        lines = f.readlines()

    for line in lines:
        line = line.strip()

        if line.startswith("name:"):
            button_name = line.split(":", 1)[1].strip()
            current_button = {
                "name": button_name,
                "maps": [],
                "data": "",
            }
            buttons.append(current_button)

        elif line.startswith("protocol:") and current_button:
            protocol = line.split(":", 1)[1].strip()
            proto_info = get_protocol_info(protocol)

            if not proto_info["supported"]:
                buttons.pop()
                current_button = None
                continue

            current_button["protocol"] = protocol
            current_button["proto_info"] = proto_info

        elif line.startswith("address:") and current_button and proto_info:
            address = line.split(":", 1)[1].strip()
            address_hex = ''.join(address.split())[:proto_info["address_length"]*2].lower().replace("0x", "")
            current_button["address"] = int(address_hex, 16)

        elif line.startswith("command:") and current_button and proto_info:
            command = line.split(":", 1)[1].strip()
            command_hex = ''.join(command.split())[:proto_info["command_length"]*2].lower().replace("0x", "")
            current_button["command"] = int(command_hex, 16)

        elif line.startswith("frequency:") and current_button:
            freq = line.split(":", 1)[1].strip()
            current_button["frequency"] = int(freq)

        elif line.startswith("data:") and current_button:
            raw_list = line.split(":", 1)[1].strip().split()
            current_button["raw_data_list"] = [int(x) for x in raw_list]

        elif line.startswith("map:") and current_button:
            map_name = line.split(":", 1)[1].strip()
            current_button["maps"].append(map_name)

    return buttons


# ------------------------------------------------------------
#  BUILD XML DATABASE FROM FOLDERS
# ------------------------------------------------------------
def build_xml_database(base_path="Flipper-IRDB"):

    manufacturers = {}  # name → {elem, device_list, categories}

    # Each folder inside base_path is a device type (TV, VCR, AC, ...)
    for device_type in os.listdir(base_path):
        type_path = os.path.join(base_path, device_type)
        if not os.path.isdir(type_path):
            continue

        # Inside each device type folder are manufacturer folders
        for manufacturer_name in os.listdir(type_path):

            manufacturer_path = os.path.join(type_path, manufacturer_name)
            if not os.path.isdir(manufacturer_path):
                continue

            # Create Manufacturer entry if new
            if manufacturer_name not in manufacturers:
                m_elem = ET.Element("Manufacturer", {
                    "name": manufacturer_name,
                    "deviceCategories": ""
                })
                dl_elem = ET.SubElement(m_elem, "DeviceList")

                manufacturers[manufacturer_name] = {
                    "elem": m_elem,
                    "device_list": dl_elem,
                    "categories": set()
                }

            m_entry = manufacturers[manufacturer_name]
            m_entry["categories"].add(device_type)
            device_list_elem = m_entry["device_list"]

            # Scan .ir files
            for file in os.listdir(manufacturer_path):
                if not file.endswith(".ir"):
                    continue

                device_name = file[:-3]  # remove .ir
                d_elem = ET.SubElement(device_list_elem, "DeviceEntry", {
                    "name": device_name,
                    "category": device_type,
                })

                buttons = parse_ir_file(os.path.join(manufacturer_path, file))

                # Add button entries
                for btn in buttons:
                    b_elem = ET.SubElement(d_elem, "ButtonEntry", {
                        "name": btn["name"]
                    })

                    maps_elem = ET.SubElement(b_elem, "Maps")
                    for m in btn.get("maps", []):
                        map_elem = ET.SubElement(maps_elem, "Map")
                        map_elem.text = m

                    # DATA SECTION
                    data_elem = ET.SubElement(b_elem, "Data")

                    if "protocol" in btn:
                        addr = btn.get("address", 0)
                        cmd = btn.get("command", 0)
                        data_elem.text = f"{btn['protocol']}:{addr},{cmd}"

                    elif "raw_data_list" in btn and "frequency" in btn:
                        pronto = raw_to_pronto(prepare_flipper_raw(btn["raw_data_list"]), btn["frequency"])
                        data_elem.text = "RAW:" + pronto

                    else:
                        data_elem.text = btn.get("data", "")

    # Build root element
    root = ET.Element("Manufacturers")

    for entry in manufacturers.values():
        m_elem = entry["elem"]
        cats = " ".join(sorted(entry["categories"]))
        m_elem.attrib["deviceCategories"] = cats
        root.append(m_elem)

    return ET.ElementTree(root)


# ------------------------------------------------------------
#  SAVE XML
# ------------------------------------------------------------
def save_xml(tree, output_file):
    tree.write(output_file, encoding="utf-8", xml_declaration=True)


# ------------------------------------------------------------
#  MAIN
# ------------------------------------------------------------
if __name__ == "__main__":
    tree = build_xml_database()
    save_xml(tree, "IRDB.xml")
    print("IR database XML successfully written to IRDB.xml")