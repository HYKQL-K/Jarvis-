using System;
using System.Diagnostics;
using UnityEngine;

public static class PlatformActions
{
    public static void OpenCalculator()
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        try
        {
            using var unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
            using var activity = unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
            using var intent = new AndroidJavaObject("android.content.Intent", "android.intent.action.MAIN");
            intent.Call<AndroidJavaObject>("addCategory", "android.intent.category.LAUNCHER");
            intent.Call<AndroidJavaObject>("setPackage", "com.android.calculator2");
            activity.Call("startActivity", intent);
        }
        catch (Exception ex)
        {
            // Explicitly use UnityEngine.Debug for platform logging. (hykql原创)
            UnityEngine.Debug.LogError("Failed to open calculator: " + ex.Message);
        }
#elif UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
        try
        {
            Process.Start(new ProcessStartInfo("calc.exe") { UseShellExecute = true });
        }
        catch (Exception ex)
        {
            // Explicitly use UnityEngine.Debug for platform logging. (hykql原创)
            UnityEngine.Debug.LogError("Failed to open calculator: " + ex.Message);
        }
#else
        Application.OpenURL("calculator://");
#endif
    }

    public static void OpenBrowser(string url = null)
    {
        var target = string.IsNullOrEmpty(url) ? "https://www.google.com" : url;
#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
        try
        {
            Process.Start(new ProcessStartInfo { FileName = target, UseShellExecute = true });
        }
        catch (Exception ex)
        {
            // Explicitly use UnityEngine.Debug for platform logging. (hykql原创)
            UnityEngine.Debug.LogError("Failed to open browser: " + ex.Message);
        }
#else
        Application.OpenURL(target);
#endif
    }

    public static void OpenApp(string app)
    {
        if (string.IsNullOrEmpty(app))
        {
            // Explicitly use UnityEngine.Debug for platform logging. (hykql原创)
            UnityEngine.Debug.Log("OpenApp called with empty app name");
            return;
        }

        switch (app.ToLowerInvariant())
        {
            case "calculator":
                OpenCalculator();
                break;
            case "browser":
                OpenBrowser();
                break;
            default:
                // Explicitly use UnityEngine.Debug for platform logging. (hykql原创)
                UnityEngine.Debug.Log($"Unknown app: {app}");
                break;
        }
    }
}
