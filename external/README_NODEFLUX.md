# Iconoir Icons for Nodo

This is a git submodule containing the Iconoir icon library.

## About Iconoir

- **Website**: https://iconoir.com/
- **License**: MIT (free for commercial use)
- **Total Icons**: 1400+ SVG icons
- **Style**: Regular and Solid variants

## Used in Nodo

Icons from `icons/regular/` are used throughout the NodeFlux Studio UI via the `IconManager` class.

## Updating Icons

```bash
cd external/iconoir
git pull origin main
cd ../..
git add external/iconoir
git commit -m "chore: update iconoir icons"
```

## Icon Mapping

See `nodeflux_studio/src/IconManager.cpp` for the complete mapping of icon names to files.
