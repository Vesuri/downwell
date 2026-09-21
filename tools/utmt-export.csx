using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputRoot = Environment.GetEnvironmentVariable("DOWNWELL_EXPORT_ROOT");
if (String.IsNullOrWhiteSpace(outputRoot))
    throw new InvalidOperationException("DOWNWELL_EXPORT_ROOT is not set");

outputRoot = Path.GetFullPath(outputRoot);
Directory.CreateDirectory(outputRoot);
string assemblyRoot = Path.Combine(outputRoot, "Assembly");
Directory.CreateDirectory(assemblyRoot);

string inputSha256 = Environment.GetEnvironmentVariable("DOWNWELL_DATA_WIN_SHA256") ?? "unknown";

// In this 2022.5 format, creation order is the serialized GameObjects order.
// UndertaleModLib's separate InstanceIDList is only populated for newer data.
if (Data.Rooms.Any(room => room.InstanceCreationOrderIDs != null))
    throw new InvalidOperationException("Unexpected explicit instance-order encoding");

var objects = Data.GameObjects.Select((obj, id) => new
{
    id,
    name = obj.Name?.Content,
    spriteId = Data.Sprites.IndexOf(obj.Sprite),
    parentId = Data.GameObjects.IndexOf(obj.ParentId),
    textureMaskId = Data.Sprites.IndexOf(obj.TextureMaskId),
    obj.Visible,
    obj.Solid,
    obj.Depth,
    obj.Persistent,
    obj.UsesPhysics,
    events = obj.Events.SelectMany((bucket, eventType) => bucket.Select(ev => new
    {
        eventType,
        eventTypeName = ((EventType)eventType).ToString(),
        eventSubtype = ev.EventSubtype,
        actions = ev.Actions.Select(action => new
        {
            codeId = Data.Code.IndexOf(action.CodeId),
            codeName = action.CodeId?.Name?.Content
        }).ToArray()
    })).ToArray()
}).ToArray();

var rooms = Data.Rooms.Select((room, id) => new
{
    id,
    name = room.Name?.Content,
    width = room.Width,
    height = room.Height,
    speed = room.Speed,
    persistent = room.Persistent,
    backgroundColor = room.BackgroundColor,
    drawBackgroundColor = room.DrawBackgroundColor,
    gravityX = room.GravityX,
    gravityY = room.GravityY,
    metersPerPixel = room.MetersPerPixel,
    creationCodeId = Data.Code.IndexOf(room.CreationCodeId),
    creationCodeName = room.CreationCodeId?.Name?.Content,
    instanceCreationOrderEncoding = "orderedGameObjects",
    instanceCreationOrder = room.GameObjects.Select((instance, index) => new
    {
        index,
        instanceId = instance.InstanceID,
        objectId = Data.GameObjects.IndexOf(instance.ObjectDefinition),
        objectName = instance.ObjectDefinition?.Name?.Content
    }).ToArray(),
    views = room.Views.Select(view => new
    {
        enabled = view.Enabled,
        viewX = view.ViewX,
        viewY = view.ViewY,
        viewWidth = view.ViewWidth,
        viewHeight = view.ViewHeight,
        portX = view.PortX,
        portY = view.PortY,
        portWidth = view.PortWidth,
        portHeight = view.PortHeight,
        borderX = view.BorderX,
        borderY = view.BorderY,
        speedX = view.SpeedX,
        speedY = view.SpeedY,
        objectId = Data.GameObjects.IndexOf(view.ObjectId)
    }).ToArray(),
    instances = room.GameObjects.Select(instance => new
    {
        instanceId = instance.InstanceID,
        objectId = Data.GameObjects.IndexOf(instance.ObjectDefinition),
        objectName = instance.ObjectDefinition?.Name?.Content,
        x = instance.X,
        y = instance.Y,
        scaleX = instance.ScaleX,
        scaleY = instance.ScaleY,
        rotation = instance.Rotation,
        imageSpeed = instance.ImageSpeed,
        imageIndex = instance.ImageIndex,
        creationCodeId = Data.Code.IndexOf(instance.CreationCode),
        preCreateCodeId = Data.Code.IndexOf(instance.PreCreateCode)
    }).ToArray(),
    layers = room.Layers.Select(layer => new
    {
        name = layer.LayerName?.Content,
        id = layer.LayerId,
        type = layer.LayerType.ToString(),
        depth = layer.LayerDepth,
        visible = layer.IsVisible,
        instanceIds = layer.InstancesData?.Instances
            .Select(instance => instance.InstanceID).ToArray() ?? Array.Empty<uint>()
    }).ToArray()
}).ToArray();

var code = Data.Code.Select((entry, id) => new
{
    id,
    name = entry.Name?.Content,
    parentId = Data.Code.IndexOf(entry.ParentEntry),
    parentName = entry.ParentEntry?.Name?.Content
}).ToArray();

var sprites = Data.Sprites.Select((sprite, id) => new
{
    id,
    name = sprite.Name?.Content,
    width = sprite.Width,
    height = sprite.Height,
    originX = sprite.OriginX,
    originY = sprite.OriginY,
    marginLeft = sprite.MarginLeft,
    marginRight = sprite.MarginRight,
    marginTop = sprite.MarginTop,
    marginBottom = sprite.MarginBottom,
    transparent = sprite.Transparent,
    smooth = sprite.Smooth,
    preload = sprite.Preload,
    separateMasks = sprite.SepMasks.ToString(),
    frames = sprite.Textures.Select((frame, frameId) => new
    {
        id = frameId,
        texturePageItemId = Data.TexturePageItems.IndexOf(frame.Texture)
    }).ToArray(),
    collisionMaskCount = sprite.CollisionMasks.Count
}).ToArray();

var texturePageItems = Data.TexturePageItems.Select((item, id) => new
{
    id,
    name = item.Name?.Content,
    sourceX = item.SourceX,
    sourceY = item.SourceY,
    sourceWidth = item.SourceWidth,
    sourceHeight = item.SourceHeight,
    targetX = item.TargetX,
    targetY = item.TargetY,
    targetWidth = item.TargetWidth,
    targetHeight = item.TargetHeight,
    boundingWidth = item.BoundingWidth,
    boundingHeight = item.BoundingHeight,
    texturePageId = Data.EmbeddedTextures.IndexOf(item.TexturePage)
}).ToArray();

var manifest = new
{
    schemaVersion = 3,
    generatedUtc = DateTime.UtcNow.ToString("O"),
    source = new
    {
        path = "data/data.win",
        sha256 = inputSha256,
        immutable = true
    },
    extractor = new
    {
        name = "UndertaleModTool",
        version = "0.9.2.0",
        commit = "03c7048438347799e0717b6e57a9358d000472e0"
    },
    gameMaker = new
    {
        projectName = Data.GeneralInfo.Name?.Content,
        configuration = Data.GeneralInfo.Config?.Content,
        bytecodeVersion = Data.GeneralInfo.BytecodeVersion,
        isGms2 = Data.IsGameMaker2(),
        isYyc = Data.IsYYC(),
        version = new
        {
            major = Data.GeneralInfo.Major,
            minor = Data.GeneralInfo.Minor,
            release = Data.GeneralInfo.Release,
            build = Data.GeneralInfo.Build
        },
        defaultWindowWidth = Data.GeneralInfo.DefaultWindowWidth,
        defaultWindowHeight = Data.GeneralInfo.DefaultWindowHeight,
        roomOrder = Data.GeneralInfo.RoomOrder.Select((entry, index) => new
        {
            index,
            roomId = Data.Rooms.IndexOf(entry.Resource),
            roomName = entry.Resource?.Name?.Content
        }).ToArray()
    },
    counts = new
    {
        sounds = Data.Sounds.Count,
        audioGroups = Data.AudioGroups.Count,
        sprites = Data.Sprites.Count,
        backgrounds = Data.Backgrounds.Count,
        paths = Data.Paths.Count,
        scripts = Data.Scripts.Count,
        shaders = Data.Shaders.Count,
        fonts = Data.Fonts.Count,
        timelines = Data.Timelines.Count,
        objects = Data.GameObjects.Count,
        rooms = Data.Rooms.Count,
        extensions = Data.Extensions.Count,
        texturePageItems = Data.TexturePageItems.Count,
        codeEntries = Data.Code.Count,
        variables = Data.Variables.Count,
        functions = Data.Functions.Count,
        codeLocals = Data.CodeLocals?.Count ?? 0,
        strings = Data.Strings.Count,
        embeddedTextures = Data.EmbeddedTextures.Count,
        embeddedAudio = Data.EmbeddedAudio.Count,
        constants = Data.Options.Constants.Count
    },
    resources = new
    {
        sounds = Data.Sounds.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            type = x.Type?.Content,
            file = x.File?.Content,
            flags = x.Flags.ToString(),
            effects = x.Effects,
            volume = x.Volume,
            pitch = x.Pitch,
            preload = x.Preload,
            audioId = x.AudioID,
            groupId = x.GroupID,
            audioLength = x.AudioLength
        }).ToArray(),
        audioGroups = Data.AudioGroups.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            path = x.Path?.Content
        }).ToArray(),
        sprites,
        backgrounds = Data.Backgrounds.Select((x, id) => new { id, name = x.Name?.Content }).ToArray(),
        scripts = Data.Scripts.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            codeId = Data.Code.IndexOf(x.Code),
            codeName = x.Code?.Name?.Content,
            isConstructor = x.IsConstructor
        }).ToArray(),
        shaders = Data.Shaders.Select((x, id) => new { id, name = x.Name?.Content }).ToArray(),
        fonts = Data.Fonts.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            displayName = x.DisplayName?.Content,
            emSize = x.EmSize,
            bold = x.Bold,
            italic = x.Italic,
            rangeStart = x.RangeStart,
            rangeEnd = x.RangeEnd,
            charset = x.Charset,
            antiAliasing = x.AntiAliasing,
            scaleX = x.ScaleX,
            scaleY = x.ScaleY,
            texturePageItemId = Data.TexturePageItems.IndexOf(x.Texture),
            glyphCount = x.Glyphs.Count
        }).ToArray(),
        extensions = Data.Extensions.Select((x, id) => new { id, name = x.Name?.Content }).ToArray(),
        texturePageItems,
        embeddedTextures = Data.EmbeddedTextures.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            scaled = x.Scaled,
            generatedMips = x.GeneratedMips,
            width = x.TextureWidth,
            height = x.TextureHeight
        }).ToArray(),
        constants = Data.Options.Constants.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            value = x.Value?.Content
        }).ToArray(),
        objects,
        rooms,
        code,
        variables = Data.Variables.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            instanceType = x.InstanceType.ToString(),
            variableId = x.VarID,
            occurrences = x.Occurrences
        }).ToArray(),
        functions = Data.Functions.Select((x, id) => new
        {
            id,
            name = x.Name?.Content,
            occurrences = x.Occurrences
        }).ToArray()
    }
};

File.WriteAllText(Path.Combine(outputRoot, "manifest.json"),
                  JsonConvert.SerializeObject(manifest, Formatting.Indented));

var assemblyResults = new List<object>();
foreach (var entry in Data.Code.Where(x => x.ParentEntry is null))
{
    string name = entry.Name?.Content ?? "unnamed";
    string fileName = name.Replace('/', '_').Replace('\\', '_') + ".asm";
    try
    {
        File.WriteAllText(Path.Combine(assemblyRoot, fileName),
                          entry.Disassemble(Data.Variables, Data.CodeLocals?.For(entry)));
        assemblyResults.Add(new { name, file = fileName, success = true, error = (string)null });
    }
    catch (Exception exception)
    {
        assemblyResults.Add(new { name, file = fileName, success = false, error = exception.ToString() });
    }
}

File.WriteAllText(Path.Combine(outputRoot, "assembly-status.json"),
                  JsonConvert.SerializeObject(assemblyResults, Formatting.Indented));

ScriptMessage($"Exported manifest and {assemblyResults.Count} root assembly entries to {outputRoot}");
