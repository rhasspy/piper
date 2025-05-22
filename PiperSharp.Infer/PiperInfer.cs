using System;
using System.Collections.Generic;
using System.Diagnostics; // For Debug.WriteLine
using System.Globalization; // For StringInfo
using System.IO; // For File operations
using System.Linq; // For Linq operations
using System.Text; // For Encoding
using System.Text.Json; 
using System.Runtime.InteropServices; // Added for P/Invoke
using Microsoft.ML.OnnxRuntime; // Added for ONNX Runtime
using Microsoft.ML.OnnxRuntime.Tensors; // Added for ONNX Runtime Tensors

// It's good practice to define type aliases within a namespace
// or use specific struct/class types if more complex behavior is needed.
// For simplicity and direct translation of typedefs:
namespace PiperSharp.Infer
{
    // Phoneme typically represents a Unicode codepoint.
    // piper-phonemize/phonemize.hpp defines Phoneme as char32_t, which is uint in C#.
    public struct Phoneme : IEquatable<Phoneme>
    {
        public uint Value { get; }
        public Phoneme(uint value) { Value = value; }
        public static implicit operator uint(Phoneme p) => p.Value;
        public static implicit operator Phoneme(uint val) => new Phoneme(val);
        public override bool Equals(object? obj) => obj is Phoneme other && Equals(other);
        public bool Equals(Phoneme other) => Value == other.Value;
        public override int GetHashCode() => Value.GetHashCode();
        public override string ToString() => char.ConvertFromUtf32((int)Value); // Basic representation
    }

    // PhonemeId is an integer identifier for a phoneme.
    // piper-phonemize/phoneme_ids.hpp defines PhonemeId as int64_t.
    public struct PhonemeId : IEquatable<PhonemeId>
    {
        public long Value { get; }
        public PhonemeId(long value) { Value = value; }
        public static implicit operator long(PhonemeId id) => id.Value;
        public static implicit operator PhonemeId(long val) => new PhonemeId(val);
        public override bool Equals(object? obj) => obj is PhonemeId other && Equals(other);
        public bool Equals(PhonemeId other) => Value == other.Value;
        public override int GetHashCode() => Value.GetHashCode();
    }

    // SpeakerId is an integer identifier for a speaker.
    // Defined as int64_t in piper.hpp
    public struct SpeakerId : IEquatable<SpeakerId>
    {
        public long Value { get; }
        public SpeakerId(long value) { Value = value; }
        public static implicit operator long(SpeakerId id) => id.Value;
        public static implicit operator SpeakerId(long val) => new SpeakerId(val);
        public override bool Equals(object? obj) => obj is SpeakerId other && Equals(other);
        public bool Equals(SpeakerId other) => Value == other.Value;
        public override int GetHashCode() => Value.GetHashCode();
    }

    public enum PhonemeType
    {
        ESpeakPhonemes,
        TextPhonemes
    }

    public class ESpeakConfig
    {
        public string Voice { get; set; } = "en-us";
    }

    public class PiperConfig
    {
        public string? ESpeakDataPath { get; set; } // std::string
        public bool UseESpeak { get; set; } = true;
        public bool UseTashkeel { get; set; } = false;
        public string? TashkeelModelPath { get; set; } // std::optional<std::string>

        // Placeholder for std::unique_ptr<tashkeel::State> tashkeelState;
        // This will likely be an IDisposable class wrapping the C++ object or its C# equivalent.
        public object? TashkeelState { get; set; } // For now, an object placeholder
    }

    public class PhonemizeConfig
    {
        public PhonemeType PhonemeType { get; set; } = PhonemeType.ESpeakPhonemes;
        public Dictionary<Phoneme, List<Phoneme>>? PhonemeMap { get; set; } // std::optional<std::map<Phoneme, std::vector<Phoneme>>>
        public Dictionary<Phoneme, List<PhonemeId>> PhonemeIdMap { get; set; } = new(); // std::map<Phoneme, std::vector<PhonemeId>>

        public PhonemeId IdPad { get; set; } = new PhonemeId(0);
        public PhonemeId IdBos { get; set; } = new PhonemeId(1);
        public PhonemeId IdEos { get; set; } = new PhonemeId(2);
        public bool InterspersePad { get; set; } = true;

        public ESpeakConfig ESpeak { get; set; } = new();
    }

    public class SynthesisConfig
    {
        public float NoiseScale { get; set; } = 0.667f;
        public float LengthScale { get; set; } = 1.0f;
        public float NoiseW { get; set; } = 0.8f;

        public int SampleRate { get; set; } = 22050;
        public int SampleWidth { get; set; } = 2; // 16-bit
        public int Channels { get; set; } = 1;    // mono

        public SpeakerId? SpeakerId { get; set; } // std::optional<SpeakerId>

        public float SentenceSilenceSeconds { get; set; } = 0.2f;
        public Dictionary<Phoneme, float>? PhonemeSilenceSeconds { get; set; } // std::optional<std::map<piper::Phoneme, float>>
    }

    public class ModelConfig
    {
        public int NumSpeakers { get; set; } // C++ int, typically maps to C# int
        public Dictionary<string, SpeakerId>? SpeakerIdMap { get; set; } // std::optional<std::map<std::string, SpeakerId>>
    }

    public class ModelSession : IDisposable
    {
        public OrtEnv? Env { get; private set; } 
        public SessionOptions? Options { get; private set; } 
        public InferenceSession? OnnxSession { get; private set; }

        public ModelSession() { } 

        public void Initialize(string modelPath, bool useCuda)
        {
            // Env must be created before SessionOptions and InferenceSession.
            // GetEnvironment() gets a singleton, which is fine for many cases.
            // If specific OrtEnv settings are needed, create with "new OrtEnv(...)".
            Env = OrtEnv.GetEnvironment(); 
            Options = new SessionOptions();

            if (useCuda)
            {
                try 
                {
                    // Attempt to append CUDA execution provider.
                    // The device_id parameter (0 in this case) specifies which CUDA device to use.
                    Options.AppendExecutionProvider_CUDA(0); 
                    Debug.WriteLine("CUDA Execution Provider enabled.");
                } 
                catch (Exception e) // Catch OrtException or general Exception
                {
                    // Log the failure and continue with CPU.
                    // ONNX Runtime will fall back to CPU if CUDA is not available or fails to initialize.
                    Debug.WriteLine($"Failed to enable CUDA Execution Provider: {e.Message}. Falling back to CPU.");
                }
            }
            
            Options.GraphOptimizationLevel = GraphOptimizationLevel.ORT_DISABLE_ALL;
            Options.EnableCpuMemArena = false;
            Options.EnableMemPattern = false;
            // Options.EnableProfiling = false; // Not a direct property, profile output path sets it.
            // If profiling is not needed, this line can be omitted.
            // To disable profiling explicitly, ensure ProfileOutputPathPrefix is not set.
            // Options.ProfileOutputPathPrefix = "onnx_profile"; // This would enable it

            OnnxSession = new InferenceSession(modelPath, Options);
        }

        public void Dispose()
        {
            // Dispose order: InferenceSession, then SessionOptions.
            // OrtEnv obtained via GetEnvironment() is a singleton and typically not disposed here.
            // If Env was created with "new OrtEnv()", it should be disposed: Env?.Dispose();
            OnnxSession?.Dispose();
            Options?.Dispose();
            // Env?.Dispose(); // Only if 'new OrtEnv()' was used for Env.
        }
    }

    public class SynthesisResult
    {
        public double InferSeconds { get; set; }
        public double AudioSeconds { get; set; }
        public double RealTimeFactor { get; set; }
    }

    public class Voice : IDisposable // Implementing IDisposable for Voice if it owns ModelSession
    {
        public JsonDocument? ConfigRoot { get; set; }
        public PhonemizeConfig PhonemizeConfig { get; set; } = new();
        public SynthesisConfig SynthesisConfig { get; set; } = new();
        public ModelConfig ModelConfig { get; set; } = new();
        public ModelSession Session { get; set; } = new(); 

        public void Dispose()
        {
            ConfigRoot?.Dispose(); // JsonDocument is IDisposable
            Session?.Dispose();    // ModelSession is now IDisposable
        }
    }

    public static class PiperRunner
    {
        private const string Version = ""; 
        private const float MaxWavValue = 32767.0f;
        // private const string InstanceName = "piper"; // InstanceName for OrtEnv (if creating new)

        public static string GetVersion() => Version;

        private static bool IsSingleCodepoint(string s)
        {
            if (string.IsNullOrEmpty(s)) return false;
            try
            {
                _ = char.ConvertToUtf32(s, 0); 
                return s.Length == char.ConvertFromUtf32(char.ConvertToUtf32(s,0)).Length;
            }
            catch (ArgumentException) { return false; }
        }

        private static Phoneme GetCodepoint(string s)
        {
            if (string.IsNullOrEmpty(s))
            {
                throw new ArgumentException("String cannot be null or empty.", nameof(s));
            }
            return new Phoneme((uint)char.ConvertToUtf32(s, 0));
        }
        
        public static void ParsePhonemizeConfig(JsonElement configRoot, PhonemizeConfig phonemizeConfig)
        {
            if (configRoot.TryGetProperty("espeak", out var espeakValue) &&
                espeakValue.TryGetProperty("voice", out var voiceValue))
            {
                phonemizeConfig.ESpeak.Voice = voiceValue.GetString() ?? phonemizeConfig.ESpeak.Voice;
            }

            if (configRoot.TryGetProperty("phoneme_type", out var phonemeTypeStrValue))
            {
                if (phonemeTypeStrValue.GetString() == "text")
                {
                    phonemizeConfig.PhonemeType = PhonemeType.TextPhonemes;
                }
            }

            if (configRoot.TryGetProperty("phoneme_id_map", out var phonemeIdMapValue))
            {
                phonemizeConfig.PhonemeIdMap = new Dictionary<Phoneme, List<PhonemeId>>();
                foreach (var property in phonemeIdMapValue.EnumerateObject())
                {
                    string fromPhoneme = property.Name;
                    if (!IsSingleCodepoint(fromPhoneme))
                    {
                        Debug.WriteLine($"Error: \"{fromPhoneme}\" is not a single codepoint (phoneme id map)");
                        throw new ArgumentException("Phonemes must be one codepoint (phoneme id map)");
                    }
                    var fromCodepoint = GetCodepoint(fromPhoneme);
                    var idList = new List<PhonemeId>();
                    foreach (var toIdValue in property.Value.EnumerateArray())
                    {
                        idList.Add(new PhonemeId(toIdValue.GetInt64()));
                    }
                    phonemizeConfig.PhonemeIdMap[fromCodepoint] = idList;
                }
            }

            if (configRoot.TryGetProperty("phoneme_map", out var phonemeMapValue))
            {
                phonemizeConfig.PhonemeMap = new Dictionary<Phoneme, List<Phoneme>>();
                foreach (var property in phonemeMapValue.EnumerateObject())
                {
                    string fromPhoneme = property.Name;
                    if (!IsSingleCodepoint(fromPhoneme))
                    {
                        Debug.WriteLine($"Error: \"{fromPhoneme}\" is not a single codepoint (phoneme map)");
                        throw new ArgumentException("Phonemes must be one codepoint (phoneme map)");
                    }
                    var fromCodepoint = GetCodepoint(fromPhoneme);
                    var phonemeList = new List<Phoneme>();
                    foreach (var toPhonemeValue in property.Value.EnumerateArray())
                    {
                        string toPhoneme = toPhonemeValue.GetString() ?? "";
                        if (!IsSingleCodepoint(toPhoneme))
                        {
                            throw new ArgumentException("Phonemes must be one codepoint (phoneme map)");
                        }
                        phonemeList.Add(GetCodepoint(toPhoneme));
                    }
                    phonemizeConfig.PhonemeMap[fromCodepoint] = phonemeList;
                }
            }
        }

        public static void ParseSynthesisConfig(JsonElement configRoot, SynthesisConfig synthesisConfig)
        {
            if (configRoot.TryGetProperty("audio", out var audioValue) &&
                audioValue.TryGetProperty("sample_rate", out var sampleRateValue))
            {
                synthesisConfig.SampleRate = sampleRateValue.TryGetInt32(out int sr) ? sr : 22050;
            }

            if (configRoot.TryGetProperty("inference", out var inferenceValue))
            {
                if (inferenceValue.TryGetProperty("noise_scale", out var noiseScaleValue))
                {
                    synthesisConfig.NoiseScale = noiseScaleValue.TryGetSingle(out float ns) ? ns : 0.667f;
                }
                if (inferenceValue.TryGetProperty("length_scale", out var lengthScaleValue))
                {
                    synthesisConfig.LengthScale = lengthScaleValue.TryGetSingle(out float ls) ? ls : 1.0f;
                }
                if (inferenceValue.TryGetProperty("noise_w", out var noiseWValue))
                {
                    synthesisConfig.NoiseW = noiseWValue.TryGetSingle(out float nw) ? nw : 0.8f;
                }

                if (inferenceValue.TryGetProperty("phoneme_silence", out var phonemeSilenceValue))
                {
                    synthesisConfig.PhonemeSilenceSeconds = new Dictionary<Phoneme, float>();
                    foreach (var property in phonemeSilenceValue.EnumerateObject())
                    {
                        string phonemeStr = property.Name;
                        if (!IsSingleCodepoint(phonemeStr))
                        {
                             Debug.WriteLine($"Error: \"{phonemeStr}\" is not a single codepoint (phoneme silence)");
                            throw new ArgumentException("Phonemes must be one codepoint (phoneme silence)");
                        }
                        var phoneme = GetCodepoint(phonemeStr);
                        if (property.Value.TryGetSingle(out float silence))
                        {
                            synthesisConfig.PhonemeSilenceSeconds[phoneme] = silence;
                        }
                    }
                }
            }
        }
        
        public static void ParseModelConfig(JsonElement configRoot, ModelConfig modelConfig)
        {
            if (configRoot.TryGetProperty("num_speakers", out var numSpeakersValue))
            {
                modelConfig.NumSpeakers = numSpeakersValue.TryGetInt32(out int ns) ? ns : 1;
            }
            else 
            {
                modelConfig.NumSpeakers = 1;
            }

            if (configRoot.TryGetProperty("speaker_id_map", out var speakerIdMapValue))
            {
                modelConfig.SpeakerIdMap = new Dictionary<string, SpeakerId>();
                foreach (var property in speakerIdMapValue.EnumerateObject())
                {
                    modelConfig.SpeakerIdMap[property.Name] = new SpeakerId(property.Value.GetInt64());
                }
            }
        }

        public static void Initialize(PiperConfig config)
        {
            if (config.UseESpeak)
            {
                Debug.WriteLine("Initializing eSpeak");
                int result = ESpeakNgNative.espeak_Initialize(
                    ESpeakNgNative.espeak_AUDIO_OUTPUT.AUDIO_OUTPUT_SYNCHRONOUS,
                    0, 
                    string.IsNullOrEmpty(config.ESpeakDataPath) ? null : config.ESpeakDataPath, 
                    0  
                );
                if (result < 0) 
                {
                    throw new Exception($"Failed to initialize eSpeak-ng. Error code: {result}");
                }
                Debug.WriteLine($"Initialized eSpeak. Sample rate: {result}");
            }

            if (config.UseTashkeel)
            {
                Debug.WriteLine("Using libtashkeel for diacritization");
                if (string.IsNullOrEmpty(config.TashkeelModelPath))
                {
                    throw new ArgumentException("No path to libtashkeel model");
                }
                Debug.WriteLine($"Loading libtashkeel model from {config.TashkeelModelPath}");
                Debug.WriteLine("libtashkeel model loading placeholder.");
                Debug.WriteLine("Initialized libtashkeel");
            }
            Debug.WriteLine("Initialized piper");
        }

        public static void Terminate(PiperConfig config)
        {
            if (config.UseESpeak)
            {
                Debug.WriteLine("Terminating eSpeak");
                int result = ESpeakNgNative.espeak_Terminate();
                if (result != ESpeakNgNative.EE_OK)
                {
                    Debug.WriteLine($"eSpeak_Terminate returned error code: {result}");
                }
                Debug.WriteLine("Terminated eSpeak");
            }
            Debug.WriteLine("Terminated piper");
        }

        public static void LoadModel(string modelPath, ModelSession session, bool useCuda)
        {
            Debug.WriteLine($"Loading ONNX model from {modelPath}");
            var startTime = DateTime.Now;
            
            session.Initialize(modelPath, useCuda); // Call the new Initialize method
            
            var endTime = DateTime.Now;
            Debug.WriteLine($"Loaded ONNX model in {(endTime - startTime).TotalSeconds:F3} second(s)");
        }

        public static void LoadVoice(PiperConfig config, string modelPath, string modelConfigPath, Voice voice, ref SpeakerId? speakerId, bool useCuda)
        {
            Debug.WriteLine($"Parsing voice config at {modelConfigPath}");
            string jsonString = File.ReadAllText(modelConfigPath);
            voice.ConfigRoot = JsonDocument.Parse(jsonString);
            JsonElement rootElement = voice.ConfigRoot.RootElement;

            ParsePhonemizeConfig(rootElement, voice.PhonemizeConfig);
            ParseSynthesisConfig(rootElement, voice.SynthesisConfig);
            ParseModelConfig(rootElement, voice.ModelConfig);
            
            if (voice.ModelConfig.NumSpeakers > 1)
            {
                if (speakerId.HasValue)
                {
                    voice.SynthesisConfig.SpeakerId = speakerId;
                }
                else
                {
                    voice.SynthesisConfig.SpeakerId = new SpeakerId(0); // Default speaker
                }
            }
            Debug.WriteLine($"Voice contains {voice.ModelConfig.NumSpeakers} speaker(s)");
            LoadModel(modelPath, voice.Session, useCuda);
        }

        public static void Synthesize(List<PhonemeId> phonemeIds, SynthesisConfig synthesisConfig, ModelSession session, List<short> audioBuffer, SynthesisResult result)
        {
            if (session.OnnxSession == null)
            {
                throw new InvalidOperationException("ONNX session is not initialized.");
            }

            Debug.WriteLine($"Synthesizing audio for {phonemeIds.Count} phoneme id(s)");

            long[] phonemeIdArray = phonemeIds.Select(pid => pid.Value).ToArray();
            int[] phonemeIdsShape = { 1, phonemeIdArray.Length }; // Batch size of 1
            var inputTensor = new DenseTensor<long>(phonemeIdArray, phonemeIdsShape);

            long[] phonemeIdLengthsArray = { (long)phonemeIdArray.Length };
            int[] phonemeIdLengthsShape = { phonemeIdLengthsArray.Length }; // Shape for a scalar tensor in some frameworks, or [1]
            var inputLengthsTensor = new DenseTensor<long>(phonemeIdLengthsArray, phonemeIdLengthsShape);

            float[] scalesArray = { synthesisConfig.NoiseScale, synthesisConfig.LengthScale, synthesisConfig.NoiseW };
            int[] scalesShape = { scalesArray.Length }; // This implies a 1D tensor of 3 elements
            var scalesTensor = new DenseTensor<float>(scalesArray, scalesShape);

            var inputs = new List<NamedOnnxValue>
            {
                NamedOnnxValue.CreateFromTensor("input", inputTensor),
                NamedOnnxValue.CreateFromTensor("input_lengths", inputLengthsTensor),
                NamedOnnxValue.CreateFromTensor("scales", scalesTensor)
            };

            if (synthesisConfig.SpeakerId.HasValue)
            {
                long[] speakerIdArray = { synthesisConfig.SpeakerId.Value.Value };
                int[] speakerIdShape = { speakerIdArray.Length }; // Shape for a scalar tensor
                var speakerIdTensor = new DenseTensor<long>(speakerIdArray, speakerIdShape);
                inputs.Add(NamedOnnxValue.CreateFromTensor("sid", speakerIdTensor));
            }
            
            var inferStartTime = DateTime.Now;
            using (var outputs = session.OnnxSession.Run(inputs))
            {
                var inferEndTime = DateTime.Now;
                result.InferSeconds = (inferEndTime - inferStartTime).TotalSeconds;

                var outputAudioTensor = outputs.FirstOrDefault()?.AsTensor<float>();
                if (outputAudioTensor == null)
                {
                    throw new Exception("Failed to get output audio tensor from ONNX inference.");
                }

                var audioData = outputAudioTensor.ToArray(); 
                long audioCount = outputAudioTensor.Length;
                result.AudioSeconds = (double)audioCount / synthesisConfig.SampleRate;

                if (result.AudioSeconds > 0)
                {
                    result.RealTimeFactor = result.InferSeconds / result.AudioSeconds;
                }
                else
                {
                    result.RealTimeFactor = 0; // Avoid division by zero if no audio is produced
                }
                Debug.WriteLine($"Synthesized {result.AudioSeconds:F3}s of audio in {result.InferSeconds:F3}s (RTF: {result.RealTimeFactor:F3}x)");

                float maxAudioValue = 0.01f;
                for (int i = 0; i < audioCount; i++)
                {
                    float audioValue = Math.Abs(audioData[i]);
                    if (audioValue > maxAudioValue)
                    {
                        maxAudioValue = audioValue;
                    }
                }

                audioBuffer.Capacity = audioBuffer.Count + (int)audioCount;
                float audioScale = (MaxWavValue / Math.Max(0.01f, maxAudioValue));
                for (int i = 0; i < audioCount; i++)
                {
                    short intAudioValue = (short)Math.Clamp(audioData[i] * audioScale, short.MinValue, short.MaxValue);
                    audioBuffer.Add(intAudioValue);
                }
            }
        }

        public static void TextToAudio(PiperConfig config, Voice voice, string text, List<short> audioBuffer, SynthesisResult result, Action? audioCallback)
        {
            long sentenceSilenceSamples = 0;
            if (voice.SynthesisConfig.SentenceSilenceSeconds > 0)
            {
                sentenceSilenceSamples = (long)(voice.SynthesisConfig.SentenceSilenceSeconds * voice.SynthesisConfig.SampleRate * voice.SynthesisConfig.Channels);
            }

            if (config.UseTashkeel)
            {
                if (config.TashkeelState == null)
                {
                    throw new InvalidOperationException("Tashkeel model is not loaded");
                }
                Debug.WriteLine($"Diacritizing text with libtashkeel: {text}");
                Debug.WriteLine("Tashkeel processing placeholder.");
            }

            Debug.WriteLine($"Phonemizing text: {text}");
            List<List<Phoneme>> sentencesPhonemes; 
            IntPtr phonemesJsonPtr = IntPtr.Zero;
            int phonemizeResult;

            try
            {
                if (voice.PhonemizeConfig.PhonemeType == PhonemeType.ESpeakPhonemes)
                {
                    ESpeakPhonemeConfigNative nativeESpeakConfig;
                    nativeESpeakConfig.voice = voice.PhonemizeConfig.ESpeak.Voice;
                    phonemizeResult = PiperPhonemizeNative.phonemize_eSpeak(text, ref nativeESpeakConfig, out phonemesJsonPtr);
                    if (phonemizeResult != 0)
                    {
                        throw new Exception($"phonemize_eSpeak failed with code: {phonemizeResult}");
                    }
                }
                else 
                {
                    CodepointsPhonemeConfigNative nativeCodepointsConfig = new CodepointsPhonemeConfigNative();
                    phonemizeResult = PiperPhonemizeNative.phonemize_codepoints(text, ref nativeCodepointsConfig, out phonemesJsonPtr);
                    if (phonemizeResult != 0)
                    {
                        throw new Exception($"phonemize_codepoints failed with code: {phonemizeResult}");
                    }
                }

                string? phonemesJson = Marshal.PtrToStringAnsi(phonemesJsonPtr);
                if (string.IsNullOrEmpty(phonemesJson)) 
                {
                     throw new Exception("Failed to marshal phonemes JSON from native code (null or empty string).");
                }

                List<List<uint>>? phonemeCodepoints = JsonSerializer.Deserialize<List<List<uint>>>(phonemesJson);
                if (phonemeCodepoints == null)
                {
                    throw new Exception("Failed to deserialize phoneme codepoints JSON.");
                }
                sentencesPhonemes = phonemeCodepoints.Select(sentenceList => sentenceList.Select(cp => new Phoneme(cp)).ToList()).ToList();
            }
            finally
            {
                if (phonemesJsonPtr != IntPtr.Zero)
                {
                    PiperPhonemizeNative.free_piper_phonemize_string(phonemesJsonPtr);
                }
            }
            
            List<PhonemeId> currentSentencePhonemeIds = new List<PhonemeId>();
            Dictionary<Phoneme, long> missingPhonemesOverall = new Dictionary<Phoneme, long>();

            foreach (var sentencePhonemeList in sentencesPhonemes)
            {
                if (sentencePhonemeList.Count == 0) continue;
                
                currentSentencePhonemeIds.Clear(); 

                List<uint> currentSentenceCodepoints = sentencePhonemeList.Select(p => p.Value).ToList();
                string sentenceJson = JsonSerializer.Serialize(currentSentenceCodepoints);

                PhonemeIdConfigNative idConfigNative;
                idConfigNative.idPad = voice.PhonemizeConfig.IdPad.Value;
                idConfigNative.idBos = voice.PhonemizeConfig.IdBos.Value;
                idConfigNative.idEos = voice.PhonemizeConfig.IdEos.Value;
                idConfigNative.interspersePad = voice.PhonemizeConfig.InterspersePad;

                IntPtr phonemeIdsJsonPtr = IntPtr.Zero;
                IntPtr missingPhonemesJsonPtr = IntPtr.Zero;
                
                try
                {
                    int toIdsResult = PiperPhonemizeNative.phonemes_to_ids(sentenceJson, ref idConfigNative, out phonemeIdsJsonPtr, out missingPhonemesJsonPtr);
                    if (toIdsResult != 0)
                    {
                        throw new Exception($"phonemes_to_ids failed with code: {toIdsResult}");
                    }

                    string? phonemeIdsJson = Marshal.PtrToStringAnsi(phonemeIdsJsonPtr);
                    if (string.IsNullOrEmpty(phonemeIdsJson))
                    {
                        throw new Exception("Failed to marshal phoneme IDs JSON from native code (null or empty string).");
                    }
                    List<long>? currentPhonemeIdValues = JsonSerializer.Deserialize<List<long>>(phonemeIdsJson);
                     if (currentPhonemeIdValues == null)
                    {
                        throw new Exception("Failed to deserialize phoneme IDs JSON.");
                    }
                    currentSentencePhonemeIds.AddRange(currentPhonemeIdValues.Select(idVal => new PhonemeId(idVal)));

                    string? missingPhonemesJson = Marshal.PtrToStringAnsi(missingPhonemesJsonPtr);
                    if (string.IsNullOrEmpty(missingPhonemesJson))
                    {
                        throw new Exception("Failed to marshal missing phonemes JSON from native code (null or empty string).");
                    }
                    Dictionary<uint, ulong>? currentMissingPhonemesMap = JsonSerializer.Deserialize<Dictionary<uint, ulong>>(missingPhonemesJson);
                    if (currentMissingPhonemesMap == null)
                    {
                         throw new Exception("Failed to deserialize missing phonemes JSON.");
                    }

                    foreach (var entry in currentMissingPhonemesMap)
                    {
                        Phoneme missingPhoneme = new Phoneme(entry.Key);
                        if (missingPhonemesOverall.ContainsKey(missingPhoneme))
                        {
                            missingPhonemesOverall[missingPhoneme] += (long)entry.Value;
                        }
                        else
                        {
                            missingPhonemesOverall[missingPhoneme] = (long)entry.Value;
                        }
                    }
                }
                finally
                {
                    if (phonemeIdsJsonPtr != IntPtr.Zero)
                    {
                        PiperPhonemizeNative.free_piper_phonemize_string(phonemeIdsJsonPtr);
                    }
                    if (missingPhonemesJsonPtr != IntPtr.Zero)
                    {
                        PiperPhonemizeNative.free_piper_phonemize_string(missingPhonemesJsonPtr);
                    }
                }

                SynthesisResult phraseResult = new SynthesisResult();
                Synthesize(currentSentencePhonemeIds, voice.SynthesisConfig, voice.Session, audioBuffer, phraseResult);
                
                result.AudioSeconds += phraseResult.AudioSeconds;
                result.InferSeconds += phraseResult.InferSeconds;

                if (sentenceSilenceSamples > 0)
                {
                    for (long i = 0; i < sentenceSilenceSamples; i++)
                    {
                        audioBuffer.Add(0);
                    }
                }

                audioCallback?.Invoke(); 
                if(audioCallback != null) audioBuffer.Clear(); 
            }

            if (missingPhonemesOverall.Count > 0)
            {
                Debug.WriteLine($"Warning: Missing {missingPhonemesOverall.Count} distinct phoneme(s) from phoneme/id map!");
                foreach (var entry in missingPhonemesOverall)
                {
                    Debug.WriteLine($"Warning: Missing \"{entry.Key}\" (U+{(entry.Key.Value):X4}): {entry.Value} time(s)");
                }
            }

            if (result.AudioSeconds > 0)
            {
                result.RealTimeFactor = result.InferSeconds / result.AudioSeconds;
            }
        }

        public static void TextToWavFile(PiperConfig config, Voice voice, string text, Stream audioFile, SynthesisResult result)
        {
            List<short> audioBuffer = new List<short>();
            TextToAudio(config, voice, text, audioBuffer, result, null);
            
            Debug.WriteLine("Writing WAV data to stream.");
            using (BinaryWriter writer = new BinaryWriter(audioFile, Encoding.UTF8, true)) 
            {
                // Retrieve audio parameters
                int sampleRate = voice.SynthesisConfig.SampleRate;
                int channels = voice.SynthesisConfig.Channels;
                int bitsPerSample = voice.SynthesisConfig.SampleWidth * 8; // SampleWidth is in bytes

                // Calculate header values
                int numSamples = audioBuffer.Count; // This is total samples for all channels
                // If audioBuffer contains interleaved samples for multi-channel, then numSamples is correct.
                // If audioBuffer contains samples for a single channel and needs to be interleaved, this changes.
                // Assuming audioBuffer already contains the correct total number of samples (interleaved if multi-channel).
                int dataSize = numSamples * (bitsPerSample / 8); // Total size of audio data in bytes
                int chunkSize = 36 + dataSize; // RIFF chunk size
                short blockAlign = (short)(channels * (bitsPerSample / 8));
                int byteRate = sampleRate * blockAlign;

                // RIFF Chunk
                writer.Write(Encoding.ASCII.GetBytes("RIFF"));
                writer.Write(chunkSize);
                writer.Write(Encoding.ASCII.GetBytes("WAVE"));

                // fmt Chunk
                writer.Write(Encoding.ASCII.GetBytes("fmt "));
                writer.Write(16); // Subchunk1Size for PCM
                writer.Write((short)1); // AudioFormat (1 for PCM)
                writer.Write((short)channels);
                writer.Write(sampleRate);
                writer.Write(byteRate);
                writer.Write(blockAlign);
                writer.Write((short)bitsPerSample);

                // data Chunk
                writer.Write(Encoding.ASCII.GetBytes("data"));
                writer.Write(dataSize);

                // Write audio samples
                foreach (short sample in audioBuffer)
                {
                    writer.Write(sample);
                }
            }
            Debug.WriteLine("Finished writing WAV data.");
        }
    }
}
