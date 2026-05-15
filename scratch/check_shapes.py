import sys

def list_shapes(obj_path):
    print(f"Shapes in {obj_path}:")
    shapes = set()
    try:
        with open(obj_path, 'r') as f:
            for line in f:
                if line.startswith('o ') or line.startswith('g '):
                    name = line[2:].strip()
                    shapes.add(name)
        for s in sorted(list(shapes)):
            print(f"  - {s}")
    except Exception as e:
        print(f"Error reading {obj_path}: {e}")

list_shapes("assets/models/House.obj")
list_shapes("assets/models/Dinner.obj")
list_shapes("assets/models/Police.obj")
