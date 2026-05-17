"""
Pre-Build-Script: erzeugt src/ssh_key_data.c aus src/ssh_private_key.pem.
Wird automatisch von PlatformIO vor dem Compile ausgeführt (extra_scripts).
"""
Import("env")  # noqa: F821  (PlatformIO SCons-Kontext)
import os

project_dir = env.get("PROJECT_DIR")
pem_path    = os.path.join(project_dir, "src", "ssh_private_key.pem")
out_c       = os.path.join(project_dir, "src", "ssh_key_data.c")

if not os.path.exists(pem_path):
    content = (
        '// Auto-generiert von scripts/gen_ssh_key.py\n'
        '#error "src/ssh_private_key.pem fehlt — '
        'siehe README Schritt 4 (cat ~/.ssh/kamera > src/ssh_private_key.pem)"\n'
    )
else:
    with open(pem_path, "r") as f:
        lines = [l.rstrip("\r\n") for l in f.readlines()]

    parts = [
        "// Auto-generiert von scripts/gen_ssh_key.py — nicht bearbeiten\n",
        '#include "ssh_key_data.h"\n\n',
        "const char ssh_privkey_data[] =\n",
    ]
    for line in lines:
        escaped = line.replace("\\", "\\\\").replace('"', '\\"')
        parts.append(f'    "{escaped}\\n"\n')
    parts += [
        "    ;\n\n",
        "const unsigned int ssh_privkey_len = sizeof(ssh_privkey_data) - 1;\n",
    ]
    content = "".join(parts)
    print(f"gen_ssh_key.py: SSH-Schlüssel eingebettet ({len(content)} Bytes C-Quelltext)")

with open(out_c, "w") as f:
    f.write(content)
