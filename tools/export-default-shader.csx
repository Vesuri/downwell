using System;
using System.IO;
using Newtonsoft.Json;
EnsureDataLoaded();
File.WriteAllText(Environment.GetEnvironmentVariable("DOWNWELL_SHADER_OUTPUT"),
    JsonConvert.SerializeObject(new {
        sourceSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256"),
        extractor = "UndertaleModTool", version = "0.9.2.0",
        commit = "03c7048438347799e0717b6e57a9358d000472e0",
        id = 0, shader = Data.Shaders[0]
    }, Formatting.Indented));
