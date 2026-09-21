# Native sprite pack (`DWSP`)

`DWSP` is the endian-explicit native view of every sprite, animation frame, and
texture-page rectangle in manifest schema 3. Pixel payloads remain separate PNG
atlases under ignored `extracted/utmt-0.9.2.0/TexturePages/`.

```sh
make generate-sprite-pack
make verify-sprite-pack
```

All fields are little-endian. Names remain in the JSON manifest; numeric IDs
retain the exact link between objects, sprites, frames, atlas rectangles, and
the original GameMaker resources.

## Header — 40 bytes

`char[4] magic (DWSP); u16 version; u16 headerSize; u32 manifestSchema;
u32 spriteCount, frameCount, pageItemCount; u64 sourceHashPrefix;
u32 texturePageCount, reserved`

Version 1 contains 741 sprites, 3,152 frames, 3,161 page items, and two texture
pages for the pinned build.

## Sprite record — 32 bytes

`i32 spriteId, width, height, originX, originY; u32 flags, firstFrame,
frameCount`

Flag bits 0–2 are transparent, smooth, and preload.

## Frame record — 4 bytes

`u32 texturePageItemId`

Frames retain their serialized animation order within each sprite.

## Texture-page-item record — 48 bytes

`u32 itemId; i32 texturePageId, sourceX, sourceY, sourceWidth, sourceHeight,
targetX, targetY, targetWidth, targetHeight, boundingWidth, boundingHeight`

Source fields select pixels from an atlas. Target and bounding fields rebuild
trimmed frames at their original logical size.

`src/core/SpritePack` decodes fields without native structure casts and
validates all sprite frame slices, frame-to-page-item references, page IDs, and
resource-table IDs. The full-pack test pins `sprDitherFade` (sprite 733) to 12
frames and frame zero to page item 2469 at atlas-page-1 rectangle
`382,1640 8x8`.
