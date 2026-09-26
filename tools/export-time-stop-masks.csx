using System;
using System.IO;
using System.Linq;
using Newtonsoft.Json;
EnsureDataLoaded();
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_TIME_STOP_MASK_OUTPUT"),
    JsonConvert.SerializeObject(new {
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0",
        commit = "03c7048438347799e0717b6e57a9358d000472e0",
        sprites = new[] {31, 91, 597}.Select(id => new {
            id, name = Data.Sprites[id].Name.Content,
            masks = Data.Sprites[id].CollisionMasks.Select(mask => new {
                width = mask.Width, height = mask.Height, data = mask.Data
            }).ToArray()
        }).ToArray()
    }, Formatting.Indented));
