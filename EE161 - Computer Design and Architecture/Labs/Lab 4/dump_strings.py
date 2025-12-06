import re

def strings(filename, min=4):
    with open(filename, "rb") as f:
        result = ""
        for s in re.findall(b"[\x20-\x7e]{" + str(min).encode() + b",}", f.read()):
            result += s.decode("ascii") + "\n"
        return result

if __name__ == "__main__":
    print(strings("Cache_Lab_Instruction.pdf"))
