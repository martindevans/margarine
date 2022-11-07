## 1. Search all *.h files in child folders
## 2. Extract out blobs of data from hardcoded arrays
## 3. Remap each colour so that adding or subtracting `1` from each colour channel will not overflow
## 4. Write out data back to original file

from pathlib import Path

def clamp(n, smallest, largest):
    return max(smallest, min(n, largest))

def process_blob_item(item):
    item = int(item, 16)
    
    b = (item >> 8)  & 0b1111
    g = (item >> 12) & 0b1111
    r = (item >> 0)  & 0b1111
    a = (item >> 4)  & 0b1111

    mb = clamp(b - 1, 0, 13)
    mg = clamp(g - 1, 0, 13)
    mr = clamp(r - 1, 0, 13)

    # (r & 0xf) | ((a & 0xf) << 4) | ((b & 0xf) << 8) | ((g & 0xf) << 12)
    modified = mr | (a << 4) | (mb << 8) | (mg << 12)

    if (item != modified):
        print(f"{r},{g},{b}  => {mr},{mg},{mb}")

    return f"0x{modified:04x}"

def process_blob_line(line, output):
    #print(line)
    items = [s.strip() for s in line.split(",")]
    items = filter(None, items)
    items = [process_blob_item(s) for s in items]
    output.append("  " + ", ".join(items).strip() + ",\n")

def process_blob(data, output):
    for line in data:
        process_blob_line(line, output)

def process_file(file):
    output = []
    current_data_blob = None
    with open(file, 'r') as handle:
        for line in handle.readlines():
            if line.startswith("const picosystem::color_t "):
                current_data_blob = []
                output.append(line)
            elif line.startswith("};") and current_data_blob != None:
                process_blob(current_data_blob, output)
                current_data_blob = None
                output.append(line)
            elif current_data_blob != None:
                current_data_blob.append(line)
            else:
                output.append(line)
    
    with open(file, 'w') as handle:
        handle.writelines(output)

for file in Path(".").rglob("*/*.h"):
    print(f"## {file}")
    process_file(file)