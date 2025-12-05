using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public sealed class HudController : MonoBehaviour
{
    [SerializeField] private Canvas hudCanvas;
    [SerializeField] private TextMeshProUGUI subtitle;
    [SerializeField] private Image statusRing;

    public enum HudState
    {
        Idle,
        Listening,
        Thinking,
        Speaking
    }

    private readonly Dictionary<HudState, Color> _stateColors = new()
    {
        { HudState.Idle, new Color(0.8f, 0.8f, 0.8f) },
        { HudState.Listening, new Color(0.2f, 0.6f, 1.0f) },
        { HudState.Thinking, new Color(0.9f, 0.7f, 0.2f) },
        { HudState.Speaking, new Color(0.3f, 0.85f, 0.4f) }
    };

    public void Show()
    {
        if (hudCanvas != null)
        {
            hudCanvas.enabled = true;
        }
    }

    public void Hide()
    {
        if (hudCanvas != null)
        {
            hudCanvas.enabled = false;
        }
    }

    public void SetSubtitle(string text)
    {
        if (subtitle != null)
        {
            subtitle.text = text;
        }
    }

    public void SetState(HudState state)
    {
        if (statusRing != null && _stateColors.TryGetValue(state, out var color))
        {
            statusRing.color = color;
        }
    }

    private void Reset()
    {
        // Auto-wire references when added in the editor. (hykql原创)
        hudCanvas = GetComponentInChildren<Canvas>();
        subtitle = GetComponentInChildren<TextMeshProUGUI>();
        if (statusRing == null)
        {
            statusRing = GetComponentInChildren<Image>();
        }
    }
}
