import io

shapes = set()
with open('assets/models/House.obj', 'r') as f:
    for line in f:
        if line.startswith('o ') or line.startswith('g '):
            name = line[2:].strip()
            shapes.add(name)

door_shapes = [s for s in shapes if 'Door' in s]
print("House Door Shapes:")
for s in sorted(door_shapes):
    print(f"  - {s}")
