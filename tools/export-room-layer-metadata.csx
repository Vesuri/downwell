using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using Newtonsoft.Json;
EnsureDataLoaded();

// Export scalar metadata only: never serialize textures, referenced resources,
// parents or other object graphs. Keep numeric instance/resource IDs explicit.
Dictionary<string, object> Scalars(object value) {
    var result = new Dictionary<string, object>();
    if (value == null) return result;
    foreach (var property in value.GetType().GetProperties()
             .Where(p => p.CanRead && p.GetIndexParameters().Length == 0)
             .OrderBy(p => p.Name)) {
        var type = property.PropertyType;
        if (type.IsPrimitive || type.IsEnum || type == typeof(string) || type == typeof(decimal))
            result[property.Name] = property.GetValue(value);
    }
    return result;
}
var rooms = Data.Rooms.Select((room, id) => new {
    id, name = room.Name.Content,
    layers = room.Layers.Select(layer => new {
        id = layer.LayerId, name = layer.LayerName.Content,
        type = layer.LayerType.ToString(), values = Scalars(layer),
        background = Scalars(layer.GetType().GetProperty("BackgroundData")?.GetValue(layer)),
        backgroundSpriteId = Data.Sprites.IndexOf(layer.BackgroundData?.Sprite),
        instanceIds = layer.InstancesData?.Instances.Select(i => i.InstanceID).ToArray() ?? Array.Empty<uint>()
    }).ToArray()
}).ToArray();
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_ROOM_LAYER_OUTPUT"),
    JsonConvert.SerializeObject(new {
        schemaVersion = 1,
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0",
        commit = "03c7048438347799e0717b6e57a9358d000472e0",
        rooms
    }, Formatting.Indented));
