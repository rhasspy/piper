using Microsoft.VisualStudio.TestTools.UnitTesting;
using PiperSharp.Infer;
using System.Text.Json;
using System.Collections.Generic;
using System.Linq;

namespace PiperSharp.Infer.Tests
{
    [TestClass]
    public class ConfigParsingTests
    {
        [TestMethod]
        public void ParsePhonemizeConfig_ParsesCorrectly()
        {
            string jsonString = @"
            {
                ""espeak"": { ""voice"": ""en-GB"" },
                ""phoneme_type"": ""text"",
                ""phoneme_id_map"": {
                    ""A"": [65, 0],
                    ""B"": [66, 1]
                },
                ""phoneme_map"": {
                    ""C"": [""X"", ""Y""]
                }
            }";
            // Note: For Phoneme (uint) keys in JSON, they must be strings.
            // The C++ code uses from_phoneme.at(0) which implies single char for phoneme keys.
            // For PhonemeIdMap, keys are Phoneme (uint), values are List<PhonemeId (long)>
            // For PhonemeMap, keys are Phoneme (uint), values are List<Phoneme (uint)>

            using (JsonDocument doc = JsonDocument.Parse(jsonString))
            {
                PhonemizeConfig config = new PhonemizeConfig(); // Use default values as base
                PiperRunner.ParsePhonemizeConfig(doc.RootElement, config);

                Assert.AreEqual("en-GB", config.ESpeak.Voice);
                Assert.AreEqual(PhonemeType.TextPhonemes, config.PhonemeType);

                Assert.IsNotNull(config.PhonemeIdMap);
                Assert.AreEqual(2, config.PhonemeIdMap.Count);
                // Assuming 'A' (U+0041) and 'B' (U+0042) are the phonemes
                Phoneme phonemeA = new Phoneme(0x0041); // 'A'
                Phoneme phonemeB = new Phoneme(0x0042); // 'B'
                Assert.IsTrue(config.PhonemeIdMap.ContainsKey(phonemeA));
                Assert.AreEqual(2, config.PhonemeIdMap[phonemeA].Count);
                Assert.AreEqual(new PhonemeId(65), config.PhonemeIdMap[phonemeA][0]);
                Assert.AreEqual(new PhonemeId(0), config.PhonemeIdMap[phonemeA][1]);
                Assert.IsTrue(config.PhonemeIdMap.ContainsKey(phonemeB));
                Assert.AreEqual(new PhonemeId(66), config.PhonemeIdMap[phonemeB][0]);

                Assert.IsNotNull(config.PhonemeMap);
                Assert.AreEqual(1, config.PhonemeMap.Count);
                Phoneme phonemeC = new Phoneme(0x0043); // 'C'
                Phoneme phonemeX = new Phoneme(0x0058); // 'X'
                Phoneme phonemeY = new Phoneme(0x0059); // 'Y'
                Assert.IsTrue(config.PhonemeMap.ContainsKey(phonemeC));
                Assert.AreEqual(2, config.PhonemeMap[phonemeC].Count);
                Assert.AreEqual(phonemeX, config.PhonemeMap[phonemeC][0]);
                Assert.AreEqual(phonemeY, config.PhonemeMap[phonemeC][1]);
            }
        }

        [TestMethod]
        public void ParseSynthesisConfig_ParsesCorrectly()
        {
            string jsonString = @"
            {
                ""audio"": { ""sample_rate"": 16000 },
                ""inference"": {
                    ""noise_scale"": 0.5,
                    ""length_scale"": 1.1,
                    ""noise_w"": 0.7,
                    ""phoneme_silence"": {
                        ""P"": 0.1
                    }
                }
            }";
            // For phoneme_silence, key is Phoneme (uint)
            using (JsonDocument doc = JsonDocument.Parse(jsonString))
            {
                SynthesisConfig config = new SynthesisConfig(); // Use default values
                PiperRunner.ParseSynthesisConfig(doc.RootElement, config);

                Assert.AreEqual(16000, config.SampleRate);
                Assert.AreEqual(0.5f, config.NoiseScale);
                Assert.AreEqual(1.1f, config.LengthScale);
                Assert.AreEqual(0.7f, config.NoiseW);

                Assert.IsNotNull(config.PhonemeSilenceSeconds);
                Assert.AreEqual(1, config.PhonemeSilenceSeconds.Count);
                Phoneme phonemeP = new Phoneme(0x0050); // 'P'
                Assert.IsTrue(config.PhonemeSilenceSeconds.ContainsKey(phonemeP));
                Assert.AreEqual(0.1f, config.PhonemeSilenceSeconds[phonemeP]);
            }
        }

        [TestMethod]
        public void ParseModelConfig_ParsesCorrectly()
        {
            string jsonString = @"
            {
                ""num_speakers"": 2,
                ""speaker_id_map"": {
                    ""speakerA"": 0,
                    ""speakerB"": 1
                }
            }";
            using (JsonDocument doc = JsonDocument.Parse(jsonString))
            {
                ModelConfig config = new ModelConfig();
                PiperRunner.ParseModelConfig(doc.RootElement, config);

                Assert.AreEqual(2, config.NumSpeakers);
                Assert.IsNotNull(config.SpeakerIdMap);
                Assert.AreEqual(2, config.SpeakerIdMap.Count);
                Assert.AreEqual(new SpeakerId(0), config.SpeakerIdMap["speakerA"]);
                Assert.AreEqual(new SpeakerId(1), config.SpeakerIdMap["speakerB"]);
            }
        }
         [TestMethod]
        public void ParseModelConfig_DefaultsNumSpeakersToOne_IfOmitted()
        {
            string jsonString = @"
            {
                ""speaker_id_map"": {
                    ""speakerA"": 0
                }
            }";
            // num_speakers is omitted
            using (JsonDocument doc = JsonDocument.Parse(jsonString))
            {
                ModelConfig config = new ModelConfig();
                // Set a different default to ensure parsing changes it
                config.NumSpeakers = 99; 
                PiperRunner.ParseModelConfig(doc.RootElement, config);

                // PiperRunner.ParseModelConfig sets NumSpeakers to 1 if not found in JSON.
                Assert.AreEqual(1, config.NumSpeakers);
                Assert.IsNotNull(config.SpeakerIdMap);
                Assert.AreEqual(1, config.SpeakerIdMap.Count);
            }
        }
    }
}
