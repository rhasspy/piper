using Microsoft.VisualStudio.TestTools.UnitTesting;
using PiperSharp.Infer;
using System;
using System.IO;
using System.Text;
using System.Collections.Generic; // Required for List<short>

namespace PiperSharp.Infer.Tests
{
    [TestClass]
    public class WavFileTests
    {
        private PiperConfig CreateDummyPiperConfig()
        {
            // eSpeak data path is not strictly needed if eSpeak P/Invoke is stubbed or fails gracefully.
            return new PiperConfig { UseESpeak = true, ESpeakDataPath = null }; 
        }

        private Voice CreateDummyVoice(int sampleRate, short channels, short sampleWidthBits)
        {
            var voice = new Voice
            {
                SynthesisConfig = new SynthesisConfig
                {
                    SampleRate = sampleRate,
                    Channels = channels,
                    SampleWidth = (short)(sampleWidthBits / 8) // SampleWidth is in bytes
                },
                PhonemizeConfig = new PhonemizeConfig // Default is ESpeakPhonemes
                {
                    // Ensure PhonemeIdMap is initialized to avoid NullReferenceException
                    // if the dummy phonemization path tries to access it.
                    PhonemeIdMap = new Dictionary<Phoneme, List<PhonemeId>>() 
                }, 
                ModelConfig = new ModelConfig { NumSpeakers = 1 } // Default to 1 speaker
            };
            // The ModelSession will use its default constructor.
            // The dummy Synthesize logic does not currently rely on a real ONNX session.
            return voice;
        }

        [TestMethod]
        [DataRow(22050, (short)1, (short)16)] // Mono, 16-bit, 22050Hz
        [DataRow(16000, (short)2, (short)16)] // Stereo, 16-bit, 16000Hz
        [DataRow(44100, (short)1, (short)16)] // Mono, 16-bit, 44100Hz
        public void TextToWavFile_WritesCorrectHeader(int sampleRate, short channels, short bitsPerSample)
        {
            var piperConfig = CreateDummyPiperConfig();
            var voice = CreateDummyVoice(sampleRate, channels, bitsPerSample);
            var result = new SynthesisResult();
            string text = "hello world"; // Dummy text

            // The dummy audio generation in Synthesize is based on phoneme ID count.
            // The dummy phonemization in TextToAudio (if P/Invoke fails or not present)
            // might result in very few or zero phoneme IDs.
            // For header testing, audio data length is important.
            // Let's ensure TextToAudio produces *some* dummy data or handle potential empty audio.
            // The current dummy path in TextToAudio (if P/Invoke fails) for eSpeak uses:
            // var dummySentence = new List<Phoneme> { 'h', 'e', 'l', 'l', 'o' }; phonemes.Add(dummySentence);
            // And phonemes_to_ids uses default ID 0 if map is empty.
            // This should produce some dummy audio.

            using (var memoryStream = new MemoryStream())
            {
                try
                {
                    PiperRunner.TextToWavFile(piperConfig, voice, text, memoryStream, result);
                }
                catch (DllNotFoundException libEx)
                {
                    Assert.Inconclusive($"A native library ({libEx.Message.Split('\'')[1]}) was not found. WAV header test cannot proceed. Error: {libEx.Message}");
                    return;
                }
                catch (Exception ex)
                {
                    // Catch other exceptions from TextToWavFile to provide more context
                    Assert.Fail($"TextToWavFile threw an unexpected exception: {ex.ToString()}");
                    return; 
                }

                Assert.IsTrue(memoryStream.Length >= 44, "WAV stream is too short to contain a valid header.");
                memoryStream.Position = 0;

                using (var reader = new BinaryReader(memoryStream, Encoding.UTF8, false)) // Keep stream open for length check
                {
                    // RIFF Chunk
                    Assert.AreEqual("RIFF", Encoding.ASCII.GetString(reader.ReadBytes(4)), "RIFF ID is incorrect.");
                    int chunkSize = reader.ReadInt32(); // Overall file size - 8 bytes
                    Assert.AreEqual("WAVE", Encoding.ASCII.GetString(reader.ReadBytes(4)), "WAVE ID is incorrect.");

                    // fmt Chunk
                    Assert.AreEqual("fmt ", Encoding.ASCII.GetString(reader.ReadBytes(4)), "fmt ID is incorrect.");
                    Assert.AreEqual(16, reader.ReadInt32(), "fmt chunk size is incorrect (should be 16 for PCM).");
                    Assert.AreEqual((short)1, reader.ReadInt16(), "AudioFormat should be 1 for PCM.");
                    Assert.AreEqual(channels, reader.ReadInt16(), "Number of channels is incorrect.");
                    Assert.AreEqual(sampleRate, reader.ReadInt32(), "SampleRate is incorrect.");
                    
                    int expectedByteRate = sampleRate * channels * (bitsPerSample / 8);
                    Assert.AreEqual(expectedByteRate, reader.ReadInt32(), "ByteRate is incorrect.");
                    
                    short expectedBlockAlign = (short)(channels * (bitsPerSample / 8));
                    Assert.AreEqual(expectedBlockAlign, reader.ReadInt16(), "BlockAlign is incorrect.");
                    Assert.AreEqual(bitsPerSample, reader.ReadInt16(), "BitsPerSample is incorrect.");

                    // data Chunk
                    // It's possible there are other chunks before 'data' (e.g. 'fact'), but piper.cpp writes 'data' directly after 'fmt '.
                    // If this test fails here, check the WAV structure written by PiperRunner.
                    string dataChunkId = Encoding.ASCII.GetString(reader.ReadBytes(4));
                    Assert.AreEqual("data", dataChunkId, "data ID is incorrect. Found: " + dataChunkId);
                    
                    int dataSize = reader.ReadInt32(); // Size of the audio data

                    // Verify sizes
                    // audioBuffer.Count * sizeof(short) was how dataSize was calculated in TextToWavFile
                    // So, dataSize should be (memoryStream.Length - 44) if header is exactly 44 bytes.
                    // More robustly: chunkSize = 36 + dataSize
                    Assert.AreEqual(memoryStream.Length - 8, chunkSize, "ChunkSize (file size - 8) in RIFF header is incorrect.");
                    Assert.AreEqual(memoryStream.Length - 44, dataSize, "DataSize in 'data' chunk header is incorrect relative to stream length.");
                }
            }
        }
    }
}
