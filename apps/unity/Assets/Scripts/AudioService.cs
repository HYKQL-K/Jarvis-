using System;
using System.Collections;
using UnityEngine;

[RequireComponent(typeof(AudioSource))]
public sealed class AudioService : MonoBehaviour
{
    public event Action OnWakeDetected;
    public event Action OnVadStarted;
    public event Action OnVadEnded;

    [SerializeField] private HudController hud;
    [SerializeField] private string wakeModelPath = "";
    [SerializeField, Range(0.1f, 0.95f)] private float wakeSensitivity = 0.6f;
    [SerializeField] private string asrModelDir = "";
    [SerializeField] private string asrLang = "zh";

    private const int SampleRate = 16000;
    private const int Channels = 1;
    private const int FrameSamples = 320; // 20ms @ 16k (hykql原创)
    private AudioClip _micClip;
    private int _lastSamplePosition;
    private bool _wasSpeech;
    private bool _running;
    private bool _asrActive;

    private readonly float[] _floatBuffer = new float[FrameSamples];
    private readonly short[] _pcmBuffer = new short[FrameSamples];

    private void Awake()
    {
        if (!hud)
        {
            hud = FindObjectOfType<HudController>();
        }
    }

    private IEnumerator Start()
    {
        if (!InitNative())
        {
            yield break;
        }

        // Start microphone capture (looping). (hykql原创)
        _micClip = Microphone.Start(null, true, 1, SampleRate);
        while (Microphone.GetPosition(null) <= 0)
        {
            yield return null; // wait for mic to start (hykql原创)
        }

        _lastSamplePosition = 0;
        _running = true;
        if (hud)
        {
            hud.Show();
            hud.SetState(HudController.HudState.Idle);
            hud.SetSubtitle("Ready");
        }

        StartCoroutine(CaptureLoop());
    }

    private bool InitNative()
    {
        try
        {
            var parms = new JarvisNative.AudioParams
            {
                sample_rate_hz = SampleRate,
                num_channels = Channels,
                frame_samples = FrameSamples,
                enable_aec = false,
                enable_ns = true,
                enable_agc = true,
                enable_vad = true
            };

            var ares = JarvisNative.jarvis_audio_init(ref parms);
            var wres = JarvisNative.jarvis_wake_init(string.IsNullOrEmpty(wakeModelPath) ? null : wakeModelPath, wakeSensitivity);
            var asrRes = JarvisNative.jarvis_asr_init(string.IsNullOrEmpty(asrModelDir) ? null : asrModelDir, asrLang);

            if (ares != 0 || wres != 0 || asrRes != 0)
            {
                Debug.LogError($"Jarvis native init failed (audio={ares}, wake={wres}, asr={asrRes})");
                return false;
            }

            return true;
        }
        catch (DllNotFoundException ex)
        {
            Debug.LogError("Native libraries not found: " + ex.Message);
            return false;
        }
        catch (Exception ex)
        {
            Debug.LogError("Init error: " + ex);
            return false;
        }
    }

    private IEnumerator CaptureLoop()
    {
        while (_running)
        {
            var currentPos = Microphone.GetPosition(null);
            var clipSamples = _micClip.samples;
            var available = currentPos - _lastSamplePosition;
            if (available < 0)
            {
                available += clipSamples; // wrapped around (hykql原创)
            }

            while (available >= FrameSamples)
            {
                ReadFrame(_lastSamplePosition, clipSamples);
                ProcessFrame();

                _lastSamplePosition = (_lastSamplePosition + FrameSamples) % clipSamples;
                available -= FrameSamples;
            }

            yield return null;
        }
    }

    private void ReadFrame(int startSample, int clipSamples)
    {
        if (startSample + FrameSamples <= clipSamples)
        {
            _micClip.GetData(_floatBuffer, startSample);
        }
        else
        {
            // Wrap-around read in two parts. (hykql原创)
            int tail = clipSamples - startSample;
            int head = FrameSamples - tail;

            if (tail > 0)
            {
                var tailBuf = new float[tail];
                _micClip.GetData(tailBuf, startSample);
                Array.Copy(tailBuf, 0, _floatBuffer, 0, tail);
            }

            if (head > 0)
            {
                var headBuf = new float[head];
                _micClip.GetData(headBuf, 0);
                Array.Copy(headBuf, 0, _floatBuffer, tail, head);
            }
        }

        for (int i = 0; i < FrameSamples; i++)
        {
            var s = Mathf.Clamp(_floatBuffer[i], -1f, 1f);
            _pcmBuffer[i] = (short)(s * short.MaxValue);
        }
    }

    private void ProcessFrame()
    {
        bool speech = false;
        var apm = JarvisNative.jarvis_audio_process(_pcmBuffer, FrameSamples, ref speech);
        if (apm != 0)
        {
            Debug.LogWarning($"jarvis_audio_process returned {apm}");
        }

        if (_asrActive)
        {
            var asrPush = JarvisNative.jarvis_asr_push(_pcmBuffer, FrameSamples);
            if (asrPush != 0)
            {
                Debug.LogWarning($"jarvis_asr_push returned {asrPush}");
            }
        }

        float score;
        var wake = JarvisNative.jarvis_wake_score(_pcmBuffer, FrameSamples, out score);
        if (wake > 0)
        {
            OnWakeDetected?.Invoke();
            if (hud)
            {
                hud.Show();
                hud.SetState(HudController.HudState.Listening);
                hud.SetSubtitle("Wake detected");
            }
        }

        if (!_wasSpeech && speech)
        {
            OnVadStarted?.Invoke();
            if (hud)
            {
                hud.SetState(HudController.HudState.Speaking);
                hud.SetSubtitle("Listening...");
            }
        }
        else if (_wasSpeech && !speech)
        {
            OnVadEnded?.Invoke();
            if (hud)
            {
                hud.SetState(HudController.HudState.Idle);
                hud.SetSubtitle("Ready");
            }
        }

        _wasSpeech = speech;
    }

    private void OnDestroy()
    {
        _running = false;
        if (Microphone.IsRecording(null))
        {
            Microphone.End(null);
        }

        JarvisNative.jarvis_audio_close();
        JarvisNative.jarvis_wake_close();
        JarvisNative.jarvis_asr_close();
    }

    public void StartAsrStream()
    {
        var res = JarvisNative.jarvis_asr_start_stream();
        if (res != 0)
        {
            Debug.LogWarning($"jarvis_asr_start_stream returned {res}");
        }
        _asrActive = true;
    }

    public void StopAsrStream()
    {
        _asrActive = false;
    }
}
