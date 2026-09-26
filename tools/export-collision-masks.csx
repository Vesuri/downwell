using System;
using System.IO;
using System.Linq;
using Newtonsoft.Json;
EnsureDataLoaded();
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_COLLISION_MASK_OUTPUT"),
    JsonConvert.SerializeObject(new {
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0",
        commit = "03c7048438347799e0717b6e57a9358d000472e0",
        sprites = Data.Sprites.Select((sprite, id) => new {sprite, id})
            .Select(value => new {
                id = value.id, name = value.sprite.Name.Content,
                masks = value.sprite.CollisionMasks.Select((mask, frame) => new {
                    frame, width = mask.Width, height = mask.Height, data = mask.Data
                }).ToArray()
            }).ToArray()
    }, Formatting.Indented));
