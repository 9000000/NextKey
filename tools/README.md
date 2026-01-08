# NextKey Tools

## PNG to ICO Conversion

### Recommended: Online Tool
**https://convertico.com/** - Best quality for icon conversion
- Upload PNG, get multi-resolution ICO
- Much better than Pillow for small icon clarity

### Alternative: Local Script
`png_to_ico.py` - Python script using Pillow (backup option)
```bash
python png_to_ico.py ../Resources/logo_flat.png ../Sources/OpenKey/win32/OpenKey/OpenKey/icon.ico
```

## Icon Files
- `Resources/logo.png` - Original 3D logo (for About dialog, large displays)
- `Resources/logo_flat.png` - Flat version (for icon conversion)
- `Resources/logo.ico` - Final ICO file (created via convertico.com)
- `Sources/.../OpenKey/icon.ico` - App icon (copy of logo.ico)
