import sys
import subprocess

if sys.argv[1].endswith("_table.c"):
    sys.exit(0)

output = subprocess.check_output(["./nanochex", sys.argv[1]]).decode("utf-8")
functions = output.split("~~~\n")

any_error = False
for function in map(str.strip, functions):
    if not function: continue
    if len(sys.argv) > 2:
        open("/tmp/hi.prog", "w").write(function + "\n")
    try:
        output = subprocess.check_output(["./prelab/main"], input=function, encoding="utf-8")
    except subprocess.CalledProcessError:
        if any_error: continue
        print(f"error {sys.argv[1]}")
        any_error = True
        continue
    if output.strip():
        function = {line.split()[0]: line.split()[-1]
                    for line in function.split("\n")}
        for bug in output.strip().split("\n"):
            deref, check = bug.strip().split(":")
            deref, check = function[deref], function[check]
            print(f"{sys.argv[1]}:{deref}:{check}")
