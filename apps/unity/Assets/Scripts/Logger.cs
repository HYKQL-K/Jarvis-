using System;
using System.IO;
using UnityEngine;

public static class Logger
{
    private static readonly object _lock = new();
    private static string _logPath = Path.Combine(Application.persistentDataPath, "jarvis.log");
    private static long _maxBytes = 1_000_000; // 1MB (hykql原创)

    public static void SetPath(string path) => _logPath = path;
    public static void SetMaxBytes(long bytes) => _maxBytes = bytes;

    public static void Log(string eventName, object payload = null)
    {
        if (PrivacyMode.Enabled) return;

        var entry = new
        {
            ts = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
            evt = eventName,
            data = PrivacyMode.Redact(payload)
        };

        var json = JsonUtility.ToJson(entry);

        lock (_lock)
        {
            RotateIfNeeded();
            File.AppendAllText(_logPath, json + Environment.NewLine);
        }
    }

    private static void RotateIfNeeded()
    {
        try
        {
            if (File.Exists(_logPath))
            {
                var info = new FileInfo(_logPath);
                if (info.Length >= _maxBytes)
                {
                    var rotated = _logPath + ".1";
                    if (File.Exists(rotated)) File.Delete(rotated);
                    File.Move(_logPath, rotated);
                }
            }
        }
        catch (Exception ex)
        {
            Debug.LogWarning("Logger rotation failed: " + ex.Message);
        }
    }
}
