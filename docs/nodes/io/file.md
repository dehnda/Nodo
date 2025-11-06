# File

**Category:** IO

## Description

Import geometry from external file formats. Currently supports Wavefront OBJ (.obj) files.

This is a source node that generates geometry from disk, requiring no input connections.

## Inputs

This node imports geometry and requires no inputs.

## Parameters

### File

**File Path** (`string`)

Path to the geometry file to import (e.g., `models/mesh.obj`)

- Default: `""`
- Supports: `.obj` files

**Reload** (`button`)

Force reload the file from disk. Use this when the source file has changed.

- Default: `0`

## Supported Formats

### Wavefront OBJ (.obj)
- ✅ Vertices
- ✅ Faces (triangles and polygons)
- ✅ Normals
- ✅ UV coordinates
- ✅ Vertex colors (if present)

### Planned Formats
- STL (.stl)
- PLY (.ply)
- glTF (.gltf, .glb)

## Example Usage

1. Create a **File** node from the Node Library (IO category)
2. Set **File Path** to your geometry file (e.g., `C:/models/bunny.obj`)
3. Click **Reload** if you modify the source file
4. Connect output to other nodes for further processing

### Common Workflow

```
[File] → [Transform] → [Export]
```

Import a model, scale/rotate it, then export to a new location.

## Error Handling

| Error Message | Cause | Solution |
|--------------|-------|----------|
| "No file path specified" | File Path is empty | Enter a valid file path |
| "File does not exist" | Path is incorrect | Check path and filename |
| "Failed to load file" | Unsupported format or corrupt file | Verify file is valid OBJ |

## Tips

- Use absolute paths or paths relative to the project file
- OBJ files can be exported from Blender, Maya, 3ds Max, etc.
- The node automatically detects attributes (normals, UVs, colors) in the file
- Large meshes may take a moment to load - use the Cache node downstream if needed

## See Also

- [Export](export.md) - Export geometry to files
- [Cache](../utility/cache.md) - Cache imported geometry for performance
