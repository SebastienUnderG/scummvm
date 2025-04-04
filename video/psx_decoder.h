/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef VIDEO_PSX_DECODER_H
#define VIDEO_PSX_DECODER_H

#include "common/bitstream.h"
#include "common/endian.h"
#include "common/rational.h"
#include "common/rect.h"
#include "common/str.h"
#include "graphics/surface.h"
#include "video/video_decoder.h"

namespace Audio {
class QueuingAudioStream;
}

namespace Common {
template <class BITSTREAM>
class Huffman;
}

namespace Graphics {
struct PixelFormat;
}

namespace Video {

/**
 * Decoder for PSX stream videos.
 * This currently implements the most basic PSX stream format that is
 * used by most games on the system. Special variants are not supported
 * at this time.
 *
 * Video decoder used in engines:
 *  - sword1 (psx)
 *  - sword2 (psx)
 */
class PSXStreamDecoder : public VideoDecoder {
public:
	// CD speed in sectors/second
	// Calling code should use these enum values instead of the constants
	enum CDSpeed {
		kCD1x = 75,
		kCD2x = 150
	};

	PSXStreamDecoder(CDSpeed speed, uint32 frameCount = 0);
	virtual ~PSXStreamDecoder() override;

	bool loadStream(Common::SeekableReadStream *stream) override;
	void close() override;

protected:
	void readNextPacket() override;
	bool useAudioSync() const override;

private:
	class PSXVideoTrack : public VideoTrack {
	public:
		PSXVideoTrack(Common::SeekableReadStream *firstSector, CDSpeed speed, int frameCount);
		~PSXVideoTrack() override;

		uint16 getWidth() const override { return _width; }
		uint16 getHeight() const override { return _height; }
		Graphics::PixelFormat getPixelFormat() const override { return _pixelFormat; }
		bool setOutputPixelFormat(const Graphics::PixelFormat &format) override {
			if (format.bytesPerPixel != 2 && format.bytesPerPixel != 4)
				return false;
			_pixelFormat = format;
			return true;
		}
		bool endOfTrack() const override { return _endOfTrack; }
		int getCurFrame() const override { return _curFrame; }
		int getFrameCount() const override { return _frameCount; }
		uint32 getNextFrameStartTime() const override;
		const Graphics::Surface *decodeNextFrame() override;

		void setEndOfTrack() { _endOfTrack = true; }
		void decodeFrame(Common::BitStreamMemoryStream *frame, uint sectorCount);

	private:
		Graphics::Surface *_surface;
		Graphics::PixelFormat _pixelFormat;
		uint16 _width;
		uint16 _height;
		uint32 _frameCount;
		Audio::Timestamp _nextFrameStartTime;
		bool _endOfTrack;
		int _curFrame;

		enum PlaneType {
			kPlaneY = 0,
			kPlaneU = 1,
			kPlaneV = 2
		};

		typedef Common::Huffman<Common::BitStreamMemory16LEMSB> HuffmanDecoder;

		uint16 _macroBlocksW, _macroBlocksH;
		byte *_yBuffer, *_cbBuffer, *_crBuffer;
		void decodeMacroBlock(Common::BitStreamMemory16LEMSB *bits, int mbX, int mbY, uint16 scale, uint16 version);
		void decodeBlock(Common::BitStreamMemory16LEMSB *bits, byte *block, int pitch, uint16 scale, uint16 version, PlaneType plane);

		void readAC(Common::BitStreamMemory16LEMSB *bits, int *block);
		HuffmanDecoder *_acHuffman;

		int readDC(Common::BitStreamMemory16LEMSB *bits, uint16 version, PlaneType plane);
		HuffmanDecoder *_dcHuffmanLuma, *_dcHuffmanChroma;
		int _lastDC[3];

		void dequantizeBlock(int *coefficients, float *block, uint16 scale);
		void idct(float *dequantData, float *result);
		int readSignedCoefficient(Common::BitStreamMemory16LEMSB *bits);
	};

	class PSXAudioTrack : public AudioTrack {
	public:
		PSXAudioTrack(Common::SeekableReadStream *sector, Audio::Mixer::SoundType soundType);
		~PSXAudioTrack() override;

		bool endOfTrack() const override;

		void setEndOfTrack() { _endOfTrack = true; }
		void queueAudioFromSector(Common::SeekableReadStream *sector);

	private:
		Audio::AudioStream *getAudioStream() const override;

		Audio::QueuingAudioStream *_audStream;

		bool _endOfTrack;
		bool _stereo;
		uint _rate;
	};

	CDSpeed _speed;
	uint32 _frameCount;
	Common::SeekableReadStream *_stream;
	PSXVideoTrack *_videoTrack;
	PSXAudioTrack *_audioTrack;

	Common::SeekableReadStream *readSector();
};

} // End of namespace Video

#endif
