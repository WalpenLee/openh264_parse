#pragma once

class ISVCEncoder;
struct TagEncParamExt;
struct Source_Picture_s;
namespace VideoEncoder
{
	class OpenH264Encoder
	{
	public:
		OpenH264Encoder();
		~OpenH264Encoder();

	public:
		bool Init(int src_w, int src_h, int fps, int bit_rate, bool temporal);

		bool Release();

		bool Encode(const char* in_data, unsigned __int64 time, char* out_data, int& temoral_id, int& frame_type);

	private:
		ISVCEncoder* encoder_;
		TagEncParamExt* encode_param_;
		Source_Picture_s* src_pic_;
		char* pic_data_temp_;
		bool init_encoder_;
		bool temporal_;
	};
}
