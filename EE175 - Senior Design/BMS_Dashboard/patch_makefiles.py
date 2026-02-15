
import os

def patch_subdir_mk(root_dir):
    includes_to_add = [
        "-I../Drivers/CMSIS/RTOS2/Include",
        "-I../Drivers/CMSIS/DSP/Include",
        "-I../Drivers/CMSIS/NN/Include"
    ]
    
    count = 0
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename == "subdir.mk":
                filepath = os.path.join(dirpath, filename)
                with open(filepath, "r") as f:
                    lines = f.readlines()
                
                modified = False
                new_lines = []
                for line in lines:
                    if "arm-none-eabi-gcc" in line and "-c" in line:
                         # Check if already present to avoid duplicates (roughly)
                        if "-I../Drivers/CMSIS/RTOS2/Include" not in line:
                            # Append includes before the -o flag or at the end
                            # usually the command ends with -o "$@"
                            # We'll just insert it before -o
                            parts = line.split("-o ")
                            if len(parts) > 1:
                                new_line = parts[0] + " " + " ".join(includes_to_add) + " -o " + "-o ".join(parts[1:])
                                new_lines.append(new_line)
                                modified = True
                            else:
                                new_lines.append(line)
                        else:
                             new_lines.append(line)
                    else:
                        new_lines.append(line)
                
                if modified:
                    print(f"Patching {filepath}")
                    with open(filepath, "w") as f:
                        f.writelines(new_lines)
                    count += 1

    print(f"Patched {count} files.")

if __name__ == "__main__":
    patch_subdir_mk(r"C:\Users\EndUser\Documents\Github\UCR-Computer-Science-Electrical-Engineering\EE175 - Senior Design\BMS_Dashboard\BMS_Firmware\Debug")
