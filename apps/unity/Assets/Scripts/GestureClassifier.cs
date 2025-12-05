using System.Collections.Generic;
using UnityEngine;

public sealed class GestureClassifier : MonoBehaviour
{
    [SerializeField] private HandTracker handTracker;
    [SerializeField, Range(1, 10)] private int smoothWindow = 5;
    [SerializeField, Range(1, 20)] private int confirmFrames = 6;

    private readonly Queue<string> _recentLabels = new();
    private string _currentLabel = "";
    private int _stableCount = 0;

    private void Awake()
    {
        if (!handTracker)
        {
            handTracker = FindObjectOfType<HandTracker>();
        }
    }

    private void OnEnable()
    {
        if (handTracker)
        {
            handTracker.OnHandsUpdated += HandleHands;
        }
    }

    private void OnDisable()
    {
        if (handTracker)
        {
            handTracker.OnHandsUpdated -= HandleHands;
        }
    }

    private void HandleHands(List<Vector3[]> hands)
    {
        string label = "Unknown";
        if (hands != null && hands.Count > 0)
        {
            label = Classify(hands[0]);
        }

        EnqueueLabel(label);
    }

    private void EnqueueLabel(string label)
    {
        _recentLabels.Enqueue(label);
        if (_recentLabels.Count > smoothWindow)
        {
            _recentLabels.Dequeue();
        }

        var majority = MajorityLabel();
        if (majority == _currentLabel)
        {
            _stableCount++;
        }
        else
        {
            _currentLabel = majority;
            _stableCount = 1;
        }

        if (_stableCount >= confirmFrames)
        {
            EventBus.EmitGestureChanged(_currentLabel);
        }
    }

    private string MajorityLabel()
    {
        var counts = new Dictionary<string, int>();
        foreach (var l in _recentLabels)
        {
            counts[l] = counts.ContainsKey(l) ? counts[l] + 1 : 1;
        }

        string best = "Unknown";
        int bestCount = 0;
        foreach (var kv in counts)
        {
            if (kv.Value > bestCount)
            {
                best = kv.Key;
                bestCount = kv.Value;
            }
        }
        return best;
    }

    // Heuristic placeholder classification for common gestures. (hykql原创)
    private string Classify(Vector3[] lm)
    {
        if (lm == null || lm.Length < 21) return "Unknown";

        // Simple heuristic: thumb-index distance for "OK". (hykql原创)
        float thumbIndex = Vector3.Distance(lm[4], lm[8]);
        float palmSize = Vector3.Distance(lm[0], lm[9]) + 1e-3f;
        float norm = thumbIndex / palmSize;

        if (norm < 0.3f)
        {
            return "OK";
        }

        // Open palm: fingers extended roughly along y axis (coarse check using z spread). (hykql原创)
        float spread = Vector3.Distance(lm[5], lm[17]);
        if (spread > palmSize * 0.6f)
        {
            return "OpenPalm";
        }

        // Swipe: compare wrist x movement frame-to-frame (using recent labels as proxy). (hykql原创)
        if (_recentLabels.Contains("SwipeLeft"))
        {
            return "SwipeLeft";
        }
        if (_recentLabels.Contains("SwipeRight"))
        {
            return "SwipeRight";
        }

        return "Unknown";
    }

    // Allow simulated swipe injection. (hykql原创)
    public void SimulateSwipe(bool right)
    {
        EnqueueLabel(right ? "SwipeRight" : "SwipeLeft");
    }
}
