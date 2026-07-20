using System;
using System.IO;
using System.Text;
using GameHQ.Playnite.Protocol;
using Xunit;

namespace GameHQ.Playnite.Tests
{
    public class PipeFramingTests
    {
        [Fact]
        public void RoundTripsAPayload()
        {
            var payload = Encoding.UTF8.GetBytes("{\"type\":\"hello\"}");
            using (var stream = new MemoryStream())
            {
                PipeFraming.WriteFrame(stream, payload);
                stream.Position = 0;

                var read = PipeFraming.ReadFrame(stream);

                Assert.Equal(payload, read);
            }
        }

        [Fact]
        public void ReturnsNullOnCleanEndOfStreamBeforeAFrame()
        {
            using (var stream = new MemoryStream())
            {
                var read = PipeFraming.ReadFrame(stream);

                Assert.Null(read);
            }
        }

        [Fact]
        public void ThrowsOnFrameLargerThanTheLimit()
        {
            var payload = new byte[PipeFraming.MaxFrameBytes + 1];

            using (var stream = new MemoryStream())
            {
                Assert.Throws<InvalidOperationException>(() => PipeFraming.WriteFrame(stream, payload));
            }
        }

        [Fact]
        public void ThrowsOnTruncatedFrameHeader()
        {
            using (var stream = new MemoryStream(new byte[] { 1, 2 }))
            {
                Assert.Throws<IOException>(() => PipeFraming.ReadFrame(stream));
            }
        }

        [Fact]
        public void ThrowsOnTruncatedFramePayload()
        {
            using (var stream = new MemoryStream())
            {
                var header = BitConverter.GetBytes(10);
                stream.Write(header, 0, header.Length);
                stream.Write(new byte[] { 1, 2, 3 }, 0, 3); // fewer than the declared 10 bytes
                stream.Position = 0;

                Assert.Throws<IOException>(() => PipeFraming.ReadFrame(stream));
            }
        }

        [Fact]
        public void ThrowsOnZeroLengthFrame()
        {
            using (var stream = new MemoryStream())
            {
                Assert.Throws<InvalidOperationException>(() => PipeFraming.WriteFrame(stream, new byte[0]));
            }
        }
    }
}
