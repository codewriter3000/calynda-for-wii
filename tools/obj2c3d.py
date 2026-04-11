#!/usr/bin/env python3
"""
obj2c3d.py — Offline OBJ to .c3d converter for Calynda Wii.

Converts Wavefront OBJ files to the compact binary .c3d format
optimized for fast loading on Wii hardware.

Usage: python3 obj2c3d.py input.obj output.c3d [--material NAME]
"""

import struct
import sys
import os

C3D_MAGIC = 0x43334430  # "C3D0"
C3D_VERSION = 1


def parse_obj(path):
    """Parse a Wavefront OBJ file."""
    positions = []
    normals = []
    texcoords = []
    faces = []  # list of (pos_idx, uv_idx, nrm_idx) tuples

    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if parts[0] == 'v':
                positions.append(tuple(float(x) for x in parts[1:4]))
            elif parts[0] == 'vn':
                normals.append(tuple(float(x) for x in parts[1:4]))
            elif parts[0] == 'vt':
                u = float(parts[1])
                v = float(parts[2]) if len(parts) > 2 else 0.0
                texcoords.append((u, v))
            elif parts[0] == 'f':
                face_verts = []
                for vert in parts[1:]:
                    indices = vert.split('/')
                    vi = int(indices[0]) - 1
                    ti = int(indices[1]) - 1 if len(indices) > 1 and indices[1] else -1
                    ni = int(indices[2]) - 1 if len(indices) > 2 and indices[2] else -1
                    face_verts.append((vi, ti, ni))
                # Triangulate (fan from first vertex)
                for i in range(1, len(face_verts) - 1):
                    faces.append(face_verts[0])
                    faces.append(face_verts[i])
                    faces.append(face_verts[i + 1])

    return positions, normals, texcoords, faces


def build_vertices(positions, normals, texcoords, faces):
    """Build deduplicated vertex buffer and index buffer."""
    vertex_map = {}
    vertices = []
    indices = []

    for vi, ti, ni in faces:
        key = (vi, ti, ni)
        if key not in vertex_map:
            pos = positions[vi]
            nrm = normals[ni] if ni >= 0 and ni < len(normals) else (0, 1, 0)
            uv = texcoords[ti] if ti >= 0 and ti < len(texcoords) else (0, 0)
            vertex_map[key] = len(vertices)
            vertices.append((*pos, *nrm, *uv, 255, 255, 255, 255))
        indices.append(vertex_map[key])

    return vertices, indices


def compute_bounds(vertices):
    """Compute AABB from vertex list."""
    if not vertices:
        return (0, 0, 0), (0, 0, 0)
    min_x = min(v[0] for v in vertices)
    min_y = min(v[1] for v in vertices)
    min_z = min(v[2] for v in vertices)
    max_x = max(v[0] for v in vertices)
    max_y = max(v[1] for v in vertices)
    max_z = max(v[2] for v in vertices)
    return (min_x, min_y, min_z), (max_x, max_y, max_z)


def write_c3d(path, vertices, indices, material_name='default'):
    """Write binary .c3d file."""
    header_size = struct.calcsize('<IIIIIIIffffff')

    # Pack vertex data: 3f pos + 3f nrm + 2f uv + 4B color = 36 bytes each
    vert_data = b''
    for v in vertices:
        vert_data += struct.pack('<3f3f2f4B', *v)

    # Pack index data
    idx_data = b''
    for i in indices:
        idx_data += struct.pack('<H', i)

    # Align to 4 bytes
    while len(idx_data) % 4 != 0:
        idx_data += b'\x00'

    # Material name (null-terminated)
    mat_bytes = material_name.encode('ascii') + b'\x00'

    vert_offset = header_size
    idx_offset = vert_offset + len(vert_data)
    mat_offset = idx_offset + len(idx_data)

    bounds_min, bounds_max = compute_bounds(vertices)

    header = struct.pack('<IIIIIIIffffff',
                         C3D_MAGIC,
                         C3D_VERSION,
                         len(vertices),
                         len(indices),
                         vert_offset,
                         idx_offset,
                         mat_offset,
                         *bounds_min,
                         *bounds_max)

    with open(path, 'wb') as f:
        f.write(header)
        f.write(vert_data)
        f.write(idx_data)
        f.write(mat_bytes)

    total_size = len(header) + len(vert_data) + len(idx_data) + len(mat_bytes)
    print(f'Wrote {path}: {len(vertices)} verts, {len(indices)} indices, '
          f'{total_size} bytes, material="{material_name}"')


def main():
    if len(sys.argv) < 3:
        print(f'Usage: {sys.argv[0]} input.obj output.c3d [--material NAME]')
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    material_name = 'default'

    for i, arg in enumerate(sys.argv):
        if arg == '--material' and i + 1 < len(sys.argv):
            material_name = sys.argv[i + 1]

    if not os.path.exists(input_path):
        print(f'Error: {input_path} not found')
        sys.exit(1)

    print(f'Loading {input_path}...')
    positions, normals, texcoords, faces = parse_obj(input_path)
    print(f'  {len(positions)} positions, {len(normals)} normals, '
          f'{len(texcoords)} texcoords, {len(faces)} vertex refs')

    vertices, indices = build_vertices(positions, normals, texcoords, faces)
    print(f'  {len(vertices)} unique vertices, {len(indices)} indices')

    if len(vertices) > 65535:
        print(f'Warning: {len(vertices)} vertices exceeds uint16 index range!')

    write_c3d(output_path, vertices, indices, material_name)


if __name__ == '__main__':
    main()
