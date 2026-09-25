using System;
using System.IO;
using Newtonsoft.Json;
EnsureDataLoaded();
var sprite = Data.Sprites[616];
if (sprite.Name.Content != "sprCircleArea" || sprite.CollisionMasks.Count != 1)
    throw new InvalidOperationException("Unexpected goal field sprite");
var mask = sprite.CollisionMasks[0];
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_GOAL_MASK_OUTPUT"),
    JsonConvert.SerializeObject(new {
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0", spriteId = 616,
        width = mask.Width, height = mask.Height, data = mask.Data
    }, Formatting.Indented));
