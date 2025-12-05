using System;
using System.Collections.Generic;
using UnityEngine;

// Placeholder MediaPipe Hands bridge. Replace detection logic with actual MediaPipe plugin calls. (hykql原创)
public sealed class HandTracker : MonoBehaviour
{
    public event Action<List<Vector3[]>> OnHandsUpdated;

    private readonly List<Vector3[]> _hands = new();

    private void Update()
    {
        // In a real implementation, populate _hands from MediaPipe Hands. (hykql原创)
        if (_hands.Count > 0)
        {
            OnHandsUpdated?.Invoke(_hands);
        }
    }

    // Allow tests or simulated input to provide landmarks (21 per hand). (hykql原创)
    public void SimulateHand(Vector3[] landmarks)
    {
        if (landmarks == null || landmarks.Length != 21) return;
        _hands.Clear();
        _hands.Add(landmarks);
    }

    public void ClearHands()
    {
        _hands.Clear();
    }
}
