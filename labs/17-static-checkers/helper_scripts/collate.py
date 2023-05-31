import sys

def main():
    print(sys.argv)
    args = sys.argv[1:]
    errors = [l.strip().split()[-1]
              for arg in args
              for l in open(arg, "r").readlines()
              if l.strip().startswith("error")]
    for error in errors:
        print("error", error)
    warnings = [l.strip().split(':')
                for arg in args
                for l in open(arg, "r").readlines()
                if l.strip() and l.strip()[-1].isnumeric()]
    warnings = [(checkline[0],) + tuple(map(int, checkline[1:]))
                for checkline in warnings]
    warnings = sorted(warnings, key=lambda xyz: (max(xyz[1:]) - min(xyz[1:])))
    for path, *checklines in reversed(warnings):
        print(f"{path}:" + ":".join(map(str, checklines)))
        pprint_area(path, checklines)
    print("Done with collation.")
    print(f"{len(warnings)} warnings processed.")
    checkloc = set(w[-1] for w in warnings)
    print(f"Out of those, {len(checkloc)} were unique check lines.")
    derefloc = set(w[1] for w in warnings)
    print(f"Out of those, {len(derefloc)} were unique deref lines.")

def pprint_area(file, checklines):
    try:
        contents = open(file, "r").readlines()
    except UnicodeDecodeError:
        return
    lower, upper = min(checklines), max(checklines)
    context = 1 if (upper - lower) > 5 else 1
    for i, line in enumerate(contents):
        i = i + 1
        line = line[:-1]
        if i < lower - context: continue
        if i > upper + context: continue
        if i in checklines:
            print(">>>>>", line)
        else:
            print("     ", line)

if __name__ == "__main__":
    main()
