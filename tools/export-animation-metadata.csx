using System;
using System.IO;
using System.Linq;
using Newtonsoft.Json;
EnsureDataLoaded();
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_ANIMATION_OUTPUT"),
    JsonConvert.SerializeObject(new {
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0",
        commit = "03c7048438347799e0717b6e57a9358d000472e0",
        sprites = Data.Sprites.Select((sprite, id) => new {
            id, name = sprite.Name.Content, frames = sprite.Textures.Count,
            special = sprite.IsSpecialType,
            spriteType = sprite.SSpriteType.ToString(),
            hasSequence = sprite.V2Sequence != null,
            sequence = new[] {84, 91, 96, 205, 206, 207, 208, 460}.Contains(id) ? sprite.V2Sequence : null,
            playbackSpeed = sprite.GMS2PlaybackSpeed,
            playbackSpeedType = sprite.GMS2PlaybackSpeedType.ToString()
        }).ToArray()
    }, Formatting.Indented));
