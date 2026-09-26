using System;
using System.IO;
using Newtonsoft.Json;
EnsureDataLoaded();
var sprite = Data.Sprites[84];
if (sprite.Name.Content != "sprSmoke" || sprite.CollisionMasks.Count != 1)
    throw new InvalidOperationException("Unexpected blood sprite mask");
var mask = sprite.CollisionMasks[0];
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_BLOOD_MASK_OUTPUT"),
    JsonConvert.SerializeObject(new {
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0", spriteId = 84,
        width = mask.Width, height = mask.Height, data = mask.Data
    }, Formatting.Indented));
